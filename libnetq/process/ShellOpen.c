/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/process/ShellOpen.h"

#include <libnetq/Env.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/string/String.h>
#include <libnetq/Malloc.h>

/* Windows needs Shell32: without it there is nothing to open with */
#if defined(NQ_OS_WINDOWS) && defined(WITH_SHELL32)
# define OPEN_WINDOWS 1
/* macOS needs CoreServices (Launch Services) */
#elif defined(NQ_OS_MACOSX) && defined(WITH_CORESERVICES)
# define OPEN_MACOS 1
#elif defined(NQ_OS_UNIX) && !defined(NQ_OS_ANDROID) && !defined(NQ_OS_DARWIN) && !defined(__EMSCRIPTEN__)
# define OPEN_XDG 1
#endif

#if defined(OPEN_WINDOWS)
# include <windows.h>
# include <wchar.h>
# if defined(WITH_OLE32)
#  include <objbase.h>
# endif
# include <shellapi.h>
# include <shlobj.h>
# if defined(WITH_SHLWAPI)
#  include <shlwapi.h>
# endif
#elif defined(OPEN_MACOS)
# include <CoreServices/CoreServices.h>
# include <dirent.h>
# include <dlfcn.h>
# include <errno.h>
# include <objc/message.h>
# include <objc/runtime.h>
# include <signal.h>
# include <stdlib.h>
# include <strings.h>
# include <sys/stat.h>
# include <unistd.h>
#elif defined(OPEN_XDG)
# include <dirent.h>
# include <errno.h>
# include <fcntl.h>
# include <signal.h>
# include <spawn.h>
# include <stdlib.h>
# include <strings.h>
# include <sys/stat.h>
# include <sys/wait.h>
# include <unistd.h>
extern char** environ;
#endif

/* --- what NQ_SHELL_OPEN_WAIT gives ---------------------------------------------- */

struct NQShellOpenWait {
  size_t count;
  size_t cap;
  /* process IDs; on Windows process HANDLEs */
  int64_t* ids;
};

#if defined(OPEN_WINDOWS) || defined(OPEN_MACOS) || defined(OPEN_XDG)

/* Adds a process to *wait, made when there is none yet */
static int wait_add(NQShellOpenWait** wait, int64_t id)
{
  if (!*wait) {
    *wait = NQZeroMalloc(sizeof(**wait));
    if (!*wait)
      return -NQ_ENOMEM;
  }
  NQShellOpenWait* w = *wait;
  if (w->count == w->cap) {
    size_t cap = w->cap ? w->cap * 2 : 4;
    int64_t* ids = NQRealloc(w->ids, cap * sizeof(*ids));
    if (!ids)
      return -NQ_ENOMEM;
    w->ids = ids;
    w->cap = cap;
  }
  w->ids[w->count++] = id;
  return 0;
}

#endif

/* --- strings -------------------------------------------------------------- */

#if defined(OPEN_MACOS) || defined(OPEN_XDG)

typedef struct Str {
  char* p;
  size_t len;
  size_t cap;
  int failed;
} Str;

static void str_addn(Str* s, const char* t, size_t n)
{
  if (s->failed)
    return;
  if (s->len + n + 1 > s->cap) {
    size_t cap = s->cap ? s->cap : 64;
    while (cap < s->len + n + 1)
      cap *= 2;
    char* p = NQRealloc(s->p, cap);
    if (!p) {
      s->failed = 1;
      return;
    }
    s->p = p;
    s->cap = cap;
  }
  NQMemcpy(s->p + s->len, t, n);
  s->len += n;
  s->p[s->len] = '\0';
}

static void str_add(Str* s, const char* t)
{
  str_addn(s, t, NQStrlen(t));
}

static void str_addc(Str* s, char c)
{
  str_addn(s, &c, 1);
}

/* Takes the string, NULL if it could not be made */
static char* str_take(Str* s)
{
  if (!s->failed && !s->p)
    str_addn(s, "", 0);
  char* p = s->failed ? NULL : s->p;
  if (s->failed)
    NQFree(s->p);
  s->p = NULL;
  s->len = s->cap = 0;
  s->failed = 0;
  return p;
}

static char* str_dup(const char* t)
{
  Str s = { 0 };
  str_add(&s, t);
  return str_take(&s);
}

/* "dir/name" */
static char* path_join(const char* dir, const char* name)
{
  Str s = { 0 };
  str_add(&s, dir);
  if (s.len == 0 || s.p[s.len - 1] != '/')
    str_addc(&s, '/');
  str_add(&s, name);
  return str_take(&s);
}

static int is_regular(const char* path)
{
  struct stat st;
  return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

#if defined(OPEN_MACOS)
static int is_directory(const char* path)
{
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}
#endif

/* A list of strings it owns */
typedef struct StrList {
  char** v;
  size_t n;
  size_t cap;
  int failed;
} StrList;

/* Adds `s`, which it takes over */
static void list_take(StrList* l, char* s)
{
  if (!s)
    l->failed = 1;
  if (l->failed) {
    NQFree(s);
    return;
  }
  if (l->n + 2 > l->cap) {
    size_t cap = l->cap ? l->cap * 2 : 8;
    char** v = NQRealloc(l->v, cap * sizeof(*v));
    if (!v) {
      l->failed = 1;
      NQFree(s);
      return;
    }
    l->v = v;
    l->cap = cap;
  }
  l->v[l->n++] = s;
  l->v[l->n] = NULL;
}

static void list_add(StrList* l, const char* s)
{
  list_take(l, str_dup(s));
}

static void list_free(StrList* l)
{
  for (size_t i = 0; i < l->n; i++)
    NQFree(l->v[i]);
  NQFree(l->v);
  NQMemset(l, 0, sizeof(*l));
}

/* "file:///a%20b" for "/a b" */
static char* file_uri(const char* path)
{
  static const char hex[] = "0123456789ABCDEF";
  Str s = { 0 };
  str_add(&s, "file://");
  for (const unsigned char* p = (const unsigned char*)path; *p; p++) {
    unsigned char c = *p;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '.'
        || c == '_' || c == '~' || c == '/') {
      str_addc(&s, (char)c);
    } else {
      str_addc(&s, '%');
      str_addc(&s, hex[c >> 4]);
      str_addc(&s, hex[c & 15]);
    }
  }
  return str_take(&s);
}

static int is_path(const char* item)
{
  return item[0] == '/';
}

#endif /* OPEN_MACOS || OPEN_XDG */

/* ========================================================================== */
/* Linux and the BSDs: freedesktop.org                                        */
/* ========================================================================== */

#if defined(OPEN_XDG)

/* How a program is started */
enum {
  RUN_DETACHED, /* on its own, not waited for */
  RUN_SYNC,     /* waited for: returns its exit status */
  RUN_CHILD,    /* left running as our child, for NQShellOpenWait */
};

/* The program's path from PATH, NULL if there is none */
static char* find_program(const char* name)
{
  if (!*name)
    return NULL;
  if (NQStrchr(name, '/'))
    return access(name, X_OK) == 0 && is_regular(name) ? str_dup(name) : NULL;
  const char* env_path = NQEnvCreate("PATH");
  const char* path = env_path && *env_path ? env_path : "/usr/local/bin:/usr/bin:/bin";
  char* found = NULL;
  while (*path && !found) {
    const char* end = NQStrchr(path, ':');
    size_t len = end ? (size_t)(end - path) : strlen(path);
    Str s = { 0 };
    if (len == 0)
      str_add(&s, ".");
    else
      str_addn(&s, path, len);
    char* dir = str_take(&s);
    char* candidate = dir ? path_join(dir, name) : NULL;
    NQFree(dir);
    if (candidate && access(candidate, X_OK) == 0 && is_regular(candidate))
      found = candidate;
    else
      NQFree(candidate);
    if (!end)
      break;
    path = end + 1;
  }
  NQEnvFree(env_path);
  return found;
}

/* --- the environment of what is started ------------------------------------ */

