/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/gfx/FileDialog.h"

#include <libnetq/ErrorCode.h>
#include <libnetq/Malloc.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/string/String.h>

#if defined(NQ_OS_WINDOWS) && defined(WITH_OLE32) && defined(WITH_SHELL32)
# define DIALOG_WINDOWS 1
#elif defined(NQ_OS_MACOSX) && defined(WITH_CORESERVICES)
# define DIALOG_MACOS 1
#elif defined(NQ_OS_UNIX) && !defined(NQ_OS_ANDROID) && !defined(NQ_OS_DARWIN) && !defined(__EMSCRIPTEN__)
# define DIALOG_XDG 1
#endif

#if defined(DIALOG_WINDOWS)
# ifndef COBJMACROS
#  define COBJMACROS
# endif
# include <libnetq/UTF.h>
# include <libnetq/string/StringData.h>
# include <windows.h>
# include <objbase.h>
# include <shobjidl.h>
#elif defined(DIALOG_MACOS)
# include <libnetq/backend/cf/AutoRelease.h>
# include <libnetq/Library.h>
# include <CoreFoundation/CoreFoundation.h>
# include <objc/message.h>
# include <objc/runtime.h>
# include <pthread.h>
# include <sys/stat.h>
#elif defined(DIALOG_XDG)
# include <libnetq/Env.h>
# include <libnetq/fs/Path.h>
# include <libnetq/Thread.h>
# include <errno.h>
# include <fcntl.h>
# include <signal.h>
# include <spawn.h>
# include <stddef.h>
# include <stdio.h>
# include <sys/socket.h>
# include <sys/stat.h>
# include <sys/un.h>
# include <sys/wait.h>
# include <unistd.h>
#endif

static const NQFileDialogOptions s_defaultOptions;

#if defined(DIALOG_WINDOWS)

/* The GUIDs, here so that libuuid is not needed */
static const CLSID s_clsidFileOpenDialog =
  {0xDC1C5A9C, 0xE88A, 0x4DDE, {0xA5, 0xA1, 0x60, 0xF8, 0x2A, 0x20, 0xAE, 0xF7}};
static const IID s_iidFileOpenDialog =
  {0xD57C7288, 0xD4AD, 0x4768, {0xBE, 0x02, 0x9D, 0x96, 0x95, 0x32, 0xD9, 0x60}};
static const IID s_iidShellItem =
  {0x43826D1E, 0xE718, 0x42EE, {0xBC, 0x55, 0xA1, 0xE2, 0x61, 0xC3, 0x7B, 0xFE}};

/* UTF-8 to an NQMalloc'd UTF-16 string, with '/' turned into '\\' */
static wchar_t* to_wide(const char* s, int path)
{
  NQUnicodeInfo info;
  if (!NQConvertUTF8ToUTF16((const uint8_t*)s, NULL, NULL, NULL, &info))
    return NULL;
  wchar_t* w = NQMalloc((info.utf16Size + 1) * sizeof(wchar_t));
  if (!w)
    return NULL;
  NQConvertUTF8ToUTF16((const uint8_t*)s, (const uint8_t*)s + info.utf8Size, (uint16_t*)w, (uint16_t*)w + info.utf16Size + 1, NULL);
  if (path) {
    for (wchar_t* p = w; *p; p++) {
      if (*p == L'/')
        *p = L'\\';
    }
  }
  return w;
}

static IShellItem* shell_item(const wchar_t* path)
{
  IShellItem* item = NULL;
  if (FAILED(SHCreateItemFromParsingName(path, NULL, &s_iidShellItem, (void**)&item)))
    return NULL;
  return item;
}

/* Starts the dialog in the folder of `initial`, or in it if it is one,
 * with a file's name filled in */
static void set_initial(IFileOpenDialog* dlg, const char* initial)
{
  wchar_t* path = to_wide(initial, 1);
  if (!path)
    return;
  DWORD attrs = GetFileAttributesW(path);
  if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
    IShellItem* folder = shell_item(path);
    if (folder) {
      IFileOpenDialog_SetFolder(dlg, folder);
      IShellItem_Release(folder);
    }
  }
  else {
    wchar_t* slash = wcsrchr(path, L'\\');
    if (slash) {
      *slash = L'\0';
      IShellItem* folder = shell_item(path);
      if (folder) {
        IFileOpenDialog_SetFolder(dlg, folder);
        IShellItem_Release(folder);
      }
      if (attrs != INVALID_FILE_ATTRIBUTES)
        IFileOpenDialog_SetFileName(dlg, slash + 1);
    }
  }
  NQFree(path);
}

static int dialog_open(const NQFileDialogOptions* options, NQFileDialogCallback callback, void* userdata)
{
  HRESULT init = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  IFileOpenDialog* dlg = NULL;
  HRESULT hr = CoCreateInstance(&s_clsidFileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &s_iidFileOpenDialog, (void**)&dlg);
  if (FAILED(hr)) {
    if (SUCCEEDED(init))
      CoUninitialize();
    return hr == E_OUTOFMEMORY ? -NQ_ENOMEM : -NQ_ENOSYS;
  }

  FILEOPENDIALOGOPTIONS fos = 0;
  IFileOpenDialog_GetOptions(dlg, &fos);
  fos |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR;
  fos &= ~(FILEOPENDIALOGOPTIONS)FOS_ALLOWMULTISELECT;
  if (options->flags & NQ_FILE_DIALOG_DIRECTORY)
    fos |= FOS_PICKFOLDERS;
  else
    fos |= FOS_FILEMUSTEXIST;
  IFileOpenDialog_SetOptions(dlg, fos);

  if (options->title) {
    wchar_t* title = to_wide(options->title, 0);
    if (title) {
      IFileOpenDialog_SetTitle(dlg, title);
      NQFree(title);
    }
  }
  if (options->initialPath && options->initialPath[0])
    set_initial(dlg, options->initialPath);

  NQStringData chosen;
  NQStringData_init(&chosen);

  int error = 0;
  hr = IFileOpenDialog_Show(dlg, (HWND)options->parent);
  if (SUCCEEDED(hr)) {
    IShellItem* item = NULL;
    hr = IFileOpenDialog_GetResult(dlg, &item);
    if (SUCCEEDED(hr)) {
      LPWSTR path = NULL;
      hr = IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &path);
      if (SUCCEEDED(hr)) {
        if (!NQStringData_setw(&chosen, path))
          error = -NQ_ENOMEM;
        CoTaskMemFree(path);
      }
      IShellItem_Release(item);
    }
    if (FAILED(hr))
      error = -NQ_EIO;
  }
  else if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
    error = hr == E_OUTOFMEMORY ? -NQ_ENOMEM : -NQ_EIO;
  }

  IFileOpenDialog_Release(dlg);
  if (SUCCEEDED(init))
    CoUninitialize();

  callback(userdata, NQStringData_characters(&chosen), error);
  NQStringData_finalize(&chosen);

  return 0;
}

