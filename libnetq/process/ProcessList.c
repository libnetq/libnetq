/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/process/Process.h"

#include <libnetq/ErrorCode.h>
#include <libnetq/Malloc.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/string/String.h>

#if defined(NQ_OS_WINDOWS)
# define PSAPI_VERSION 2
# include <windows.h>
# include <psapi.h>
# include <tlhelp32.h>
# include <libnetq/Time.h>
# include <libnetq/UTF.h>
#elif defined(NQ_OS_UNIX)
# include <errno.h>
# include <pwd.h>
# include <stdlib.h>
# include <sys/types.h>
# include <unistd.h>
# if defined(NQ_OS_LINUX)
#  include <dirent.h>
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <sys/sysmacros.h>
# elif defined(NQ_OS_DARWIN)
#  include <libproc.h>
#  include <mach/mach_time.h>
#  include <sys/proc_info.h>
#  include <sys/stat.h>
#  include <sys/sysctl.h>
# endif
#endif

/* --- common -------------------------------------------------------------- */

typedef struct {
  NQProcessInfo* items;
  size_t count;
  size_t capacity;
} ProcessArray;

static NQProcessInfo* array_add(ProcessArray* a)
{
  if (a->count == a->capacity) {
    size_t capacity = a->capacity ? a->capacity * 2 : 256;
    NQProcessInfo* items = NQRealloc(a->items, capacity * sizeof(*items));
    if (!items)
      return NULL;
    a->items = items;
    a->capacity = capacity;
  }
  NQProcessInfo* p = &a->items[a->count++];
  NQMemset(p, 0, sizeof(*p));
  p->tpgid = -1;
  p->ruid = p->euid = p->rgid = p->egid = -1;
  return p;
}

static void copy_string(char* dst, size_t n, const char* src, size_t len)
{
  if (len >= n)
    len = n - 1;
  NQMemcpy(dst, src, len);
  dst[len] = '\0';
}

void NQFreeProcessList(NQProcessInfo* list, size_t count)
{
  if (!list)
    return;
  for (size_t i = 0; i < count; i++)
    NQFree(list[i].args);
  NQFree(list);
}

int64_t NQGetProcessId(void)
{
#if defined(NQ_OS_WINDOWS)
  return (int64_t)GetCurrentProcessId();
#elif defined(NQ_OS_UNIX)
  return (int64_t)getpid();
#else
  return 0;
#endif
}

#if defined(NQ_OS_UNIX)

/* The user names looked up so far: a listing has few users */
typedef struct {
  uid_t uid;
  char name[64];
} UserName;

typedef struct {
  UserName* items;
  size_t count;
} UserCache;

static void user_name(UserCache* cache, uid_t uid, char* out, size_t n)
{
  for (size_t i = 0; i < cache->count; i++) {
    if (cache->items[i].uid == uid) {
      copy_string(out, n, cache->items[i].name, NQStrlen(cache->items[i].name));
      return;
    }
  }
  char name[64];
  struct passwd* pw = getpwuid(uid);
  if (pw && pw->pw_name)
    copy_string(name, sizeof(name), pw->pw_name, NQStrlen(pw->pw_name));
  else
    NQSnprintf(name, sizeof(name), "%u", (unsigned)uid);
  UserName* items = NQRealloc(cache->items, (cache->count + 1) * sizeof(*items));
  if (items) {
    cache->items = items;
    items[cache->count].uid = uid;
    NQMemcpy(items[cache->count].name, name, sizeof(name));
    cache->count++;
  }
  copy_string(out, n, name, NQStrlen(name));
}

#endif

/* --- Linux --------------------------------------------------------------- */

#if defined(NQ_OS_LINUX)

/* Reads a small file whole, NUL-terminated; NULL if it cannot */
static char* read_file(const char* path, size_t* length)
{
  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return NULL;
  size_t size = 0, capacity = 1024;
  char* buf = NQMalloc(capacity);
  while (buf) {
    if (size + 1 >= capacity) {
      char* more = NQRealloc(buf, capacity * 2);
      if (!more) {
        NQFree(buf);
        buf = NULL;
        break;
      }
      buf = more;
      capacity *= 2;
    }
    ssize_t n = read(fd, buf + size, capacity - size - 1);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      NQFree(buf);
      buf = NULL;
      break;
    }
    if (n == 0)
      break;
    size += (size_t)n;
  }
  close(fd);
  if (!buf)
    return NULL;
  buf[size] = '\0';
  if (length)
    *length = size;
  return buf;
}