/* "NAME=value": is it `name`? */
static int env_is(const char* entry, const char* name, size_t len)
{
  return strncmp(entry, name, len) == 0 && entry[len] == '=';
}

/* NQEnvCreate of a name `len` bytes long; NQEnvFree frees it */
static const char* env_create_n(const char* name, size_t len)
{
  Str s = { 0 };
  str_addn(&s, name, len);
  char* n = str_take(&s);
  const char* value = n ? NQEnvCreate(n) : NULL;
  NQFree(n);
  return value;
}

/* Is the directory (len bytes) `prefix` or below it? */
static int dir_under(const char* dir, size_t len, const char* prefix)
{
  size_t n = prefix ? strlen(prefix) : 0;
  while (n > 1 && prefix[n - 1] == '/')
    n--;
  return n > 0 && len >= n && strncmp(dir, prefix, n) == 0 && (len == n || dir[n] == '/');
}

/* "NAME=a:b:c" without the directories of the snap; NULL when none is left */
static char* without_snap_dirs(const char* entry, size_t name_len, const char* const* snap_dirs)
{
  Str s = { 0 };
  str_addn(&s, entry, name_len + 1);
  size_t kept = 0;
  for (const char* p = entry + name_len + 1;;) {
    const char* end = strchr(p, ':');
    size_t len = end ? (size_t)(end - p) : strlen(p);
    int drop = 0;
    for (size_t i = 0; snap_dirs[i]; i++)
      drop |= dir_under(p, len, snap_dirs[i]);
    if (len > 0 && !drop) {
      if (kept++)
        str_addc(&s, ':');
      str_addn(&s, p, len);
    }
    if (!end)
      break;
    p = end + 1;
  }
  char* out = str_take(&s);
  if (!kept) {
    NQFree(out);
    return NULL;
  }
  return out;
}

#define VSCODE_ORIG "_VSCODE_SNAP_ORIG"

/* The environment for an application the desktop would start: ours, less
 * what a snap we were started from (VS Code's, say) set up for itself. Its
 * GSETTINGS_SCHEMA_DIR, GTK_PATH, GIO_MODULE_DIR and XDG_DATA_DIRS make
 * the system's GTK applications load the snap's libraries and schemas,
 * and abort. VS Code keeps what it changed as NAME_VSCODE_SNAP_ORIG
 * (empty when NAME was not set), which is put back. NULL: no memory. */
static char** child_environment(void)
{
  static const char* const snap_vars[] = {
    "GSETTINGS_SCHEMA_DIR", "GTK_PATH", "GTK_EXE_PREFIX", "GTK_IM_MODULE_FILE", "GTK_MODULES",
    "GDK_PIXBUF_MODULE_FILE", "GDK_PIXBUF_MODULEDIR", "GIO_MODULE_DIR", "GIO_EXTRA_MODULES", "LOCPATH",
    "LIBGL_DRIVERS_PATH", "LIBVA_DRIVERS_PATH", "__EGL_VENDOR_LIBRARY_DIRS", "FONTCONFIG_FILE", "FONTCONFIG_PATH",
    "SNAP", NULL,
  };
  static const char* const snap_lists[] = {
    "XDG_DATA_DIRS", "XDG_CONFIG_DIRS", "PATH", "LD_LIBRARY_PATH", "GI_TYPELIB_PATH", NULL,
  };
  /* what its launcher says about the process that started us */
  static const char* const launcher_vars[] = {
    "GIO_LAUNCHED_DESKTOP_FILE", "GIO_LAUNCHED_DESKTOP_FILE_PID", "DESKTOP_STARTUP_ID", "XDG_ACTIVATION_TOKEN",
    NULL,
  };
  const char* env_snap = NQEnvCreate("SNAP");
  const char* env_user_data = NQEnvCreate("SNAP_USER_DATA");
  const char* env_user_common = NQEnvCreate("SNAP_USER_COMMON");
  const char* snap_dirs[] = { env_snap, env_user_data, env_user_common, NULL };
  const char* snap = env_snap && *env_snap ? env_snap : NULL;
  size_t olen = strlen(VSCODE_ORIG);

  StrList env = { 0 };
  for (char** e = environ; *e; e++) {
    const char* eq = strchr(*e, '=');
    if (!eq)
      continue;
    size_t len = (size_t)(eq - *e);
    if (len > olen && strncmp(eq - olen, VSCODE_ORIG, olen) == 0) {
      /* put back what was there before; added here when NAME is not set */
      const char* now = env_create_n(*e, len - olen);
      int set = now != NULL;
      NQEnvFree(now);
      if (eq[1] && !set) {
        Str s = { 0 };
        str_addn(&s, *e, len - olen);
        str_add(&s, eq);
        list_take(&env, str_take(&s));
      }
      continue;
    }
    Str orig_name = { 0 };
    str_addn(&orig_name, *e, len);
    str_add(&orig_name, VSCODE_ORIG);
    char* oname = str_take(&orig_name);
    const char* orig = oname ? NQEnvCreate(oname) : NULL;
    NQFree(oname);
    if (orig) {
      if (*orig) {
        Str s = { 0 };
        str_addn(&s, *e, len + 1);
        str_add(&s, orig);
        list_take(&env, str_take(&s));
      }
      NQEnvFree(orig);
      continue;
    }
    int drop = 0;
    for (size_t i = 0; launcher_vars[i]; i++)
      drop |= env_is(*e, launcher_vars[i], strlen(launcher_vars[i]));
    if (snap) {
      for (size_t i = 0; snap_vars[i]; i++)
        drop |= env_is(*e, snap_vars[i], strlen(snap_vars[i]));
      drop |= strncmp(*e, "SNAP_", 5) == 0;
    }
    if (drop)
      continue;
    int list = 0;
    for (size_t i = 0; snap && snap_lists[i]; i++)
      list |= env_is(*e, snap_lists[i], strlen(snap_lists[i]));
    if (list) {
      char* filtered = without_snap_dirs(*e, len, snap_dirs);
      if (filtered)
        list_take(&env, filtered);
      continue;
    }
    list_add(&env, *e);
  }
  NQEnvFree(env_snap);
  NQEnvFree(env_user_data);
  NQEnvFree(env_user_common);
  if (env.failed) {
    list_free(&env);
    return NULL;
  }
  return env.v ? env.v : NQZeroMalloc(sizeof(char*)); /* or an empty one */
}

static void free_environment(char** env)
{
  if (!env)
    return;
  for (char** e = env; *e; e++)
    NQFree(*e);
  NQFree(env);
}

/* Starts argv (argv[0] a path), its standard streams on /dev/null, in a
 * session of its own with the signals as a new program has them. Returns
 * 0, or for RUN_SYNC the exit status (127 when it was not started) */