#elif defined(DIALOG_MACOS)

# define MODAL_RESPONSE_OK 1

static int load_appkit(void)
{
  static int loaded;
  if (!loaded)
    loaded = NQLibraryOpen("/System/Library/Frameworks/AppKit.framework/AppKit") ? 1 : -1;
  return loaded > 0;
}

static void send_bool(id obj, const char* sel, BOOL value)
{
  ((void (*)(id, SEL, BOOL))objc_msgSend)(obj, sel_registerName(sel), value);
}

static void send_obj(id obj, const char* sel, const void* value)
{
  ((void (*)(id, SEL, id))objc_msgSend)(obj, sel_registerName(sel), (id)value);
}

static void send_string(id obj, const char* sel, const char* utf8)
{
  CFStringRef s = CFStringCreateWithCString(NULL, utf8, kCFStringEncodingUTF8);
  if (!s)
    return;
  send_obj(obj, sel, s); /* CFString is toll-free bridged to NSString */
  CFRelease(s);
}

/* Starts the panel in `initial`, or in its folder when it is a file */
static void set_initial(id panel, const char* initial)
{
  struct stat st;
  int dir = stat(initial, &st) == 0 && S_ISDIR(st.st_mode);
  CFURLRef url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8*)initial, (CFIndex)strlen(initial), dir);
  if (!url)
    return;
  if (dir) {
    send_obj(panel, "setDirectoryURL:", url); /* CFURL is toll-free bridged to NSURL */
  }
  else {
    CFURLRef parent = CFURLCreateCopyDeletingLastPathComponent(NULL, url);
    if (parent) {
      send_obj(panel, "setDirectoryURL:", parent);
      CFRelease(parent);
    }
  }
  CFRelease(url);
}

static int dialog_open(const NQFileDialogOptions* options, NQFileDialogCallback callback, void* userdata)
{
  if (!pthread_main_np())
    return -NQ_EINVAL;
  if (!load_appkit())
    return -NQ_ENOSYS;
  Class cls = objc_getClass("NSOpenPanel");
  if (!cls)
    return -NQ_ENOSYS;

  void* pool = objc_autoreleasePoolPush();
  id panel = ((id (*)(Class, SEL))objc_msgSend)(cls, sel_registerName("openPanel"));
  if (!panel) {
    objc_autoreleasePoolPop(pool);
    return -NQ_ENOSYS;
  }
  int dir = (options->flags & NQ_FILE_DIALOG_DIRECTORY) != 0;
  send_bool(panel, "setCanChooseFiles:", dir ? NO : YES);
  send_bool(panel, "setCanChooseDirectories:", dir ? YES : NO);
  send_bool(panel, "setAllowsMultipleSelection:", NO);
  send_bool(panel, "setResolvesAliases:", YES);
  if (dir)
    send_bool(panel, "setCanCreateDirectories:", YES);
  if (options->title) {
    /* open panels have no title bar on recent macOS: say it as the message too */
    send_string(panel, "setTitle:", options->title);
    send_string(panel, "setMessage:", options->title);
  }
  if (options->initialPath && options->initialPath[0])
    set_initial(panel, options->initialPath);

  long response = ((long (*)(id, SEL))objc_msgSend)(panel, sel_registerName("runModal"));
  const char* chosen = NULL;
  int error = 0;
  if (response == MODAL_RESPONSE_OK) {
    id url = ((id (*)(id, SEL))objc_msgSend)(panel, sel_registerName("URL"));
    if (url)
      chosen = ((const char* (*)(id, SEL))objc_msgSend)(url, sel_registerName("fileSystemRepresentation"));
    if (!chosen)
      error = -NQ_EIO;
  }
  /* the path lives in the pool */
  callback(userdata, chosen, error);
  objc_autoreleasePoolPop(pool);
  return 0;
}

/* --- Linux and the BSDs: zenity, kdialog, qarma -------------------------- */

#elif defined(DIALOG_XDG)

/* Runs `callback` on a thread of its own that nobody waits for: detached,
 * and destroyed at once, which NQThread defers until the callback returns.
 * 0, or -NQ_ENOMEM when the thread could not be started. */
static int run_detached(NQThreadCallback callback, void* data)
{
  NQThread* thread = NQThread_create(callback, data, "NQFileDialog");
  if (!thread)
    return -NQ_ENOMEM;
  NQThread_detach(thread); /* before destroy, which may free it at once */
  NQThread_destroy(thread);
  return 0;
}

/* When the desktop portal is not there to ask (see further down) */

typedef enum {
  TOOL_ZENITY, /* GTK; also qarma, its Qt clone, which takes the same arguments */
  TOOL_KDIALOG
} ToolKind;

typedef struct {
  const char* name;
  ToolKind kind;
} Tool;

static const Tool s_tools[] = {
  {"zenity", TOOL_ZENITY},
  {"qarma", TOOL_ZENITY},
  {"kdialog", TOOL_KDIALOG},
};

/* What the reading thread needs */
typedef struct {
  pid_t pid;
  int fd; /* the read end of the program's standard output */
  NQFileDialogCallback callback;
  void* userdata;
} Pending;

/* The program `name` on PATH, written to `out` */
static int find_program(const char* name, char* out, size_t size)
{
  const char* env = NQEnvCreate("PATH");
  const char* path = env && *env ? env : "/usr/local/bin:/usr/bin:/bin";
  int found = 0;
  while (*path && !found) {
    const char* end = NQStrchr(path, NQ_POSIX_PATH_DELIMITER);
    size_t len = end ? (size_t)(end - path) : NQStrlen(path);
    if (len > 0 && (size_t)NQSnprintf(out, size, "%.*s/%s", (int)len, path, name) < size && access(out, X_OK) == 0)
      found = 1;
    path += len;
    if (*path == NQ_POSIX_PATH_DELIMITER)
      path++;
  }
  if (env)
    NQEnvFree(env);
  return found;
}