/* The terminal names found so far, by device number */
typedef struct {
  dev_t dev;
  char name[32];
} TtyName;

typedef struct {
  TtyName* items;
  size_t count;
} TtyCache;

/* Whether /dev/`name` is the character device `dev` */
static int is_device(const char* name, dev_t dev)
{
  char path[64];
  struct stat st;
  NQSnprintf(path, sizeof(path), "/dev/%s", name);
  return stat(path, &st) == 0 && S_ISCHR(st.st_mode) && st.st_rdev == dev;
}

/* Finds the name of terminal `dev` as procps does: a pseudo-terminal by
 * its number, else what the process has open on it, else a search of
 * /dev */
static void tty_name(TtyCache* cache, int64_t pid, dev_t dev, char* out, size_t n)
{
  for (size_t i = 0; i < cache->count; i++) {
    if (cache->items[i].dev == dev) {
      copy_string(out, n, cache->items[i].name, NQStrlen(cache->items[i].name));
      return;
    }
  }

  char name[32] = "";
  unsigned maj = major(dev), min = minor(dev);
  if (maj >= 136 && maj <= 143) {
    NQSnprintf(name, sizeof(name), "pts/%u", (maj - 136) * 256 + min);
  } else if (maj == 4 && min < 64) {
    NQSnprintf(name, sizeof(name), "tty%u", min);
  } else if (maj == 4 && min < 256) {
    NQSnprintf(name, sizeof(name), "ttyS%u", min - 64);
  }
  if (name[0] && !is_device(name, dev))
    name[0] = '\0';

  static const int fds[] = { 2, 0, 1, 255 };
  for (size_t i = 0; !name[0] && i < NQ_ARRAY_LENGTH(fds); i++) {
    char path[64], link[256];
    NQSnprintf(path, sizeof(path), "/proc/%lld/fd/%d", (long long)pid, fds[i]);
    ssize_t len = readlink(path, link, sizeof(link) - 1);
    if (len <= 5)
      continue;
    link[len] = '\0';
    if (NQStrncmp(link, "/dev/", 5) == 0 && is_device(link + 5, dev))
      copy_string(name, sizeof(name), link + 5, (size_t)len - 5);
  }

  if (!name[0]) {
    DIR* d = opendir("/dev");
    struct dirent* e;
    while (d && (e = readdir(d)) != NULL) {
      if (e->d_type != DT_CHR && e->d_type != DT_UNKNOWN)
        continue;
      if (is_device(e->d_name, dev)) {
        copy_string(name, sizeof(name), e->d_name, NQStrlen(e->d_name));
        break;
      }
    }
    if (d)
      closedir(d);
  }
  if (!name[0])
    NQSnprintf(name, sizeof(name), "%u,%u", maj, min);

  TtyName* items = NQRealloc(cache->items, (cache->count + 1) * sizeof(*items));
  if (items) {
    cache->items = items;
    items[cache->count].dev = dev;
    NQMemcpy(items[cache->count].name, name, sizeof(name));
    cache->count++;
  }
  copy_string(out, n, name, NQStrlen(name));
}

/* The value of a "Name:" line of /proc/PID/status */
static const char* status_field(const char* status, const char* name)
{
  size_t len = NQStrlen(name);
  for (const char* line = status; line && *line;) {
    if (NQStrncmp(line, name, len) == 0 && line[len] == ':')
      return line + len + 1;
    line = NQStrchr(line, '\n');
    if (line)
      line++;
  }
  return NULL;
}

typedef struct {
  long hz;
  long pageSize;
  int64_t bootMs;
  UserCache users;
  TtyCache ttys;
} LinuxContext;