static int run(char* const* argv, int how, pid_t* child)
{
  posix_spawn_file_actions_t actions;
  posix_spawnattr_t attr;
  if (posix_spawn_file_actions_init(&actions) != 0)
    return -NQ_ENOMEM;
  if (posix_spawnattr_init(&attr) != 0) {
    posix_spawn_file_actions_destroy(&actions);
    return -NQ_ENOMEM;
  }
  posix_spawn_file_actions_addopen(&actions, 0, "/dev/null", O_RDONLY, 0);
  posix_spawn_file_actions_addopen(&actions, 1, "/dev/null", O_WRONLY, 0);
  posix_spawn_file_actions_adddup2(&actions, 1, 2);

  sigset_t mask, defaults;
  sigemptyset(&mask);
  sigemptyset(&defaults);
  static const int reset[] = { SIGPIPE, SIGINT, SIGQUIT, SIGTERM, SIGHUP, SIGCHLD, SIGUSR1, SIGUSR2 };
  for (size_t i = 0; i < sizeof(reset) / sizeof(reset[0]); i++)
    sigaddset(&defaults, reset[i]);
  posix_spawnattr_setsigmask(&attr, &mask);
  posix_spawnattr_setsigdefault(&attr, &defaults);
  short flags = POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF;
# ifdef POSIX_SPAWN_SETSID
  flags |= POSIX_SPAWN_SETSID;
# else
  flags |= POSIX_SPAWN_SETPGROUP;
  posix_spawnattr_setpgroup(&attr, 0);
# endif
  posix_spawnattr_setflags(&attr, flags);

  /* A detached program is started by a shell that does not wait for it,
   * so that it is not left to us to reap */
  StrList shell = { 0 };
  char* const* to_run = argv;
  if (how == RUN_DETACHED) {
    list_add(&shell, "/bin/sh");
    list_add(&shell, "-c");
    list_add(&shell, "\"$0\" \"$@\" &");
    for (size_t i = 0; argv[i]; i++)
      list_add(&shell, argv[i]);
    if (shell.failed) {
      list_free(&shell);
      posix_spawnattr_destroy(&attr);
      posix_spawn_file_actions_destroy(&actions);
      return -NQ_ENOMEM;
    }
    to_run = shell.v;
  }

  pid_t pid;
  char** env = child_environment();
  int ret = env ? posix_spawn(&pid, to_run[0], &actions, &attr, to_run, env) : ENOMEM;
  free_environment(env);
  list_free(&shell);
  posix_spawnattr_destroy(&attr);
  posix_spawn_file_actions_destroy(&actions);
  if (ret != 0)
    return -ret;

  if (how == RUN_CHILD) {
    *child = pid;
    return 0;
  }
  int status;
  while (waitpid(pid, &status, 0) < 0) {
    if (errno != EINTR)
      return 0; /* reaped by someone else */
  }
  if (how == RUN_DETACHED)
    return 0;
  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  return 128 + (WIFSIGNALED(status) ? WTERMSIG(status) : 0);
}

/* Starts argv: its program is looked up in PATH */
static int launch(StrList* argv, int wait, NQShellOpenWait** waiting)
{
  char* program = find_program(argv->v[0]);
  if (!program)
    return -NQ_ENOENT;
  NQFree(argv->v[0]);
  argv->v[0] = program;
  pid_t pid;
  int ret = run(argv->v, wait ? RUN_CHILD : RUN_DETACHED, &pid);
  if (ret == 0 && wait)
    ret = wait_add(waiting, pid);
  return ret;
}

/* --- desktop files ---------------------------------------------------------- */

/* $XDG_x_HOME (or ~/default_home), then each of $XDG_x_DIRS, each with
 * `sub` appended */
static void xdg_dirs(StrList* dirs, const char* home_var, const char* default_home, const char* dirs_var,
                     const char* default_dirs, const char* sub)
{
  const char* xdg_home = NQEnvCreate(home_var);
  if (xdg_home && *xdg_home) {
    list_take(dirs, path_join(xdg_home, sub));
  } else {
    const char* home = NQEnvCreate("HOME");
    if (home && *home) {
      char* base = path_join(home, default_home);
      list_take(dirs, base ? path_join(base, sub) : NULL);
      NQFree(base);
    }
    NQEnvFree(home);
  }
  NQEnvFree(xdg_home);
  const char* env_list = NQEnvCreate(dirs_var);
  const char* list = env_list && *env_list ? env_list : default_dirs;
  while (*list) {
    const char* end = NQStrchr(list, ':');
    size_t len = end ? (size_t)(end - list) : strlen(list);
    if (len > 0) {
      Str s = { 0 };
      str_addn(&s, list, len);
      char* dir = str_take(&s);
      list_take(dirs, dir ? path_join(dir, sub) : NULL);
      NQFree(dir);
    }
    if (!end)
      break;
    list = end + 1;
  }
  NQEnvFree(env_list);
}

static void application_dirs(StrList* dirs)
{
  xdg_dirs(dirs, "XDG_DATA_HOME", ".local/share", "XDG_DATA_DIRS", "/usr/local/share:/usr/share", "applications");
}

/* A file's contents, NUL-terminated; NULL if it cannot be read */
static char* read_text(const char* path)
{
  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return NULL;
  Str s = { 0 };
  char buf[4096];
  ssize_t n;
  while ((n = read(fd, buf, sizeof(buf))) > 0 && s.len < (1u << 20))
    str_addn(&s, buf, (size_t)n);
  close(fd);
  return n < 0 ? (NQFree(s.p), NULL) : str_take(&s);
}

/* The value of `key` in the group `group` of a desktop entry style file,
 * as it is written; NULL if it has none */
static char* ini_get(const char* text, const char* group, const char* key)
{
  size_t glen = NQStrlen(group), klen = NQStrlen(key);
  int in_group = 0;
  for (const char* line = text; line && *line;) {
    const char* end = NQStrchr(line, '\n');
    size_t len = end ? (size_t)(end - line) : strlen(line);
    if (len > 0 && line[len - 1] == '\r')
      len--;
    if (line[0] == '[') {
      in_group = len == glen + 2 && line[len - 1] == ']' && NQStrncmp(line + 1, group, glen) == 0;
    } else if (in_group && len > klen && NQStrncmp(line, key, klen) == 0) {
      const char* p = line + klen;
      const char* stop = line + len;
      while (p < stop && (*p == ' ' || *p == '\t'))
        p++;
      if (p < stop && *p == '=') {
        p++;
        while (p < stop && (*p == ' ' || *p == '\t'))
          p++;
        Str s = { 0 };
        str_addn(&s, p, (size_t)(stop - p));
        return str_take(&s);
      }
    }
    line = end ? end + 1 : NULL;
  }
  return NULL;
}

/* A string value with its escapes (\s \n \t \r \\) undone */
static char* unescape_value(const char* v)
{
  Str s = { 0 };
  for (; *v; v++) {
    if (*v == '\\' && v[1]) {
      v++;
      switch (*v) {
      case 's': str_addc(&s, ' '); break;
      case 'n': str_addc(&s, '\n'); break;
      case 't': str_addc(&s, '\t'); break;
      case 'r': str_addc(&s, '\r'); break;
      default: str_addc(&s, '\\'); str_addc(&s, *v); break; /* left for the Exec quoting */
      }
    } else {
      str_addc(&s, *v);
    }
  }
  return str_take(&s);
}

static int is_application(const char* text)
{
  char* type = ini_get(text, "Desktop Entry", "Type");
  char* hidden = ini_get(text, "Desktop Entry", "Hidden");
  int ok = type && NQStrcmp(type, "Application") == 0 && !(hidden && NQStrcmp(hidden, "true") == 0);
  NQFree(type);
  NQFree(hidden);
  return ok;
}

/* How a desktop file is looked for */
enum {
  MATCH_ID,      /* its desktop file ID, exactly */
  MATCH_ID_CASE, /* the ID, in any case, with or without ".desktop" */
  MATCH_NAME,    /* its Name, in any case */
};

static int desktop_matches(const char* dir, const char* file, const char* name, int how)
{
  size_t flen = NQStrlen(file), nlen = NQStrlen(name);
  if (flen <= 8 || NQStrcmp(file + flen - 8, ".desktop") != 0)
    return 0;
  if (how == MATCH_ID_CASE) {
    if (NQStrcasecmp(file, name) == 0)
      return 1;
    return nlen + 8 == flen && NQStrncasecmp(file, name, nlen) == 0;
  }
  if (how != MATCH_NAME)
    return 0;
  char* path = path_join(dir, file);
  char* text = path ? read_text(path) : NULL;
  int found = 0;
  if (text && is_application(text)) {
    char* raw = ini_get(text, "Desktop Entry", "Name");
    char* value = raw ? unescape_value(raw) : NULL;
    found = value && NQStrcasecmp(value, name) == 0;
    NQFree(raw);
    NQFree(value);
  }
  NQFree(text);
  NQFree(path);
  return found;
}

/* The path of the application's desktop file, NULL if there is none;
 * with by_name it may be given as the user knows it, as well as by ID */