static int is_kde_session(void)
{
  const char* desktop = NQEnvCreate("XDG_CURRENT_DESKTOP");
  int kde = desktop && strstr(desktop, "KDE") != NULL;
  if (desktop)
    NQEnvFree(desktop);
  return kde;
}

/* The first program found, KDE's own first in a KDE session */
static const Tool* find_tool(char* path, size_t size)
{
  if (is_kde_session() && find_program("kdialog", path, size))
    return &s_tools[2];
  for (size_t i = 0; i < sizeof(s_tools) / sizeof(s_tools[0]); i++) {
    if (find_program(s_tools[i].name, path, size))
      return &s_tools[i];
  }
  return NULL;
}

static void read_result(void* arg)
{
  Pending* pending = arg;
  size_t len = 0, cap = 256;
  char* out = NQMalloc(cap);
  int error = out ? 0 : -NQ_ENOMEM;
  for (;;) {
    char buf[1024];
    ssize_t n = read(pending->fd, buf, sizeof(buf));
    if (n < 0 && NQGetLastError() == EINTR)
      continue;
    if (n <= 0)
      break;
    if (!out)
      continue; /* drain it all the same */
    if (len + (size_t)n + 1 > cap) {
      while (len + (size_t)n + 1 > cap)
        cap *= 2;
      char* grown = NQRealloc(out, cap);
      if (!grown) {
        NQFree(out);
        out = NULL;
        error = -NQ_ENOMEM;
        continue;
      }
      out = grown;
    }
    NQMemcpy(out + len, buf, (size_t)n);
    len += (size_t)n;
  }
  close(pending->fd);
  if (out) {
    while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r'))
      len--;
    out[len] = '\0';
  }

  int status = 0;
  pid_t r;
  do {
    r = waitpid(pending->pid, &status, 0);
  } while (r < 0 && NQGetLastError() == EINTR);

  /* exit status 0: chosen; 1: canceled or closed; anything else failed.
   * Reaped elsewhere (SIGCHLD ignored), go by what it printed. */
  int ok = r < 0 ? len > 0 : WIFEXITED(status) && WEXITSTATUS(status) == 0;
  int canceled = r < 0 ? len == 0 : WIFEXITED(status) && WEXITSTATUS(status) == 1;
  const char* path = NULL;
  if (ok && !error && len > 0)
    path = out;
  else if (!ok && !canceled)
    error = -NQ_EIO;
  else if (!ok)
    error = 0;
  pending->callback(pending->userdata, path, error);
  NQFree(out);
  NQFree(pending);
}

static int is_x11_override(const char* entry)
{
  return strncmp(entry, "GDK_BACKEND=", 12) == 0 || strncmp(entry, "QT_QPA_PLATFORM=", 16) == 0;
}

/* The process's environment (NQEnviron) as a NULL-ended array for
 * posix_spawn, in one NQMalloc'd block: the pointers, then the strings.
 * With `x11`, GDK_BACKEND and QT_QPA_PLATFORM are set for X11, so that the
 * window is one the X11 parent can be tied to. */
static char** spawn_environment(int x11)
{
  static char gdk[] = "GDK_BACKEND=x11";
  static char qt[] = "QT_QPA_PLATFORM=xcb";
  NQEnviron* environ_ = NQEnviron_create();
  size_t n = 0, bytes = 0;
  char probe;
  for (const NQEnvironIter* it = environ_ ? NQEnviron_begin(environ_) : NULL; it; it = NQEnvironIter_next(it)) {
    n++;
    bytes += NQEnvironIter_read(it, &probe, 1) + 1;
  }
  size_t slots = n + 3; /* gdk, qt, NULL */
  char** env = NQMalloc(slots * sizeof(char*) + bytes);
  if (!env) {
    if (environ_)
      NQEnviron_destroy(environ_);
    return NULL;
  }
  char* text = (char*)(env + slots);
  char* text_end = text + bytes;
  size_t k = 0;
  for (const NQEnvironIter* it = environ_ ? NQEnviron_begin(environ_) : NULL; it && k < n; it = NQEnvironIter_next(it)) {
    size_t len = NQEnvironIter_read(it, text, (size_t)(text_end - text));
    if (len >= (size_t)(text_end - text))
      break; /* changed since counted */
    if (x11 && is_x11_override(text))
      continue; /* the space is reused */
    env[k++] = text;
    text += len + 1;
  }
  if (environ_)
    NQEnviron_destroy(environ_);
  if (x11) {
    env[k++] = gdk;
    env[k++] = qt;
  }
  env[k] = NULL;
  return env;
}

/* A folder ends in '/' for zenity to start in it, not select it */
static char* folder_with_slash(const char* path)
{
  size_t n = NQStrlen(path);
  char* s = NQMalloc(n + 2);
  if (!s)
    return NULL;
  NQMemcpy(s, path, n);
  if (n == 0 || s[n - 1] != '/')
    s[n++] = '/';
  s[n] = '\0';
  return s;
}