static int read_process(LinuxContext* ctx, int64_t pid, NQProcessInfo* p)
{
  char path[64];
  NQSnprintf(path, sizeof(path), "/proc/%lld/stat", (long long)pid);
  char* stat = read_file(path, NULL);
  if (!stat)
    return -1;

  /* the name may hold spaces and parentheses: it ends at the last ')' */
  char* open = NQStrchr(stat, '(');
  char* close = NQStrrchr(stat, ')');
  if (!open || !close || close < open || close[1] != ' ') {
    NQFree(stat);
    return -1;
  }
  p->pid = pid;
  copy_string(p->name, sizeof(p->name), open + 1, (size_t)(close - open - 1));

  /* fields from the third, the state, on */
  const char* field[40];
  int count = 0;
  for (char* s = close + 2; *s && count < (int)NQ_ARRAY_LENGTH(field);) {
    field[count++] = s;
    while (*s && *s != ' ')
      s++;
    while (*s == ' ')
      *s++ = '\0';
  }
#define F(n) (count > (n) - 3 ? field[(n) - 3] : "0")
  p->state = F(3)[0];
  p->ppid = strtoll(F(4), NULL, 10);
  p->pgid = strtoll(F(5), NULL, 10);
  p->sid = strtoll(F(6), NULL, 10);
  int ttyNr = (int)strtol(F(7), NULL, 10);
  p->tpgid = strtoll(F(8), NULL, 10);
  p->flags = (unsigned)strtoul(F(9), NULL, 10);
  p->minflt = strtoull(F(10), NULL, 10);
  p->majflt = strtoull(F(12), NULL, 10);
  unsigned long long ticks = strtoull(F(14), NULL, 10) + strtoull(F(15), NULL, 10);
  p->priority = (int)strtol(F(18), NULL, 10);
  p->nice = (int)strtol(F(19), NULL, 10);
  p->threads = (int)strtol(F(20), NULL, 10);
  unsigned long long start = strtoull(F(22), NULL, 10);
  p->vsize = strtoull(F(23), NULL, 10);
  p->rss = strtoull(F(24), NULL, 10) * (uint64_t)ctx->pageSize;
  unsigned long long startCode = strtoull(F(26), NULL, 10);
  unsigned long long endCode = strtoull(F(27), NULL, 10);
  p->processor = (int)strtol(F(39), NULL, 10);
  p->rtprio = (int)strtol(F(40), NULL, 10);
  p->policy = (int)strtol(F(41), NULL, 10);
#undef F
  NQFree(stat);

  p->text = endCode > startCode ? endCode - startCode : 0;
  p->cpuTime = (int64_t)(ticks * 1000 / (unsigned long long)ctx->hz);
  p->startTime = ctx->bootMs + (int64_t)(start * 1000 / (unsigned long long)ctx->hz);
  if (ttyNr != 0) {
    dev_t dev = makedev(((unsigned)ttyNr >> 8) & 0xfff, ((unsigned)ttyNr & 0xff) | (((unsigned)ttyNr >> 12) & 0xfff00));
    tty_name(&ctx->ttys, pid, dev, p->tty, sizeof(p->tty));
  }

  NQSnprintf(path, sizeof(path), "/proc/%lld/status", (long long)pid);
  char* status = read_file(path, NULL);
  if (status) {
    const char* v;
    if ((v = status_field(status, "Uid")) != NULL) {
      char* end;
      p->ruid = strtoll(v, &end, 10);
      p->euid = strtoll(end, NULL, 10);
    }
    if ((v = status_field(status, "Gid")) != NULL) {
      char* end;
      p->rgid = strtoll(v, &end, 10);
      p->egid = strtoll(end, NULL, 10);
    }
    if ((v = status_field(status, "VmLck")) != NULL)
      p->locked = strtoull(v, NULL, 10) * 1024;
    /* more exact than the count in stat, which may lag */
    if ((v = status_field(status, "VmRSS")) != NULL)
      p->rss = strtoull(v, NULL, 10) * 1024;
    NQFree(status);
  }
  if (p->euid >= 0)
    user_name(&ctx->users, (uid_t)p->euid, p->user, sizeof(p->user));

  size_t len = 0;
  NQSnprintf(path, sizeof(path), "/proc/%lld/cmdline", (long long)pid);
  char* args = read_file(path, &len);
  while (args && len > 0 && args[len - 1] == '\0')
    len--;
  if (args && len > 0) {
    for (size_t i = 0; i < len; i++) {
      if (args[i] == '\0')
        args[i] = ' ';
    }
    args[len] = '\0';
    p->args = args;
  } else {
    NQFree(args);
  }

  NQSnprintf(path, sizeof(path), "/proc/%lld/wchan", (long long)pid);
  char* wchan = read_file(path, &len);
  if (wchan && NQStrcmp(wchan, "0") != 0)
    copy_string(p->wchan, sizeof(p->wchan), wchan, len);
  NQFree(wchan);
  return 0;
}