static char* find_desktop_file(const char* name, int by_name)
{
  if (!*name || strchr(name, '/'))
    return NULL;
  StrList dirs = { 0 };
  application_dirs(&dirs);
  char* found = NULL;
  int last = by_name ? MATCH_NAME : MATCH_ID;
  for (int how = MATCH_ID; how <= last && !found; how++) {
    for (size_t i = 0; i < dirs.n && !found; i++) {
      if (how == MATCH_ID) {
        Str s = { 0 };
        str_add(&s, name);
        size_t len = strlen(name);
        if (len <= 8 || strcmp(name + len - 8, ".desktop") != 0)
          str_add(&s, ".desktop");
        char* file = str_take(&s);
        char* path = file ? path_join(dirs.v[i], file) : NULL;
        NQFree(file);
        char* text = path ? read_text(path) : NULL;
        if (text && is_application(text))
          found = path;
        else
          NQFree(path);
        NQFree(text);
        continue;
      }
      DIR* d = opendir(dirs.v[i]);
      if (!d)
        continue;
      struct dirent* e;
      while (!found && (e = readdir(d)) != NULL) {
        if (desktop_matches(dirs.v[i], e->d_name, name, how)) {
          char* path = path_join(dirs.v[i], e->d_name);
          char* text = path ? read_text(path) : NULL;
          if (text && is_application(text))
            found = path;
          else
            NQFree(path);
          NQFree(text);
        }
      }
      closedir(d);
    }
  }
  list_free(&dirs);
  return found;
}

/* Splits an Exec value, its string escapes undone, into arguments by the
 * quoting rules of the Desktop Entry Specification */
static int exec_split(const char* exec, StrList* words)
{
  const char* p = exec;
  for (;;) {
    while (*p == ' ' || *p == '\t')
      p++;
    if (!*p)
      break;
    Str w = { 0 };
    str_addn(&w, "", 0);
    while (*p && *p != ' ' && *p != '\t') {
      if (*p == '"') {
        p++;
        while (*p && *p != '"') {
          if (*p == '\\' && p[1] && NQStrchr("\"`$\\", p[1]))
            p++;
          str_addc(&w, *p++);
        }
        if (*p != '"') {
          NQFree(str_take(&w));
          return -NQ_EINVAL;
        }
        p++;
      } else {
        str_addc(&w, *p++);
      }
    }
    list_take(words, str_take(&w));
  }
  return words->failed ? -NQ_ENOMEM : words->n > 0 ? 0 : -NQ_EINVAL;
}

/* An item as a file argument (%f): local paths as they are */
static const char* item_for_file(const char* item)
{
  return item;
}

/* An item as a URL argument (%u) */
static char* item_for_url(const char* item)
{
  return is_path(item) ? file_uri(item) : str_dup(item);
}

/* Builds the commands that open `items` with the desktop file: one, or
 * one for each item when its Exec takes only one (%f, %u) */
static int desktop_commands(const char* desktop, const char* const* items, size_t count, const NQShellOpenOptions* o,
                            StrList** commands, size_t* ncommands)
{
  *commands = NULL;
  *ncommands = 0;
  char* text = read_text(desktop);
  if (!text)
    return -NQ_ENOENT;
  char* raw = ini_get(text, "Desktop Entry", "Exec");
  char* exec = raw ? unescape_value(raw) : NULL;
  char* rawname = ini_get(text, "Desktop Entry", "Name");
  char* name = rawname ? unescape_value(rawname) : NULL;
  char* icon = ini_get(text, "Desktop Entry", "Icon");
  NQFree(raw);
  NQFree(rawname);
  NQFree(text);

  StrList words = { 0 };
  int ret = exec ? exec_split(exec, &words) : -NQ_ENOENT;
  NQFree(exec);

  /* does it take the items all at once, or one by one? */
  int single = 0;
  for (size_t i = 0; ret == 0 && i < words.n; i++) {
    if (NQStrstr(words.v[i], "%f") || NQStrstr(words.v[i], "%u"))
      single = 1;
    if (NQStrcmp(words.v[i], "%F") == 0 || NQStrcmp(words.v[i], "%U") == 0)
      single = 0, i = words.n;
  }
  size_t n = single && count > 1 ? count : 1;
  StrList* cmds = ret == 0 ? NQZeroMalloc(n * sizeof(*cmds)) : NULL;
  if (ret == 0 && !cmds)
    ret = -NQ_ENOMEM;

  for (size_t c = 0; ret == 0 && c < n; c++) {
    const char* one = count == 0 ? NULL : items[single ? c : 0];
    StrList* cmd = &cmds[c];
    for (size_t i = 0; i < words.n; i++) {
      const char* w = words.v[i];
      if (NQStrcmp(w, "%F") == 0 || NQStrcmp(w, "%U") == 0) {
        for (size_t k = 0; k < count; k++)
          list_take(cmd, w[1] == 'F' ? str_dup(item_for_file(items[k])) : item_for_url(items[k]));
        continue;
      }
      if (NQStrcmp(w, "%i") == 0) {
        if (icon && *icon) {
          list_add(cmd, "--icon");
          list_add(cmd, icon);
        }
        continue;
      }
      Str s = { 0 };
      str_addn(&s, "", 0);
      int dropped = 0;
      for (const char* p = w; *p; p++) {
        if (*p != '%' || !p[1]) {
          str_addc(&s, *p);
          continue;
        }
        p++;
        switch (*p) {
        case '%': str_addc(&s, '%'); break;
        case 'f':
          if (one)
            str_add(&s, item_for_file(one));
          else
            dropped = 1;
          break;
        case 'u':
          if (one) {
            char* u = item_for_url(one);
            if (u)
              str_add(&s, u);
            else
              s.failed = 1;
            NQFree(u);
          } else {
            dropped = 1;
          }
          break;
        case 'c':
          if (name)
            str_add(&s, name);
          break;
        case 'k': str_add(&s, desktop); break;
        default: break; /* deprecated or unknown field codes go */
        }
      }
      char* word = str_take(&s);
      /* a %f or %u alone, with nothing to put there, is no argument */
      if (word && dropped && !*word)
        NQFree(word);
      else
        list_take(cmd, word);
    }
    for (size_t k = 0; k < o->argCount; k++)
      list_add(cmd, o->args[k]);
    if (cmd->failed || cmd->n == 0)
      ret = -NQ_ENOMEM;
  }

  list_free(&words);
  NQFree(name);
  NQFree(icon);
  if (ret != 0 && cmds) {
    for (size_t c = 0; c < n; c++)
      list_free(&cmds[c]);
    NQFree(cmds);
    return ret;
  }
  *commands = cmds;
  *ncommands = n;
  return ret;
}

static int open_with_desktop_file(const char* desktop, const char* const* items, size_t count, const NQShellOpenOptions* o,
                                  NQShellOpenWait** wait)
{
  StrList* cmds;
  size_t n;
  int ret = desktop_commands(desktop, items, count, o, &cmds, &n);
  for (size_t c = 0; c < n; c++) {
    if (ret == 0)
      ret = launch(&cmds[c], o->flags & NQ_SHELL_OPEN_WAIT, wait);
    list_free(&cmds[c]);
  }
  NQFree(cmds);
  return ret;
}

/* The desktop file of the default application for a MIME type, from the
 * mimeapps.list files, then defaults.list and mimeinfo.cache */
static char* default_application(const char* mime)
{
  StrList files = { 0 };
  StrList config = { 0 }, data = { 0 };
  xdg_dirs(&config, "XDG_CONFIG_HOME", ".config", "XDG_CONFIG_DIRS", "/etc/xdg", "mimeapps.list");
  application_dirs(&data);
  for (size_t i = 0; i < config.n; i++)
    list_add(&files, config.v[i]);
  for (size_t i = 0; i < data.n; i++)
    list_take(&files, path_join(data.v[i], "mimeapps.list"));
  for (size_t i = 0; i < data.n; i++)
    list_take(&files, path_join(data.v[i], "defaults.list"));
  size_t caches = files.n;
  for (size_t i = 0; i < data.n; i++)
    list_take(&files, path_join(data.v[i], "mimeinfo.cache"));
  list_free(&config);
  list_free(&data);

  char* found = NULL;
  for (size_t i = 0; i < files.n && !found; i++) {
    char* text = read_text(files.v[i]);
    char* ids = text ? ini_get(text, i < caches ? "Default Applications" : "MIME Cache", mime) : NULL;
    NQFree(text);
    for (char* id = ids; id && *id && !found;) {
      char* end = strchr(id, ';');
      if (end)
        *end = '\0';
      if (*id)
        found = find_desktop_file(id, 0);
      id = end ? end + 1 : NULL;
    }
    NQFree(ids);
  }
  list_free(&files);
  return found;
}