static int tool_open(const NQFileDialogOptions* options, NQFileDialogCallback callback, void* userdata)
{
  char program[4096];
  const Tool* tool = find_tool(program, sizeof(program));
  if (!tool)
    return -NQ_ENOSYS;

  int dir = (options->flags & NQ_FILE_DIALOG_DIRECTORY) != 0;
  const char* initial = options->initialPath && options->initialPath[0] ? options->initialPath : NULL;
  struct stat st;
  int initial_is_dir = initial && stat(initial, &st) == 0 && S_ISDIR(st.st_mode);

  /* what the arguments point at, freed at the end */
  char* title_arg = NULL;
  char* file_arg = NULL;
  char attach[32];
  const char* argv[12];
  int argc = 0;
  argv[argc++] = program;

  if (tool->kind == TOOL_ZENITY) {
    argv[argc++] = "--file-selection";
    if (dir)
      argv[argc++] = "--directory";
    if (options->title) {
      title_arg = NQMalloc(strlen(options->title) + 9);
      if (title_arg) {
        strcpy(title_arg, "--title=");
        strcat(title_arg, options->title);
        argv[argc++] = title_arg;
      }
    }
    if (initial) {
      char* slashed = initial_is_dir ? folder_with_slash(initial) : NULL;
      const char* start = slashed ? slashed : initial;
      file_arg = NQMalloc(strlen(start) + 12);
      if (file_arg) {
        strcpy(file_arg, "--filename=");
        strcat(file_arg, start);
        argv[argc++] = file_arg;
      }
      NQFree(slashed);
    }
  }
  else {
    if (options->title) {
      argv[argc++] = "--title";
      argv[argc++] = options->title;
    }
    if (options->parent) {
      snprintf(attach, sizeof(attach), "%lu", (unsigned long)options->parent);
      argv[argc++] = "--attach";
      argv[argc++] = attach;
    }
    argv[argc++] = dir ? "--getexistingdirectory" : "--getopenfilename";
    if (initial)
      argv[argc++] = initial;
  }
  argv[argc] = NULL;

  int error = 0;
  int fds[2] = {-1, -1};
  Pending* pending = NULL;
  posix_spawn_file_actions_t actions;
  int have_actions = 0;

  if (pipe(fds) != 0) {
    error = -errno;
    goto done;
  }
  /* other programs started meanwhile must not hold the pipe open */
  fcntl(fds[0], F_SETFD, FD_CLOEXEC);
  fcntl(fds[1], F_SETFD, FD_CLOEXEC);

  pending = NQZeroMalloc(sizeof(*pending));
  if (!pending || posix_spawn_file_actions_init(&actions) != 0) {
    error = -NQ_ENOMEM;
    goto done;
  }
  have_actions = 1;
  posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
  posix_spawn_file_actions_adddup2(&actions, fds[1], STDOUT_FILENO);
  /* GTK and Qt warnings are not for the user */
  posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);

  char** env = spawn_environment(options->parent != 0);
  if (!env) {
    error = -NQ_ENOMEM;
    goto done;
  }
  pid_t pid;
  int rc = posix_spawn(&pid, program, &actions, NULL, (char* const*)argv, env);
  NQFree(env);
  if (rc != 0) {
    error = rc == ENOENT ? -NQ_ENOSYS : -rc;
    goto done;
  }
  close(fds[1]);
  fds[1] = -1;

  pending->pid = pid;
  pending->fd = fds[0];
  pending->callback = callback;
  pending->userdata = userdata;

  if (run_detached(read_result, pending) != 0) {
    kill(pid, SIGTERM);
    waitpid(pid, NULL, 0);
    error = -NQ_ENOMEM;
    goto done;
  }
  fds[0] = -1;
  pending = NULL; /* the thread's now */
  if (options->processId)
    *options->processId = (int64_t)pid;

done:
  if (have_actions)
    posix_spawn_file_actions_destroy(&actions);
  if (fds[0] >= 0)
    close(fds[0]);
  if (fds[1] >= 0)
    close(fds[1]);
  NQFree(pending);
  NQFree(title_arg);
  NQFree(file_arg);
  return error;
}

/* org.freedesktop.portal.FileChooser, asked over the session D-Bus with a
 * small client of its own (no libdbus): the desktop shows its file window
 * modal to the parent window given as "x11:XID" or "wayland:HANDLE", so the
 * window manager keeps it above and gives the focus back to it. GTK 4
 * programs, zenity 4 among them, ask the same; only, asked by zenity, the
 * window belongs to zenity's hidden window instead of the caller's. */

# define PORTAL_NAME    "org.freedesktop.portal.Desktop"
# define PORTAL_PATH    "/org/freedesktop/portal/desktop"
# define PORTAL_REQUEST "org.freedesktop.portal.Request"
# define DBUS_NAME      "org.freedesktop.DBus"
# define DBUS_MSG_CALL   1
# define DBUS_MSG_REPLY  2
# define DBUS_MSG_ERROR  3
# define DBUS_MSG_SIGNAL 4
/* how long the calls before the window shows may take, the portal
 * starting first among them */
# define PORTAL_CALL_TIMEOUT_S 25

/* What is written: a message's header or body, little-endian */
typedef struct {
  unsigned char* data;
  size_t len, cap;
  int failed;
} DBusBuf;

static void dbuf_reserve(DBusBuf* b, size_t n)
{
  if (b->failed || b->len + n <= b->cap)
    return;
  size_t cap = b->cap ? b->cap : 256;
  while (cap < b->len + n)
    cap *= 2;
  unsigned char* data = NQRealloc(b->data, cap);
  if (!data) {
    b->failed = 1;
    return;
  }
  b->data = data;
  b->cap = cap;
}

static void dbuf_bytes(DBusBuf* b, const void* p, size_t n)
{
  dbuf_reserve(b, n);
  if (b->failed)
    return;
  memcpy(b->data + b->len, p, n);
  b->len += n;
}

static void dbuf_align(DBusBuf* b, size_t a)
{
  static const unsigned char zeros[8];
  if (b->len % a)
    dbuf_bytes(b, zeros, a - b->len % a);
}

static void dbuf_byte(DBusBuf* b, unsigned char v)
{
  dbuf_bytes(b, &v, 1);
}

static void dbuf_set_u32(DBusBuf* b, size_t at, uint32_t v)
{
  if (b->failed)
    return;
  b->data[at] = (unsigned char)v;
  b->data[at + 1] = (unsigned char)(v >> 8);
  b->data[at + 2] = (unsigned char)(v >> 16);
  b->data[at + 3] = (unsigned char)(v >> 24);
}

static void dbuf_u32(DBusBuf* b, uint32_t v)
{
  dbuf_align(b, 4);
  dbuf_reserve(b, 4);
  if (b->failed)
    return;
  b->len += 4;
  dbuf_set_u32(b, b->len - 4, v);
}

/* s and o */
static void dbuf_string(DBusBuf* b, const char* s)
{
  size_t n = strlen(s);
  dbuf_u32(b, (uint32_t)n);
  dbuf_bytes(b, s, n + 1);
}