static int64_t boot_time_ms(void)
{
  char* stat = read_file("/proc/stat", NULL);
  int64_t ms = 0;
  for (const char* line = stat; line && *line;) {
    if (NQStrncmp(line, "btime ", 6) == 0) {
      ms = strtoll(line + 6, NULL, 10) * 1000;
      break;
    }
    line = NQStrchr(line, '\n');
    if (line)
      line++;
  }
  NQFree(stat);
  return ms;
}

static int get_process_list(ProcessArray* a)
{
  DIR* d = opendir("/proc");
  if (!d)
    return -errno;
  LinuxContext ctx;
  NQMemset(&ctx, 0, sizeof(ctx));
  ctx.hz = sysconf(_SC_CLK_TCK);
  if (ctx.hz <= 0)
    ctx.hz = 100;
  ctx.pageSize = sysconf(_SC_PAGESIZE);
  if (ctx.pageSize <= 0)
    ctx.pageSize = 4096;
  ctx.bootMs = boot_time_ms();

  int ret = 0;
  struct dirent* e;
  while ((e = readdir(d)) != NULL) {
    char* end;
    long long pid = strtoll(e->d_name, &end, 10);
    if (*end || end == e->d_name || pid <= 0)
      continue;
    NQProcessInfo* p = array_add(a);
    if (!p) {
      ret = -NQ_ENOMEM;
      break;
    }
    if (read_process(&ctx, (int64_t)pid, p) != 0)
      a->count--; /* it exited meanwhile */
  }
  closedir(d);
  NQFree(ctx.users.items);
  NQFree(ctx.ttys.items);
  return ret;
}

uint64_t NQGetPhysicalMemorySize(void)
{
  char* meminfo = read_file("/proc/meminfo", NULL);
  uint64_t total = 0;
  if (meminfo && NQStrncmp(meminfo, "MemTotal:", 9) == 0)
    total = strtoull(meminfo + 9, NULL, 10) * 1024;
  NQFree(meminfo);
  if (total == 0) {
    long pages = sysconf(_SC_PHYS_PAGES), size = sysconf(_SC_PAGESIZE);
    if (pages > 0 && size > 0)
      total = (uint64_t)pages * (uint64_t)size;
  }
  return total;
}

/* --- macOS --------------------------------------------------------------- */

#elif defined(NQ_OS_DARWIN)

/* The arguments of `pid` from KERN_PROCARGS2, joined with spaces */
static char* process_args(pid_t pid)
{
  int mib[3] = { CTL_KERN, KERN_ARGMAX, 0 };
  int argmax = 0;
  size_t size = sizeof(argmax);
  if (sysctl(mib, 2, &argmax, &size, NULL, 0) != 0 || argmax <= 0)
    return NULL;
  char* buf = NQMalloc((size_t)argmax);
  if (!buf)
    return NULL;
  mib[1] = KERN_PROCARGS2;
  mib[2] = pid;
  size = (size_t)argmax;
  if (sysctl(mib, 3, buf, &size, NULL, 0) != 0 || size <= sizeof(int)) {
    NQFree(buf);
    return NULL;
  }

  /* argc, the executable's path, padding, then the arguments */
  int argc;
  NQMemcpy(&argc, buf, sizeof(argc));
  char* end = buf + size;
  char* s = buf + sizeof(argc);
  while (s < end && *s)
    s++;
  while (s < end && !*s)
    s++;
  char* args = s;
  char* out = s;
  for (int i = 0; i < argc && s < end; i++) {
    if (i > 0)
      *out++ = ' ';
    while (s < end && *s)
      *out++ = *s++;
    s++;
  }
  size_t len = (size_t)(out - args);
  char* result = len ? NQMalloc(len + 1) : NULL;
  if (result) {
    NQMemcpy(result, args, len);
    result[len] = '\0';
  }
  NQFree(buf);
  return result;
}