/* The program that opens an item with its default application */
static int opener(StrList* argv)
{
  static const char* const openers[][2] = {
    { "xdg-open", NULL }, { "gio", "open" }, { "kde-open", NULL }, { "kde-open5", NULL }, { "exo-open", NULL },
  };
  for (size_t i = 0; i < sizeof(openers) / sizeof(openers[0]); i++) {
    char* path = find_program(openers[i][0]);
    if (!path)
      continue;
    list_take(argv, path);
    if (openers[i][1])
      list_add(argv, openers[i][1]);
    return argv->failed ? -NQ_ENOMEM : 0;
  }
  return -NQ_ENOSYS; /* no desktop to ask */
}

static int open_default(const char* item, unsigned flags, NQShellOpenWait** wait)
{
  StrList argv = { 0 };
  int ret = opener(&argv);
  if (ret == 0) {
    list_add(&argv, item);
    ret = argv.failed ? -NQ_ENOMEM : 0;
  }
  if (ret == 0) {
    pid_t pid;
    ret = run(argv.v, flags & NQ_SHELL_OPEN_WAIT ? RUN_CHILD : RUN_DETACHED, &pid);
    if (ret == 0 && (flags & NQ_SHELL_OPEN_WAIT))
      ret = wait_add(wait, pid);
  }
  list_free(&argv);
  return ret;
}

/* Asks the file manager to show the files, over D-Bus with gdbus or
 * dbus-send; 0 if it did */
static int show_items(const char* const* items, size_t count)
{
  StrList uris = { 0 };
  for (size_t i = 0; i < count; i++)
    list_take(&uris, file_uri(items[i]));
  if (uris.failed) {
    list_free(&uris);
    return -NQ_ENOMEM;
  }
  int ret = -NQ_ENOTSUPP;

  char* gdbus = find_program("gdbus");
  if (gdbus) {
    /* the URIs have nothing that needs quoting in GVariant text */
    Str a = { 0 };
    str_addc(&a, '[');
    for (size_t i = 0; i < uris.n; i++) {
      if (i)
        str_addc(&a, ',');
      str_addc(&a, '\'');
      str_add(&a, uris.v[i]);
      str_addc(&a, '\'');
    }
    str_addc(&a, ']');
    StrList argv = { 0 };
    list_take(&argv, gdbus);
    const char* const fixed[] = { "call", "--session", "--timeout", "10", "--dest", "org.freedesktop.FileManager1",
                                  "--object-path", "/org/freedesktop/FileManager1", "--method",
                                  "org.freedesktop.FileManager1.ShowItems" };
    for (size_t i = 0; i < sizeof(fixed) / sizeof(fixed[0]); i++)
      list_add(&argv, fixed[i]);
    list_take(&argv, str_take(&a));
    list_add(&argv, "");
    if (!argv.failed && run(argv.v, RUN_SYNC, NULL) == 0)
      ret = 0;
    list_free(&argv);
  }

  char* dbus_send = ret != 0 ? find_program("dbus-send") : NULL;
  if (dbus_send) {
    /* nor a comma, which would part them */
    Str a = { 0 };
    str_add(&a, "array:string:");
    for (size_t i = 0; i < uris.n; i++) {
      if (i)
        str_addc(&a, ',');
      str_add(&a, uris.v[i]);
    }
    StrList argv = { 0 };
    list_take(&argv, dbus_send);
    const char* const fixed[] = {
      "--session",
      "--print-reply",
      "--reply-timeout=10000",
      "--dest=org.freedesktop.FileManager1",
      "--type=method_call",
      "/org/freedesktop/FileManager1",
      "org.freedesktop.FileManager1.ShowItems",
    };
    for (size_t i = 0; i < sizeof(fixed) / sizeof(fixed[0]); i++)
      list_add(&argv, fixed[i]);
    list_take(&argv, str_take(&a));
    list_add(&argv, "string:");
    if (!argv.failed && run(argv.v, RUN_SYNC, NULL) == 0)
      ret = 0;
    list_free(&argv);
  }
  list_free(&uris);
  return ret;
}

/* Shows the files in the file manager; where it cannot select them, opens
 * the folders they are in */
static int reveal(const char* const* items, size_t count)
{
  for (size_t i = 0; i < count; i++) {
    if (!is_path(items[i]))
      return -NQ_ENOTSUPP;
  }
  if (count == 0 || show_items(items, count) == 0)
    return 0;
  int ret = 0;
  StrList opened = { 0 };
  for (size_t i = 0; i < count && ret == 0; i++) {
    Str s = { 0 };
    str_add(&s, items[i]);
    char* dir = str_take(&s);
    if (!dir)
      return -NQ_ENOMEM;
    size_t len = NQStrlen(dir);
    while (len > 1 && dir[len - 1] == '/')
      dir[--len] = '\0';
    char* slash = NQStrrchr(dir, '/');
    if (slash == dir)
      dir[1] = '\0';
    else if (slash)
      *slash = '\0';
    int seen = 0;
    for (size_t k = 0; k < opened.n; k++)
      seen |= NQStrcmp(opened.v[k], dir) == 0;
    if (!seen) {
      ret = open_default(dir, 0, NULL);
      list_take(&opened, dir);
    } else {
      NQFree(dir);
    }
  }
  list_free(&opened);
  return ret;
}

static int open_xdg(const char* const* items, size_t count, const NQShellOpenOptions* o, NQShellOpenWait** wait)
{
  if (o->flags & NQ_SHELL_OPEN_REVEAL)
    return reveal(items, count);

  char* desktop = NULL;
  if (o->bundleId) {
    desktop = find_desktop_file(o->bundleId, 0);
    if (!desktop)
      return -NQ_ENOENT;
  } else if (o->application) {
    const char* app = o->application;
    size_t len = NQStrlen(app);
    int is_desktop = len > 8 && strcmp(app + len - 8, ".desktop") == 0;
    if (NQStrchr(app, '/') && is_desktop) {
      if (!is_regular(app))
        return -NQ_ENOENT;
      desktop = str_dup(app);
    } else {
      /* a program, then a desktop file by its ID or name */
      char* program = is_desktop ? NULL : find_program(app);
      if (!program && !NQStrchr(app, '/')) {
        /* "Firefox" is run as firefox */
        char* lower = str_dup(app);
        for (char* p = lower; p && *p; p++)
          if (*p >= 'A' && *p <= 'Z')
            *p = (char)(*p - 'A' + 'a');
        if (lower && NQStrcmp(lower, app) != 0 && !is_desktop)
          program = find_program(lower);
        NQFree(lower);
        if (!program)
          desktop = find_desktop_file(app, 1);
      }
      if (program) {
        StrList argv = { 0 };
        list_take(&argv, program);
        for (size_t i = 0; i < count; i++)
          list_add(&argv, items[i]);
        for (size_t i = 0; i < o->argCount; i++)
          list_add(&argv, o->args[i]);
        int ret = argv.failed ? -NQ_ENOMEM : launch(&argv, o->flags & NQ_SHELL_OPEN_WAIT, wait);
        list_free(&argv);
        return ret;
      }
      if (!desktop)
        return -NQ_ENOENT;
    }
  } else if (o->flags & NQ_SHELL_OPEN_TEXT_EDITOR) {
    desktop = default_application("text/plain");
  }

  if (desktop) {
    int ret = open_with_desktop_file(desktop, items, count, o, wait);
    NQFree(desktop);
    return ret;
  }
  if (count == 0)
    return -NQ_EINVAL;
  for (size_t i = 0; i < count; i++) {
    int ret = open_default(items[i], o->flags, wait);
    if (ret != 0)
      return ret;
  }
  return 0;
}