static void dbuf_signature(DBusBuf* b, const char* s)
{
  size_t n = strlen(s);
  dbuf_byte(b, (unsigned char)n);
  dbuf_bytes(b, s, n + 1);
}

/* An a{sv} entry's start: the key and the value's signature */
static void dbuf_entry(DBusBuf* b, const char* key, const char* signature)
{
  dbuf_align(b, 8);
  dbuf_string(b, key);
  dbuf_signature(b, signature);
}

/* What is read: one message, and a cursor in it. Alignment counts from the
 * message's start, as the header is padded to 8 before the body. */
typedef struct {
  unsigned char* data;
  size_t len;
  int big; /* big-endian */
  uint8_t type;
  uint32_t replySerial;
  const char* path;
  const char* member;
  const char* signature;
  size_t body;
} DBusMsg;

typedef struct {
  const unsigned char* d;
  size_t pos, end;
  int big;
  int bad;
} DBusRd;

static void drd_align(DBusRd* r, size_t a)
{
  r->pos = (r->pos + a - 1) / a * a;
  if (r->pos > r->end)
    r->bad = 1;
}

static void drd_advance(DBusRd* r, size_t n)
{
  if (r->bad || n > r->end - r->pos)
    r->bad = 1;
  else
    r->pos += n;
}

static uint32_t drd_u32(DBusRd* r)
{
  drd_align(r, 4);
  if (r->bad || r->end - r->pos < 4) {
    r->bad = 1;
    return 0;
  }
  const unsigned char* p = r->d + r->pos;
  r->pos += 4;
  if (r->big)
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
  return (uint32_t)p[3] << 24 | (uint32_t)p[2] << 16 | (uint32_t)p[1] << 8 | p[0];
}

static uint8_t drd_byte(DBusRd* r)
{
  if (r->bad || r->pos >= r->end) {
    r->bad = 1;
    return 0;
  }
  return r->d[r->pos++];
}

/* s and o */
static const char* drd_string(DBusRd* r)
{
  uint32_t n = drd_u32(r);
  if (r->bad || (size_t)n >= r->end - r->pos || r->d[r->pos + n] != 0) {
    r->bad = 1;
    return "";
  }
  const char* s = (const char*)r->d + r->pos;
  r->pos += (size_t)n + 1;
  return s;
}

static const char* drd_signature(DBusRd* r)
{
  size_t n = drd_byte(r);
  if (r->bad || n >= r->end - r->pos || r->d[r->pos + n] != 0) {
    r->bad = 1;
    return "";
  }
  const char* s = (const char*)r->d + r->pos;
  r->pos += n + 1;
  return s;
}

static size_t dbus_alignment(char c)
{
  switch (c) {
    case 'n': case 'q': return 2;
    case 'b': case 'i': case 'u': case 'h': case 's': case 'o': case 'a': return 4;
    case 'x': case 't': case 'd': case '(': case '{': return 8;
    default: return 1; /* y g v */
  }
}

/* Moves *sig past one complete type */
static void dbus_sig_skip(const char** sig)
{
  char c = *(*sig)++;
  if (c == 'a') {
    dbus_sig_skip(sig);
  }
  else if (c == '(' || c == '{') {
    char close = c == '(' ? ')' : '}';
    while (**sig && **sig != close)
      dbus_sig_skip(sig);
    if (**sig)
      (*sig)++;
  }
}

/* Skips the value of the type at *sig, moving *sig past it */
static void drd_skip(DBusRd* r, const char** sig, int depth)
{
  if (depth > 32) {
    r->bad = 1;
    return;
  }
  char c = *(*sig)++;
  switch (c) {
    case 'y': drd_byte(r); break;
    case 'b': case 'i': case 'u': case 'h': drd_u32(r); break;
    case 'n': case 'q': drd_align(r, 2); drd_advance(r, 2); break;
    case 'x': case 't': case 'd': drd_align(r, 8); drd_advance(r, 8); break;
    case 's': case 'o': drd_string(r); break;
    case 'g': drd_signature(r); break;
    case 'v': {
      const char* inner = drd_signature(r);
      if (!r->bad)
        drd_skip(r, &inner, depth + 1);
      break;
    }
    case 'a': {
      uint32_t n = drd_u32(r);
      drd_align(r, dbus_alignment(**sig));
      drd_advance(r, n);
      dbus_sig_skip(sig);
      break;
    }
    case '(':
    case '{': {
      char close = c == '(' ? ')' : '}';
      drd_align(r, 8);
      while (!r->bad && **sig && **sig != close)
        drd_skip(r, sig, depth + 1);
      if (**sig == close)
        (*sig)++;
      else
        r->bad = 1;
      break;
    }
    default: r->bad = 1; break;
  }
}

/* A failed send or recv as an NQ error code: the receive timeout as
 * -NQ_ETIMEDOUT, the bus closing the connection (0 bytes) as -NQ_EIO */
static int dbus_io_error(ssize_t n)
{
  if (n == 0)
    return -NQ_EIO;
  if (errno == EAGAIN || errno == EWOULDBLOCK)
    return -NQ_ETIMEDOUT;
  return errno ? -errno : -NQ_EIO;
}

static int dbus_write_all(int fd, const void* data, size_t len)
{
  const char* p = data;
  while (len > 0) {
    ssize_t n = send(fd, p, len, MSG_NOSIGNAL);
    if (n < 0 && errno == EINTR)
      continue;
    if (n <= 0)
      return dbus_io_error(n);
    p += n;
    len -= (size_t)n;
  }
  return 0;
}

static int dbus_read_all(int fd, void* data, size_t len)
{
  char* p = data;
  while (len > 0) {
    ssize_t n = recv(fd, p, len, 0);
    if (n < 0 && errno == EINTR)
      continue;
    if (n <= 0)
      return dbus_io_error(n);
    p += n;
    len -= (size_t)n;
  }
  return 0;
}

static void dbus_header_field(DBusBuf* h, unsigned char code, const char* signature, const char* value)
{
  dbuf_align(h, 8);
  dbuf_byte(h, code);
  dbuf_signature(h, signature);
  if (signature[0] == 'g')
    dbuf_signature(h, value);
  else
    dbuf_string(h, value);
}