static int get_process_list(ProcessArray* a)
{
  int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
  struct kinfo_proc* procs = NULL;
  size_t size = 0;
  for (int tries = 0;; tries++) {
    if (sysctl(mib, 4, NULL, &size, NULL, 0) != 0)
      return -errno;
    size += size / 8;
    struct kinfo_proc* more = NQRealloc(procs, size);
    if (!more) {
      NQFree(procs);
      return -NQ_ENOMEM;
    }
    procs = more;
    if (sysctl(mib, 4, procs, &size, NULL, 0) == 0)
      break;
    if (errno != ENOMEM || tries == 8) {
      int ret = -errno;
      NQFree(procs);
      return ret;
    }
  }

  mach_timebase_info_data_t timebase;
  if (mach_timebase_info(&timebase) != KERN_SUCCESS || timebase.denom == 0)
    timebase.numer = timebase.denom = 1;
  UserCache users = { NULL, 0 };
  int ret = 0;
  size_t n = size / sizeof(struct kinfo_proc);
  for (size_t i = 0; i < n; i++) {
    const struct kinfo_proc* kp = &procs[i];
    NQProcessInfo* p = array_add(a);
    if (!p) {
      ret = -NQ_ENOMEM;
      break;
    }
    pid_t pid = kp->kp_proc.p_pid;
    p->pid = pid;
    p->ppid = kp->kp_eproc.e_ppid;
    p->pgid = kp->kp_eproc.e_pgid;
    pid_t sid = getsid(pid);
    p->sid = sid < 0 ? 0 : sid;
    p->tpgid = kp->kp_eproc.e_tpgid;
    p->ruid = kp->kp_eproc.e_pcred.p_ruid;
    p->euid = kp->kp_eproc.e_ucred.cr_uid;
    p->rgid = kp->kp_eproc.e_pcred.p_rgid;
    p->egid = kp->kp_eproc.e_ucred.cr_ngroups > 0 ? kp->kp_eproc.e_ucred.cr_groups[0] : p->rgid;
    user_name(&users, (uid_t)p->euid, p->user, sizeof(p->user));
    if (kp->kp_eproc.e_tdev != NODEV) {
      const char* tty = devname(kp->kp_eproc.e_tdev, S_IFCHR);
      if (tty)
        copy_string(p->tty, sizeof(p->tty), tty, NQStrlen(tty));
    }
    copy_string(p->name, sizeof(p->name), kp->kp_proc.p_comm, NQStrlen(kp->kp_proc.p_comm));
    switch (kp->kp_proc.p_stat) {
      case SRUN: p->state = 'R'; break;
      case SSTOP: p->state = 'T'; break;
      case SZOMB: p->state = 'Z'; break;
      default: p->state = 'S'; break;
    }
    p->nice = kp->kp_proc.p_nice;
    p->priority = 20 + p->nice;
    p->startTime = (int64_t)kp->kp_proc.p_starttime.tv_sec * 1000 + kp->kp_proc.p_starttime.tv_usec / 1000;

    struct proc_taskinfo ti;
    if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &ti, sizeof(ti)) == (int)sizeof(ti)) {
      uint64_t ns = (ti.pti_total_user + ti.pti_total_system) * timebase.numer / timebase.denom;
      p->cpuTime = (int64_t)(ns / 1000000);
      p->vsize = ti.pti_virtual_size;
      p->rss = ti.pti_resident_size;
      p->threads = ti.pti_threadnum;
      p->minflt = (uint64_t)ti.pti_faults;
      p->majflt = (uint64_t)ti.pti_pageins;
    }
    if (p->state != 'Z')
      p->args = process_args(pid);
  }
  NQFree(users.items);
  NQFree(procs);
  return ret;
}