#endif /* OPEN_XDG */

/* ========================================================================== */
/* macOS: Launch Services and NSWorkspace                                     */
/* ========================================================================== */

#if defined(OPEN_MACOS)

# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-declarations"

/* NSWorkspaceLaunchOptions */
# define WS_LAUNCH_AND_HIDE         0x00100000
# define WS_LAUNCH_WITHOUT_ACTIVATE 0x00000200
# define WS_LAUNCH_NEW_INSTANCE     0x00080000

extern void* objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void* pool);

/* AppKit, loaded when it is needed: a program without a window has not */
static int load_appkit(void)
{
  static int loaded;
  if (!loaded)
    loaded = dlopen("/System/Library/Frameworks/AppKit.framework/AppKit", RTLD_LAZY | RTLD_GLOBAL) ? 1 : -1;
  return loaded > 0;
}

static id shared_workspace(void)
{
  if (!load_appkit())
    return NULL;
  Class c = objc_getClass("NSWorkspace");
  if (!c)
    return NULL;
  return ((id (*)(Class, SEL))objc_msgSend)(c, sel_registerName("sharedWorkspace"));
}

static CFURLRef path_url(const char* path, Boolean dir)
{
  return CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8*)path, (CFIndex)strlen(path), dir);
}

static CFURLRef item_url(const char* item)
{
  if (is_path(item))
    return path_url(item, is_directory(item));
  return CFURLCreateWithBytes(NULL, (const UInt8*)item, (CFIndex)strlen(item), kCFStringEncodingUTF8, NULL);
}

/* The items as an array of CFURLs */
static int item_urls(const char* const* items, size_t count, CFMutableArrayRef* out)
{
  CFMutableArrayRef urls = CFArrayCreateMutable(NULL, (CFIndex)count, &kCFTypeArrayCallBacks);
  if (!urls)
    return -NQ_ENOMEM;
  for (size_t i = 0; i < count; i++) {
    CFURLRef url = item_url(items[i]);
    if (!url) {
      CFRelease(urls);
      return -NQ_EINVAL;
    }
    CFArrayAppendValue(urls, url);
    CFRelease(url);
  }
  *out = urls;
  return 0;
}

/* An application by its bundle identifier */
static CFURLRef app_for_bundle_id(CFStringRef bundle)
{
  CFArrayRef apps = LSCopyApplicationURLsForBundleIdentifier(bundle, NULL);
  CFURLRef url = NULL;
  if (apps && CFArrayGetCount(apps) > 0)
    url = CFRetain(CFArrayGetValueAtIndex(apps, 0));
  if (apps)
    CFRelease(apps);
  return url;
}

static CFURLRef app_for_bundle_id_c(const char* bundle)
{
  CFStringRef s = CFStringCreateWithCString(NULL, bundle, kCFStringEncodingUTF8);
  if (!s)
    return NULL;
  CFURLRef url = app_for_bundle_id(s);
  CFRelease(s);
  return url;
}

/* An application by its name ("Safari", "safari.app") in the folders
 * applications are kept in, or by its path */
static CFURLRef app_for_name(const char* name)
{
  if (NQStrchr(name, '/')) {
    if (!is_directory(name) && !is_regular(name))
      return NULL;
    if (name[0] == '/')
      return path_url(name, is_directory(name));
    char cwd[4096];
    if (!getcwd(cwd, sizeof(cwd)))
      return NULL;
    char* path = path_join(cwd, name);
    CFURLRef url = path ? path_url(path, is_directory(path)) : NULL;
    NQFree(path);
    return url;
  }
  Str s = { 0 };
  str_add(&s, name);
  size_t len = NQStrlen(name);
  if (len <= 4 || NQStrcasecmp(name + len - 4, ".app") != 0)
    str_add(&s, ".app");
  char* bundle = str_take(&s);
  if (!bundle)
    return NULL;

  StrList dirs = { 0 };
  const char* home = NQEnvCreate("HOME");
  if (home && *home)
    list_take(&dirs, path_join(home, "Applications"));
  NQEnvFree(home);
  static const char* const system_dirs[] = {
    "/Applications",
    "/Applications/Utilities",
    "/System/Applications",
    "/System/Applications/Utilities",
    "/System/Library/CoreServices",
    "/System/Library/CoreServices/Applications",
    "/Developer/Applications",
  };
  for (size_t i = 0; i < sizeof(system_dirs) / sizeof(system_dirs[0]); i++)
    list_add(&dirs, system_dirs[i]);

  CFURLRef url = NULL;
  for (size_t i = 0; i < dirs.n && !url; i++) {
    DIR* d = opendir(dirs.v[i]);
    if (!d)
      continue;
    struct dirent* e;
    while (!url && (e = readdir(d)) != NULL) {
      if (NQStrcasecmp(e->d_name, bundle) != 0)
        continue;
      char* path = path_join(dirs.v[i], e->d_name);
      if (path && is_directory(path))
        url = path_url(path, true);
      NQFree(path);
    }
    closedir(d);
  }
  list_free(&dirs);
  NQFree(bundle);
  return url;
}

static CFURLRef text_editor(void)
{
  CFURLRef url = NULL;
  CFStringRef bundle = LSCopyDefaultRoleHandlerForContentType(CFSTR("public.plain-text"), kLSRolesEditor);
  if (bundle) {
    url = app_for_bundle_id(bundle);
    CFRelease(bundle);
  }
  return url ? url : app_for_bundle_id(CFSTR("com.apple.TextEdit"));
}

static int reveal(const char* const* items, size_t count)
{
  for (size_t i = 0; i < count; i++) {
    if (!is_path(items[i]))
      return -NQ_ENOTSUPP;
  }
  id ws = shared_workspace();
  if (!ws)
    return -NQ_ENOTSUPP;
  CFMutableArrayRef urls;
  int ret = item_urls(items, count, &urls);
  if (ret != 0)
    return ret;
  void* pool = objc_autoreleasePoolPush();
  ((void (*)(id, SEL, id))objc_msgSend)(ws, sel_registerName("activateFileViewerSelectingURLs:"), (id)urls);
  objc_autoreleasePoolPop(pool);
  CFRelease(urls);
  return 0;
}

/* Opens the URLs with the application by NSWorkspace, which can give it
 * arguments and says what process it is */
static int open_with_workspace(CFArrayRef urls, CFURLRef app, const NQShellOpenOptions* o, NQShellOpenWait** wait)
{
  id ws = shared_workspace();
  if (!ws)
    return -NQ_ENOTSUPP;
  CFStringRef* args_key = dlsym(RTLD_DEFAULT, "NSWorkspaceLaunchConfigurationArguments");
  unsigned long options = 0;
  if (o->flags & NQ_SHELL_OPEN_BACKGROUND)
    options |= WS_LAUNCH_WITHOUT_ACTIVATE;
  if (o->flags & NQ_SHELL_OPEN_HIDE)
    options |= WS_LAUNCH_AND_HIDE;
  if (o->flags & NQ_SHELL_OPEN_NEW_INSTANCE)
    options |= WS_LAUNCH_NEW_INSTANCE;

  CFMutableArrayRef args = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
  CFMutableDictionaryRef config =
    CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  if (!args || !config) {
    if (args)
      CFRelease(args);
    if (config)
      CFRelease(config);
    return -NQ_ENOMEM;
  }
  if (o->flags & NQ_SHELL_OPEN_FRESH) {
    /* what keeps AppKit from restoring the windows */
    CFArrayAppendValue(args, CFSTR("-ApplePersistenceIgnoreState"));
    CFArrayAppendValue(args, CFSTR("YES"));
  }
  for (size_t i = 0; i < o->argCount; i++) {
    CFStringRef s = CFStringCreateWithCString(NULL, o->args[i], kCFStringEncodingUTF8);
    if (s) {
      CFArrayAppendValue(args, s);
      CFRelease(s);
    }
  }
  if (args_key && *args_key && CFArrayGetCount(args) > 0)
    CFDictionarySetValue(config, *args_key, args);

  void* pool = objc_autoreleasePoolPush();
  id error = NULL;
  id running;
  if (urls && CFArrayGetCount(urls) > 0)
    running = ((id (*)(id, SEL, id, id, unsigned long, id, id*))objc_msgSend)(
      ws, sel_registerName("openURLs:withApplicationAtURL:options:configuration:error:"), (id)urls, (id)app, options,
      (id)config, &error);
  else
    running = ((id (*)(id, SEL, id, unsigned long, id, id*))objc_msgSend)(
      ws, sel_registerName("launchApplicationAtURL:options:configuration:error:"), (id)app, options, (id)config,
      &error);
  int ret = running ? 0 : -NQ_ENOTSUPP;
  if (running && (o->flags & NQ_SHELL_OPEN_WAIT)) {
    pid_t pid = ((pid_t (*)(id, SEL))objc_msgSend)(running, sel_registerName("processIdentifier"));
    if (pid > 0)
      ret = wait_add(wait, pid);
  }
  objc_autoreleasePoolPop(pool);
  CFRelease(args);
  CFRelease(config);
  return ret;
}