/* Sends a method call */
static int dbus_call(int fd, uint32_t serial, const char* dest, const char* path, const char* iface,
                     const char* member, const char* signature, const DBusBuf* body)
{
  DBusBuf m = {0};
  dbuf_byte(&m, 'l');
  dbuf_byte(&m, DBUS_MSG_CALL);
  dbuf_byte(&m, 0);
  dbuf_byte(&m, 1);
  dbuf_u32(&m, body ? (uint32_t)body->len : 0);
  dbuf_u32(&m, serial);
  size_t at = m.len;
  dbuf_u32(&m, 0); /* the header fields' length */
  size_t start = m.len;
  dbus_header_field(&m, 1, "o", path);
  dbus_header_field(&m, 6, "s", dest);
  dbus_header_field(&m, 2, "s", iface);
  dbus_header_field(&m, 3, "s", member);
  if (signature && signature[0])
    dbus_header_field(&m, 8, "g", signature);
  dbuf_set_u32(&m, at, (uint32_t)(m.len - start));
  dbuf_align(&m, 8);
  if (body)
    dbuf_bytes(&m, body->data, body->len);
  int rc = m.failed || (body && body->failed) ? -NQ_ENOMEM : dbus_write_all(fd, m.data, m.len);
  NQFree(m.data);
  return rc;
}

static void dbus_msg_free(DBusMsg* msg)
{
  NQFree(msg->data);
  memset(msg, 0, sizeof(*msg));
}

/* Reads the next message */
static int dbus_read(int fd, DBusMsg* msg)
{
  NQMemset(msg, 0, sizeof(*msg));
  unsigned char fixed[16];
  int rc = dbus_read_all(fd, fixed, sizeof(fixed));
  if (rc != 0)
    return rc;
  if (fixed[0] != 'l' && fixed[0] != 'B')
    return -NQ_EIO;
  DBusRd r = {fixed, 0, sizeof(fixed), fixed[0] == 'B', 0};
  r.pos = 4;
  uint32_t body_len = drd_u32(&r);
  r.pos = 12;
  uint32_t fields_len = drd_u32(&r);
  size_t header = (16 + (size_t)fields_len + 7) / 8 * 8;
  if (fields_len > (1u << 26) || body_len > (1u << 26))
    return -NQ_EIO;
  size_t total = header + body_len;
  msg->data = NQMalloc(total);
  if (!msg->data)
    return -NQ_ENOMEM;
  memcpy(msg->data, fixed, sizeof(fixed));
  rc = dbus_read_all(fd, msg->data + 16, total - 16);
  if (rc != 0) {
    dbus_msg_free(msg);
    return rc;
  }
  msg->len = total;
  msg->big = fixed[0] == 'B';
  msg->type = fixed[1];
  msg->body = header;
  msg->path = msg->member = msg->signature = "";

  DBusRd f = {msg->data, 16, 16 + fields_len, msg->big, 0};
  while (!f.bad && f.pos < f.end) {
    drd_align(&f, 8);
    if (f.pos >= f.end)
      break;
    uint8_t code = drd_byte(&f);
    const char* sig = drd_signature(&f);
    if (f.bad)
      break;
    if (code == 1 && strcmp(sig, "o") == 0)
      msg->path = drd_string(&f);
    else if (code == 3 && strcmp(sig, "s") == 0)
      msg->member = drd_string(&f);
    else if (code == 5 && strcmp(sig, "u") == 0)
      msg->replySerial = drd_u32(&f);
    else if (code == 8 && strcmp(sig, "g") == 0)
      msg->signature = drd_signature(&f);
    else
      drd_skip(&f, &sig, 0);
  }
  if (f.bad) {
    dbus_msg_free(msg);
    return -NQ_EIO;
  }
  return 0;
}

/* Waits for the reply to call `serial`, skipping the rest (NameAcquired,
 * ...); 0 for a reply, -NQ_ENOSYS for an error reply (no such service or
 * method), or dbus_read's error for a broken connection */
static int dbus_wait_reply(int fd, uint32_t serial, DBusMsg* reply)
{
  for (;;) {
    int rc = dbus_read(fd, reply);
    if (rc != 0)
      return rc;
    if ((reply->type == DBUS_MSG_REPLY || reply->type == DBUS_MSG_ERROR) && reply->replySerial == serial)
      return reply->type == DBUS_MSG_REPLY ? 0 : -NQ_ENOSYS;
    dbus_msg_free(reply);
  }
}

/* The first string of a reply whose body starts with one (s or o), copied */
static int dbus_reply_string(const DBusMsg* reply, char* out, size_t size)
{
  if (reply->signature[0] != 's' && reply->signature[0] != 'o')
    return -NQ_EIO;
  DBusRd r = {reply->data, reply->body, reply->len, reply->big, 0};
  const char* s = drd_string(&r);
  if (r.bad || strlen(s) >= size)
    return -NQ_EIO;
  strcpy(out, s);
  return 0;
}

/* Connects to the session bus, at $DBUS_SESSION_BUS_ADDRESS (its first
 * unix:path= or unix:abstract=) or else $XDG_RUNTIME_DIR/bus: the socket,
 * or -NQ_ENOENT for no address, -NQ_EACCES when the bus turns us away, or
 * the socket's error */