uint64_t NQGetPhysicalMemorySize(void)
{
  int mib[2] = { CTL_HW, HW_MEMSIZE };
  uint64_t total = 0;
  size_t size = sizeof(total);
  if (sysctl(mib, 2, &total, &size, NULL, 0) != 0)
    return 0;
  return total;
}

/* --- Windows ------------------------------------------------------------- */

#elif defined(NQ_OS_WINDOWS)

/* ProcessCommandLineInformation: Windows 8.1 and later */
# define PROCESS_COMMAND_LINE_INFORMATION 60
# define STATUS_INFO_LENGTH_MISMATCH_ ((LONG)0xC0000004L)

typedef struct {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR Buffer;
} NtUnicodeString;

typedef LONG (NTAPI *NtQueryInformationProcessFunc)(HANDLE, int, PVOID, ULONG, PULONG);

/* UTF-16 to a new UTF-8 string, NULL if empty or on failure */
static char* utf16_to_utf8(const WCHAR* s, size_t len)
{
  const uint16_t* start = (const uint16_t*)s;
  const uint16_t* end = start + len;
  NQUnicodeInfo info;
  if (len == 0 || !NQCalculateUTF16Info(start, end, &info))
    return NULL;
  char* out = NQMalloc(info.utf8Size + 1);
  if (!out)
    return NULL;
  NQConvertUTF16ToUTF8(start, end, (uint8_t*)out, (uint8_t*)out + info.utf8Size + 1, NULL);
  out[info.utf8Size] = '\0';
  return out;
}

static void utf16_copy(char* dst, size_t n, const WCHAR* s)
{
  char* utf8 = utf16_to_utf8(s, wcslen(s));
  if (utf8)
    copy_string(dst, n, utf8, NQStrlen(utf8));
  else
    dst[0] = '\0';
  NQFree(utf8);
}

static char* command_line(HANDLE h, NtQueryInformationProcessFunc query)
{
  if (!query)
    return NULL;
  ULONG size = 1024;
  for (int tries = 0; tries < 4; tries++) {
    NtUnicodeString* buf = NQMalloc(size);
    if (!buf)
      return NULL;
    ULONG needed = 0;
    LONG status = query(h, PROCESS_COMMAND_LINE_INFORMATION, buf, size, &needed);
    if (status >= 0) {
      char* args = utf16_to_utf8(buf->Buffer, buf->Length / sizeof(WCHAR));
      NQFree(buf);
      return args;
    }
    NQFree(buf);
    if (status != STATUS_INFO_LENGTH_MISMATCH_ || needed <= size)
      return NULL;
    size = needed;
  }
  return NULL;
}

/* The account names found so far, by SID */
typedef struct {
  PSID sid;
  char name[64];
} AccountName;

typedef struct {
  AccountName* items;
  size_t count;
} AccountCache;

static void account_name(AccountCache* cache, HANDLE h, char* out, size_t n)
{
  HANDLE token;
  if (!OpenProcessToken(h, TOKEN_QUERY, &token))
    return;
  union {
    TOKEN_USER user;
    BYTE bytes[256];
  } info;
  DWORD size;
  if (GetTokenInformation(token, TokenUser, &info, sizeof(info), &size)) {
    PSID sid = info.user.User.Sid;
    for (size_t i = 0; i < cache->count; i++) {
      if (EqualSid(cache->items[i].sid, sid)) {
        copy_string(out, n, cache->items[i].name, NQStrlen(cache->items[i].name));
        CloseHandle(token);
        return;
      }
    }
    WCHAR name[256], domain[256];
    DWORD nameLen = NQ_ARRAY_LENGTH(name), domainLen = NQ_ARRAY_LENGTH(domain);
    SID_NAME_USE use;
    char utf8[64] = "";
    if (LookupAccountSidW(NULL, sid, name, &nameLen, domain, &domainLen, &use))
      utf16_copy(utf8, sizeof(utf8), name);
    DWORD sidLen = GetLengthSid(sid);
    PSID copy = NQMalloc(sidLen);
    AccountName* items = copy ? NQRealloc(cache->items, (cache->count + 1) * sizeof(*items)) : NULL;
    if (items && CopySid(sidLen, copy, sid)) {
      cache->items = items;
      items[cache->count].sid = copy;
      NQMemcpy(items[cache->count].name, utf8, sizeof(utf8));
      cache->count++;
    } else {
      if (items)
        cache->items = items;
      NQFree(copy);
    }
    copy_string(out, n, utf8, NQStrlen(utf8));
  }
  CloseHandle(token);
}