static int open_macos(const char* const* items, size_t count, const NQShellOpenOptions* o, NQShellOpenWait** wait)
{
  if (o->flags & NQ_SHELL_OPEN_REVEAL)
    return reveal(items, count);

  CFURLRef app = NULL;
  if (o->bundleId) {
    app = app_for_bundle_id_c(o->bundleId);
    if (!app)
      return -NQ_ENOENT;
  } else if (o->application) {
    app = app_for_name(o->application);
    if (!app)
      return -NQ_ENOENT;
  } else if (o->flags & NQ_SHELL_OPEN_TEXT_EDITOR) {
    app = text_editor();
    if (!app)
      return -NQ_ENOTSUPP;
  }
  if (!app && count == 0)
    return -NQ_EINVAL;

  CFMutableArrayRef urls;
  int ret = item_urls(items, count, &urls);
  if (ret != 0) {
    if (app)
      CFRelease(app);
    return ret;
  }

  int special = (o->flags & (NQ_SHELL_OPEN_WAIT | NQ_SHELL_OPEN_FRESH)) || o->argCount > 0;
  if (app) {
    ret = open_with_workspace(urls, app, o, wait);
  } else if (special) {
    /* each with its own application, which NSWorkspace is told */
    for (CFIndex i = 0; ret == 0 && i < CFArrayGetCount(urls); i++) {
      CFURLRef url = CFArrayGetValueAtIndex(urls, i);
      CFURLRef handler = LSCopyDefaultApplicationURLForURL(url, kLSRolesAll, NULL);
      if (!handler) {
        ret = -NQ_ENOTSUPP;
        break;
      }
      CFArrayRef one = CFArrayCreate(NULL, (const void**)&url, 1, &kCFTypeArrayCallBacks);
      ret = one ? open_with_workspace(one, handler, o, wait) : -NQ_ENOMEM;
      if (one)
        CFRelease(one);
      CFRelease(handler);
    }
  } else {
    LSLaunchURLSpec spec;
    memset(&spec, 0, sizeof(spec));
    spec.itemURLs = urls;
    spec.launchFlags = kLSLaunchDefaults;
    if (o->flags & NQ_SHELL_OPEN_BACKGROUND)
      spec.launchFlags |= kLSLaunchDontSwitch;
    if (o->flags & NQ_SHELL_OPEN_HIDE)
      spec.launchFlags |= kLSLaunchAndHide;
    if (o->flags & NQ_SHELL_OPEN_NEW_INSTANCE)
      spec.launchFlags |= kLSLaunchNewInstance;
    OSStatus st = LSOpenFromURLSpec(&spec, NULL);
    if (st == kLSApplicationNotFoundErr)
      ret = -NQ_ENOTSUPP;
    else if (st != noErr)
      ret = -NQ_EIO;
  }
  CFRelease(urls);
  if (app)
    CFRelease(app);
  return ret;
}

# pragma clang diagnostic pop

#endif /* OPEN_MACOS */

/* ========================================================================== */
/* Windows: the shell                                                         */
/* ========================================================================== */

#if defined(OPEN_WINDOWS)

static wchar_t* to_wide(const char* in)
{
  int n = MultiByteToWideChar(CP_UTF8, 0, in, -1, NULL, 0);
  wchar_t* out = n > 0 ? NQMalloc((size_t)n * sizeof(wchar_t)) : NULL;
  if (out)
    MultiByteToWideChar(CP_UTF8, 0, in, -1, out, n);
  return out;
}

/* "C:\x", "\\server\share", not "https:" (a scheme of one letter would be
 * a drive) */
static int win_is_url(const char* item)
{
  const char* p = item;
  if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')))
    return 0;
  while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '+' || *p == '-'
         || *p == '.')
    p++;
  return *p == ':' && p - item >= 2;
}

/* A path with backslashes */
static wchar_t* win_path(const char* item)
{
  wchar_t* w = to_wide(item);
  if (w && !win_is_url(item)) {
    for (wchar_t* p = w; *p; p++)
      if (*p == L'/')
        *p = L'\\';
  }
  return w;
}

typedef struct WStr {
  wchar_t* p;
  size_t len;
  size_t cap;
  int failed;
} WStr;

static void wstr_addc(WStr* s, wchar_t c)
{
  if (s->failed)
    return;
  if (s->len + 2 > s->cap) {
    size_t cap = s->cap ? s->cap * 2 : 128;
    wchar_t* p = NQRealloc(s->p, cap * sizeof(wchar_t));
    if (!p) {
      s->failed = 1;
      return;
    }
    s->p = p;
    s->cap = cap;
  }
  s->p[s->len++] = c;
  s->p[s->len] = 0;
}

static void wstr_add(WStr* s, const wchar_t* t)
{
  while (*t)
    wstr_addc(s, *t++);
}

/* Adds an argument quoted as CommandLineToArgvW takes it apart */
static void wstr_add_arg(WStr* s, const wchar_t* arg)
{
  if (s->len > 0)
    wstr_addc(s, L' ');
  if (*arg && !wcspbrk(arg, L" \t\n\v\"")) {
    wstr_add(s, arg);
    return;
  }
  wstr_addc(s, L'"');
  for (const wchar_t* p = arg;; p++) {
    size_t slashes = 0;
    while (*p == L'\\') {
      p++;
      slashes++;
    }
    if (!*p) {
      for (size_t i = 0; i < slashes * 2; i++)
        wstr_addc(s, L'\\');
      break;
    }
    if (*p == L'"') {
      for (size_t i = 0; i < slashes * 2 + 1; i++)
        wstr_addc(s, L'\\');
    } else {
      for (size_t i = 0; i < slashes; i++)
        wstr_addc(s, L'\\');
    }
    wstr_addc(s, *p);
  }
  wstr_addc(s, L'"');
}

static int show_command(unsigned flags)
{
  if (flags & NQ_SHELL_OPEN_HIDE)
    return SW_SHOWMINNOACTIVE;
  if (flags & NQ_SHELL_OPEN_BACKGROUND)
    return SW_SHOWNOACTIVATE;
  return SW_SHOWNORMAL;
}

static int shell_execute(const wchar_t* file, const wchar_t* params, unsigned flags, int app_given,
                         NQShellOpenWait** wait)
{
  SHELLEXECUTEINFOW sei;
  memset(&sei, 0, sizeof(sei));
  sei.cbSize = sizeof(sei);
  sei.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC | SEE_MASK_UNICODE;
  if (flags & NQ_SHELL_OPEN_WAIT)
    sei.fMask |= SEE_MASK_NOCLOSEPROCESS;
  sei.lpFile = file;
  sei.lpParameters = params;
  sei.nShow = show_command(flags);
  if (!ShellExecuteExW(&sei)) {
    DWORD ec = GetLastError();
    if (ec == ERROR_NO_ASSOCIATION)
      return -NQ_ENOTSUPP;
    if (app_given && (ec == ERROR_FILE_NOT_FOUND || ec == ERROR_PATH_NOT_FOUND))
      return -NQ_ENOENT;
    return -(int)ec;
  }
  if (sei.hProcess) {
    int ret = wait_add(wait, (int64_t)(intptr_t)sei.hProcess);
    if (ret != 0)
      CloseHandle(sei.hProcess);
    return ret;
  }
  return 0;
}