static int dbus_connect_session(void)
{
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  socklen_t addr_len = 0;

  const char* env = NQEnvCreate("DBUS_SESSION_BUS_ADDRESS");
  const char* address = env;
  while (address && *address && addr_len == 0) {
    const char* end = strchr(address, ';');
    size_t len = end ? (size_t)(end - address) : strlen(address);
    if (len > 5 && strncmp(address, "unix:", 5) == 0) {
      const char* keys[2] = {"path=", "abstract="};
      for (int k = 0; k < 2 && addr_len == 0; k++) {
        const char* at = address + 5;
        while (at < address + len) {
          size_t klen = strlen(keys[k]);
          if ((size_t)(address + len - at) > klen && strncmp(at, keys[k], klen) == 0) {
            const char* value = at + klen;
            const char* vend = memchr(value, ',', (size_t)(address + len - value));
            size_t vlen = vend ? (size_t)(vend - value) : (size_t)(address + len - value);
            size_t off = k == 1 ? 1 : 0; /* abstract: a NUL first */
            if (vlen > 0 && off + vlen < sizeof(addr.sun_path)) {
              memcpy(addr.sun_path + off, value, vlen);
              addr_len = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + off + vlen + (k == 0 ? 1 : 0));
            }
            break;
          }
          const char* comma = memchr(at, ',', (size_t)(address + len - at));
          if (!comma)
            break;
          at = comma + 1;
        }
      }
    }
    address = end ? end + 1 : NULL;
  }
  if (env)
    NQEnvFree(env);
  if (addr_len == 0) {
    const char* runtime = NQEnvCreate("XDG_RUNTIME_DIR");
    int fits = runtime && (size_t)snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/bus", runtime) < sizeof(addr.sun_path);
    if (runtime)
      NQEnvFree(runtime);
    if (!fits)
      return -NQ_ENOENT;
    addr_len = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path) + 1);
  }

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0)
    return -errno;
  fcntl(fd, F_SETFD, FD_CLOEXEC);
  if (connect(fd, (struct sockaddr*)&addr, addr_len) != 0) {
    int rc = -errno;
    close(fd);
    return rc;
  }

  /* SASL EXTERNAL: who we are is the socket's credentials */
  char uid[32], auth[128];
  snprintf(uid, sizeof(uid), "%lu", (unsigned long)getuid());
  size_t n = 0;
  auth[n++] = '\0';
  n += (size_t)snprintf(auth + n, sizeof(auth) - n, "AUTH EXTERNAL ");
  for (const char* c = uid; *c; c++)
    n += (size_t)snprintf(auth + n, sizeof(auth) - n, "%02x", (unsigned char)*c);
  n += (size_t)snprintf(auth + n, sizeof(auth) - n, "\r\n");
  char line[256];
  size_t got = 0;
  int rc = dbus_write_all(fd, auth, n);
  if (rc != 0)
    goto fail;
  while (got < sizeof(line) - 1) {
    rc = dbus_read_all(fd, line + got, 1);
    if (rc != 0)
      goto fail;
    if (line[got++] == '\n')
      break;
  }
  line[got] = '\0';
  if (strncmp(line, "OK ", 3) != 0) {
    rc = -NQ_EACCES;
    goto fail;
  }
  rc = dbus_write_all(fd, "BEGIN\r\n", 7);
  if (rc != 0)
    goto fail;
  return fd;

fail:
  close(fd);
  return rc;
}