/* A priority class as the nice value closest to it */
static int class_nice(LONG base)
{
  if (base <= 4)
    return 19; /* IDLE_PRIORITY_CLASS */
  if (base <= 6)
    return 10; /* BELOW_NORMAL */
  if (base <= 8)
    return 0;  /* NORMAL */
  if (base <= 10)
    return -5; /* ABOVE_NORMAL */
  if (base <= 13)
    return -10; /* HIGH */
  return -20;   /* REALTIME */
}

static int get_process_list(ProcessArray* a)
{
  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snap == INVALID_HANDLE_VALUE)
    return -(int)GetLastError();
  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  NtQueryInformationProcessFunc query =
      ntdll ? (NtQueryInformationProcessFunc)(void (*)(void))GetProcAddress(ntdll, "NtQueryInformationProcess") : NULL;
  AccountCache accounts = { NULL, 0 };

  int ret = 0;
  PROCESSENTRY32W pe;
  pe.dwSize = sizeof(pe);
  for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
    NQProcessInfo* p = array_add(a);
    if (!p) {
      ret = -NQ_ENOMEM;
      break;
    }
    p->pid = pe.th32ProcessID;
    p->ppid = pe.th32ParentProcessID;
    p->pgid = p->pid;
    DWORD session;
    if (ProcessIdToSessionId(pe.th32ProcessID, &session))
      p->sid = session;
    p->threads = (int)pe.cntThreads;
    p->state = 'R';
    p->nice = class_nice(pe.pcPriClassBase);
    p->priority = 20 + p->nice;
    utf16_copy(p->name, sizeof(p->name), pe.szExeFile);

    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
    if (!h)
      h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
    if (!h)
      continue;
    FILETIME created, exited, kernel, user;
    if (GetProcessTimes(h, &created, &exited, &kernel, &user)) {
      p->startTime = NQWinFileTimeToTimeMs(created.dwHighDateTime, created.dwLowDateTime);
      uint64_t k = ((uint64_t)kernel.dwHighDateTime << 32) | kernel.dwLowDateTime;
      uint64_t u = ((uint64_t)user.dwHighDateTime << 32) | user.dwLowDateTime;
      p->cpuTime = (int64_t)((k + u) / 10000);
    }
    PROCESS_MEMORY_COUNTERS_EX mem;
    if (GetProcessMemoryInfo(h, (PROCESS_MEMORY_COUNTERS*)&mem, sizeof(mem))) {
      p->rss = mem.WorkingSetSize;
      p->vsize = mem.PrivateUsage;
      p->minflt = mem.PageFaultCount;
    }
    account_name(&accounts, h, p->user, sizeof(p->user));
    p->args = command_line(h, query);
    CloseHandle(h);
  }
  CloseHandle(snap);
  for (size_t i = 0; i < accounts.count; i++)
    NQFree(accounts.items[i].sid);
  NQFree(accounts.items);
  return ret;
}

uint64_t NQGetPhysicalMemorySize(void)
{
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  return GlobalMemoryStatusEx(&status) ? status.ullTotalPhys : 0;
}

#else

static int get_process_list(ProcessArray* a)
{
  NQ_UNUSED_PARAM(a);
  return -NQ_ENOTSUPP;
}

uint64_t NQGetPhysicalMemorySize(void)
{
  return 0;
}

#endif

int NQGetProcessList(NQProcessInfo** list, size_t* count)
{
  ProcessArray a = { NULL, 0, 0 };
  int ret = get_process_list(&a);
  if (ret != 0) {
    NQFreeProcessList(a.items, a.count);
    *list = NULL;
    *count = 0;
    return ret;
  }
  *list = a.items;
  *count = a.count;
  return 0;
}