static int reveal(const char* const* items, size_t count)
{
  for (size_t i = 0; i < count; i++) {
    if (win_is_url(items[i]))
      return -NQ_ENOTSUPP;
  }
  for (size_t i = 0; i < count; i++) {
    wchar_t* path = win_path(items[i]);
    if (!path)
      return -NQ_ENOMEM;
# if defined(WITH_OLE32)
    /* the shell's own way, which needs COM (NQShellOpen sets it up) */
    PIDLIST_ABSOLUTE pidl = NULL;
    HRESULT hr = SHParseDisplayName(path, NULL, &pidl, 0, NULL);
    if (SUCCEEDED(hr)) {
      hr = SHOpenFolderAndSelectItems(pidl, 0, NULL, 0);
      CoTaskMemFree(pidl);
    }
# else
    HRESULT hr = E_NOTIMPL;
# endif
    if (FAILED(hr)) {
      /* Explorer can be asked on its command line */
      WStr s = { 0 };
      wstr_add(&s, L"/select,");
      WStr q = { 0 };
      wstr_add_arg(&q, path);
      if (q.p)
        wstr_add(&s, q.p);
      NQFree(q.p);
      int ret = s.failed || q.failed ? -NQ_ENOMEM : shell_execute(L"explorer.exe", s.p, 0, 1, NULL);
      NQFree(s.p);
      if (ret != 0) {
        NQFree(path);
        return ret;
      }
    }
    NQFree(path);
  }
  return 0;
}

/* The program .txt files are edited with; Notepad without Shlwapi */
static wchar_t* text_editor(void)
{
  const wchar_t* exe = L"notepad.exe";
# if defined(WITH_SHLWAPI)
  wchar_t buf[MAX_PATH * 2];
  DWORD len = (DWORD)(sizeof(buf) / sizeof(buf[0]));
  HRESULT hr = AssocQueryStringW(ASSOCF_NOTRUNCATE, ASSOCSTR_EXECUTABLE, L".txt", L"open", buf, &len);
  if (SUCCEEDED(hr) && buf[0])
    exe = buf;
# endif
  size_t n = wcslen(exe) + 1;
  wchar_t* out = NQMalloc(n * sizeof(wchar_t));
  if (out)
    NQMemcpy(out, exe, n * sizeof(wchar_t));
  return out;
}

static int open_windows(const char* const* items, size_t count, const NQShellOpenOptions* o, NQShellOpenWait** wait)
{
  if (o->flags & NQ_SHELL_OPEN_REVEAL)
    return reveal(items, count);

  wchar_t* app = NULL;
  if (o->bundleId) {
    /* a packaged application, by its AppUserModelID; it takes no files */
    if (count > 0 || o->argCount > 0)
      return -NQ_ENOTSUPP;
    WStr s = { 0 };
    wstr_add(&s, L"shell:AppsFolder\\");
    wchar_t* id = to_wide(o->bundleId);
    if (id)
      wstr_add(&s, id);
    NQFree(id);
    if (s.failed || !id) {
      NQFree(s.p);
      return -NQ_ENOMEM;
    }
    int ret = shell_execute(s.p, NULL, o->flags, 1, wait);
    NQFree(s.p);
    return ret;
  }
  if (o->application)
    app = win_path(o->application);
  else if (o->flags & NQ_SHELL_OPEN_TEXT_EDITOR)
    app = text_editor();
  if ((o->application || (o->flags & NQ_SHELL_OPEN_TEXT_EDITOR)) && !app)
    return -NQ_ENOMEM;

  int ret = 0;
  if (app) {
    WStr params = { 0 };
    for (size_t i = 0; i < count; i++) {
      wchar_t* w = win_path(items[i]);
      if (w)
        wstr_add_arg(&params, w);
      else
        params.failed = 1;
      NQFree(w);
    }
    for (size_t i = 0; i < o->argCount; i++) {
      wchar_t* w = to_wide(o->args[i]);
      if (w)
        wstr_add_arg(&params, w);
      else
        params.failed = 1;
      NQFree(w);
    }
    ret = params.failed ? -NQ_ENOMEM : shell_execute(app, params.p, o->flags, 1, wait);
    NQFree(params.p);
    NQFree(app);
    return ret;
  }
  if (count == 0)
    return -NQ_EINVAL;
  for (size_t i = 0; i < count && ret == 0; i++) {
    wchar_t* w = win_path(items[i]);
    ret = w ? shell_execute(w, NULL, o->flags, 0, wait) : -NQ_ENOMEM;
    NQFree(w);
  }
  return ret;
}

#endif /* OPEN_WINDOWS */

/* --- the interface ------------------------------------------------------------ */

int NQShellOpen(const char* const* items, size_t count, const NQShellOpenOptions* options, NQShellOpenWait** wait)
{
  static const NQShellOpenOptions defaults = { 0 };
  const NQShellOpenOptions* o = options ? options : &defaults;
  if (wait)
    *wait = NULL;
  if ((o->flags & NQ_SHELL_OPEN_WAIT) && !wait)
    return -NQ_EINVAL;
  if (count > 0 && !items)
    return -NQ_EINVAL;
  for (size_t i = 0; i < count; i++) {
    if (!items[i] || !*items[i])
      return -NQ_EINVAL;
  }

  NQShellOpenWait* waiting = NULL;
  int ret;
#if defined(OPEN_WINDOWS)
# if defined(WITH_OLE32)
  /* the shell's handlers may be COM objects */
  HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
# endif
  ret = open_windows(items, count, o, &waiting);
# if defined(WITH_OLE32)
  if (SUCCEEDED(hr))
    CoUninitialize();
# endif
#elif defined(OPEN_MACOS)
  ret = open_macos(items, count, o, &waiting);
#elif defined(OPEN_XDG)
  ret = open_xdg(items, count, o, &waiting);
#else
  ret = -NQ_ENOSYS;
#endif
  if (wait)
    *wait = waiting;
  else
    NQShellOpenWait_free(waiting);
  return ret;
}

int NQShellOpenItem(const char* item)
{
  return NQShellOpen(&item, 1, NULL, NULL);
}

int NQShellOpenWait_poll(NQShellOpenWait* wait)
{
  if (!wait)
    return 0;
  size_t kept = 0;
  for (size_t i = 0; i < wait->count; i++) {
    int64_t id = wait->ids[i];
    int running;
#if defined(OPEN_WINDOWS)
    HANDLE h = (HANDLE)(intptr_t)id;
    running = WaitForSingleObject(h, 0) == WAIT_TIMEOUT;
    if (!running)
      CloseHandle(h);
#elif defined(OPEN_XDG)
    int status;
    pid_t r = waitpid((pid_t)id, &status, WNOHANG);
    running = r == 0 || (r < 0 && errno == EINTR);
#elif defined(OPEN_MACOS)
    /* not our child: there while it can be signalled */
    running = kill((pid_t)id, 0) == 0 || errno == EPERM;
#else
    running = 0;
#endif
    if (running)
      wait->ids[kept++] = id;
  }
  wait->count = kept;
  return kept > 0;
}

void NQShellOpenWait_free(NQShellOpenWait* wait)
{
  if (!wait)
    return;
#if defined(OPEN_WINDOWS)
  for (size_t i = 0; i < wait->count; i++)
    CloseHandle((HANDLE)(intptr_t)wait->ids[i]);
#endif
  NQFree(wait->ids);
  NQFree(wait);
}