static void dbus_set_timeout(int fd, int seconds)
{
  struct timeval tv = {seconds, 0};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static int hex_value(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

/* "file:///a%20b" to "/a b", in place; 0, or -NQ_EINVAL for another kind
 * of URI */
static int file_uri_to_path(char* uri)
{
  if (strncmp(uri, "file://", 7) != 0)
    return -NQ_EINVAL;
  const char* in = strchr(uri + 7, '/'); /* past the host, if any */
  if (!in)
    return -NQ_EINVAL;
  char* out = uri;
  while (*in) {
    int hi, lo;
    if (in[0] == '%' && (hi = hex_value(in[1])) >= 0 && (lo = hex_value(in[2])) >= 0) {
      *out++ = (char)(hi << 4 | lo);
      in += 3;
    }
    else {
      *out++ = *in++;
    }
  }
  *out = '\0';
  return 0;
}

typedef struct {
  int fd;
  char request[256]; /* the Request object the answer comes from */
  NQFileDialogCallback callback;
  void* userdata;
} PortalPending;

/* `s` copied with NQMalloc */
static char* copy_string(const char* s)
{
  size_t n = strlen(s) + 1;
  char* copy = NQMalloc(n);
  if (copy)
    memcpy(copy, s, n);
  return copy;
}

/* The answer: Response (u response, a{sv} results), with the file in
 * results["uris"] (as) */
static void portal_answer(const DBusMsg* msg, NQFileDialogCallback callback, void* userdata)
{
  DBusRd r = {msg->data, msg->body, msg->len, msg->big, 0};
  uint32_t response = strcmp(msg->signature, "ua{sv}") == 0 ? drd_u32(&r) : 2;
  if (response == 1) {
    callback(userdata, NULL, 0); /* canceled */
    return;
  }
  char* path = NULL;
  if (response == 0) {
    uint32_t len = drd_u32(&r);
    drd_align(&r, 8);
    size_t end = r.pos + len;
    if (end > r.end)
      r.bad = 1;
    while (!r.bad && r.pos < end && !path) {
      drd_align(&r, 8);
      const char* key = drd_string(&r);
      const char* sig = drd_signature(&r);
      if (!r.bad && strcmp(key, "uris") == 0 && strcmp(sig, "as") == 0) {
        uint32_t n = drd_u32(&r);
        if (n > 0) {
          const char* uri = drd_string(&r);
          if (!r.bad)
            path = copy_string(uri);
        }
        break;
      }
      drd_skip(&r, &sig, 0);
    }
  }
  if (path && file_uri_to_path(path) == 0) {
    callback(userdata, path, 0);
  }
  else {
    callback(userdata, NULL, response == 0 && !path ? 0 : -NQ_EIO);
  }
  NQFree(path);
}

static void portal_wait(void* arg)
{
  PortalPending* pending = arg;
  for (;;) {
    DBusMsg msg;
    if (dbus_read(pending->fd, &msg) != 0) {
      pending->callback(pending->userdata, NULL, -NQ_EIO); /* the bus went away */
      break;
    }
    if (msg.type == DBUS_MSG_SIGNAL && strcmp(msg.member, "Response") == 0 &&
        strcmp(msg.path, pending->request) == 0) {
      portal_answer(&msg, pending->callback, pending->userdata);
      dbus_msg_free(&msg);
      break;
    }
    dbus_msg_free(&msg);
  }
  close(pending->fd);
  NQFree(pending);
}

static int dbus_add_match(int fd, uint32_t serial, const char* request)
{
  char rule[512];
  snprintf(rule, sizeof(rule), "type='signal',interface='" PORTAL_REQUEST "',member='Response',path='%s'", request);
  DBusBuf body = {0};
  dbuf_string(&body, rule);
  DBusMsg reply = {0};
  int rc = dbus_call(fd, serial, DBUS_NAME, "/org/freedesktop/DBus", DBUS_NAME, "AddMatch", "s", &body);
  NQFree(body.data);
  if (rc == 0)
    rc = dbus_wait_reply(fd, serial, &reply);
  dbus_msg_free(&reply);
  return rc;
}

/* Asks the portal; 0 when it will answer, or a negative NQ error code when
 * it cannot be asked (no bus, no portal, an old one: -NQ_ENOSYS for an
 * error reply), to ask zenity or kdialog instead */
static int portal_open(const NQFileDialogOptions* options, NQFileDialogCallback callback, void* userdata)
{
  int fd = dbus_connect_session();
  if (fd < 0)
    return fd;
  int rc;
  dbus_set_timeout(fd, PORTAL_CALL_TIMEOUT_S);
  PortalPending* pending = NULL;
  DBusMsg reply = {0};
  DBusBuf body = {0};
  char unique[128];

  /* Hello: our name on the bus, which the request's path is made from */
  if ((rc = dbus_call(fd, 1, DBUS_NAME, "/org/freedesktop/DBus", DBUS_NAME, "Hello", NULL, NULL)) != 0 ||
      (rc = dbus_wait_reply(fd, 1, &reply)) != 0 || (rc = dbus_reply_string(&reply, unique, sizeof(unique))) != 0) {
    goto fail;
  }
  dbus_msg_free(&reply);

  pending = NQZeroMalloc(sizeof(*pending));
  if (!pending) {
    rc = -NQ_ENOMEM;
    goto fail;
  }
  static unsigned s_counter;
  char token[64];
  snprintf(token, sizeof(token), "libnetq%ld_%u", (long)getpid(), __atomic_add_fetch(&s_counter, 1, __ATOMIC_RELAXED));
  char sender[128];
  size_t k = 0;
  for (const char* c = unique[0] == ':' ? unique + 1 : unique; *c && k < sizeof(sender) - 1; c++)
    sender[k++] = *c == '.' ? '_' : *c;
  sender[k] = '\0';
  snprintf(pending->request, sizeof(pending->request), PORTAL_PATH "/request/%s/%s", sender, token);
  /* listen before asking, so that the answer cannot come first */
  if ((rc = dbus_add_match(fd, 2, pending->request)) != 0)
    goto fail;

  int dir = (options->flags & NQ_FILE_DIALOG_DIRECTORY) != 0;
  char parent[64] = "";
  if (options->parentHandle)
    snprintf(parent, sizeof(parent), "%s", options->parentHandle);
  else if (options->parent)
    snprintf(parent, sizeof(parent), "x11:%lx", (unsigned long)options->parent);

  /* OpenFile (s parent_window, s title, a{sv} options) */
  dbuf_string(&body, parent);
  dbuf_string(&body, options->title ? options->title : dir ? "Open Folder" : "Open File");
  dbuf_align(&body, 4); /* where the array's length goes */
  size_t at = body.len;
  dbuf_u32(&body, 0);
  dbuf_align(&body, 8);
  size_t start = body.len;
  dbuf_entry(&body, "handle_token", "s");
  dbuf_string(&body, token);
  dbuf_entry(&body, "modal", "b");
  dbuf_u32(&body, 1);
  dbuf_entry(&body, "multiple", "b");
  dbuf_u32(&body, 0);
  dbuf_entry(&body, "directory", "b");
  dbuf_u32(&body, dir ? 1 : 0);
  if (options->initialPath && options->initialPath[0]) {
    /* a folder to start in: a file's own folder */
    char folder[4096];
    NQSnprintf(folder, sizeof(folder), "%s", options->initialPath);
    struct stat st;
    if (stat(folder, &st) != 0 || !S_ISDIR(st.st_mode)) {
      char* slash = strrchr(folder, '/');
      if (slash && slash != folder)
        *slash = '\0';
      else if (slash)
        slash[1] = '\0';
    }
    if (stat(folder, &st) == 0 && S_ISDIR(st.st_mode)) {
      dbuf_entry(&body, "current_folder", "ay");
      dbuf_u32(&body, (uint32_t)strlen(folder) + 1);
      dbuf_bytes(&body, folder, strlen(folder) + 1);
    }
  }
  dbuf_set_u32(&body, at, (uint32_t)(body.len - start));
  if ((rc = dbus_call(fd, 3, PORTAL_NAME, PORTAL_PATH, "org.freedesktop.portal.FileChooser", "OpenFile", "ssa{sv}", &body)) != 0)
    goto fail;
  NQFree(body.data);
  body.data = NULL;
  char handle[256];
  if ((rc = dbus_wait_reply(fd, 3, &reply)) != 0 || (rc = dbus_reply_string(&reply, handle, sizeof(handle))) != 0)
    goto fail; /* no portal, or no FileChooser in it */
  dbus_msg_free(&reply);
  /* portals before version 0.9 make the path up themselves */
  if (strcmp(handle, pending->request) != 0) {
    snprintf(pending->request, sizeof(pending->request), "%s", handle);
    if ((rc = dbus_add_match(fd, 4, pending->request)) != 0)
      goto fail;
  }

  /* the window is up: the answer comes when the user is done */
  dbus_set_timeout(fd, 0);
  pending->fd = fd;
  pending->callback = callback;
  pending->userdata = userdata;
  if ((rc = run_detached(portal_wait, pending)) != 0)
    goto fail;
  return 0;

fail:
  dbus_msg_free(&reply);
  NQFree(body.data);
  NQFree(pending);
  close(fd);
  return rc;
}

static int dialog_open(const NQFileDialogOptions* options, NQFileDialogCallback callback, void* userdata)
{
  if (portal_open(options, callback, userdata) == 0)
    return 0;
  return tool_open(options, callback, userdata);
}

#else

static int dialog_open(const NQFileDialogOptions* options, NQFileDialogCallback callback, void* userdata)
{
  NQ_UNUSED_PARAM(options);
  NQ_UNUSED_PARAM(callback);
  NQ_UNUSED_PARAM(userdata);
  return -NQ_ENOSYS;
}

#endif

int NQFileDialogOpen(const NQFileDialogOptions* options, NQFileDialogCallback callback, void* userdata)
{
  if (options && options->processId)
    *options->processId = 0;
  if (!callback)
    return -NQ_EINVAL;
  return dialog_open(options ? options : &s_defaultOptions, callback, userdata);
}
