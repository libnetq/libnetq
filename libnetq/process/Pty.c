/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/process/Pty.h"

#include <libnetq/Env.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Malloc.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/string/String.h>

#if defined(NQ_OS_DARWIN)
# include <TargetConditionals.h>
#endif

/* iOS has no fork, the browser no processes */
#if defined(NQ_OS_WINDOWS)
# define NQ_PTY_WINDOWS 1
#elif defined(NQ_OS_UNIX) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) && !defined(__EMSCRIPTEN__)
# define NQ_PTY_POSIX 1
#endif

#if defined(NQ_PTY_POSIX) || defined(NQ_PTY_WINDOWS)

# define DEFAULT_COLS 80
# define DEFAULT_ROWS 24

static void default_size(int* cols, int* rows)
{
  if (*cols <= 0)
    *cols = DEFAULT_COLS;
  if (*rows <= 0)
    *rows = DEFAULT_ROWS;
}

#endif

#if defined(NQ_PTY_POSIX)

# include <fcntl.h>
# include <poll.h>
# include <pwd.h>
# include <signal.h>
# include <stdlib.h>
# include <sys/ioctl.h>
# include <sys/resource.h>
# include <sys/syscall.h>
# include <sys/wait.h>
# include <termios.h>
# include <unistd.h>

/* How long a program gets to end on SIGHUP, then again on SIGKILL */
# define CLOSE_WAIT_MS 200
# define CLOSE_STEP_MS 5

/* Descriptors above this are not looked at where they cannot be closed in
 * one call */
# define MAX_CLOSE_FD 65536

struct NQPty {
  int fd;     /* the master side */
  pid_t pid;
  int exited;
  int exitCode;
};

int NQGetDefaultShell(char* out, size_t size)
{
  static const char* const usual[] = {"/bin/bash", "/usr/bin/bash", "/bin/zsh", "/usr/bin/zsh", "/bin/sh"};
  char shell[1024];

  if (NQEnvGet("SHELL", shell, sizeof(shell)) > 0 && access(shell, X_OK) == 0)
    return NQSnprintf(out, size, "%s", shell);

# if defined(HAVE_GETPWUID_R) || defined(NQ_OS_DARWIN)
  struct passwd pw;
  struct passwd* found = NULL;
  char buffer[4096];
  if (getpwuid_r(getuid(), &pw, buffer, sizeof(buffer), &found) == 0 && found && found->pw_shell &&
      access(found->pw_shell, X_OK) == 0)
    return NQSnprintf(out, size, "%s", found->pw_shell);
# endif

  for (size_t i = 0; i < sizeof(usual) / sizeof(usual[0]); i++) {
    if (access(usual[i], X_OK) == 0)
      return NQSnprintf(out, size, "%s", usual[i]);
  }
  return -NQ_ENOENT;
}

static int set_fd_flags(int fd)
{
  int flags = fcntl(fd, F_GETFL);
  if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0 || fcntl(fd, F_SETFD, FD_CLOEXEC) < 0)
    return -errno;
  return 0;
}

/* Everything from `from` up: the app's own descriptors are none of the
 * program's business, and not all of them are close-on-exec */
static void close_from(int from)
{
# if defined(SYS_close_range)
  if (syscall(SYS_close_range, (unsigned)from, ~0u, 0u) == 0)
    return;
# endif
  long most = sysconf(_SC_OPEN_MAX);
  if (most < 0 || most > MAX_CLOSE_FD)
    most = MAX_CLOSE_FD;
  for (int fd = from; fd < most; fd++)
    close(fd);
}

/* Between fork and exec, so only async-signal-safe calls: another thread may
 * have held the allocator's lock at the fork. Reports a failure through
 * `report`, which closes by itself when exec succeeds. */
static void run_child(const char* slave, const struct winsize* size, const char* cwd, char* const* argv, int report)
{
  /* signals the app ignores or blocks stay so across exec */
  static const int reset[] = {SIGPIPE, SIGCHLD, SIGINT, SIGQUIT, SIGTERM, SIGHUP, SIGALRM, SIGTSTP};
  for (size_t i = 0; i < sizeof(reset) / sizeof(reset[0]); i++)
    signal(reset[i], SIG_DFL);
  sigset_t none;
  sigemptyset(&none);
  sigprocmask(SIG_SETMASK, &none, NULL);

  if (setsid() < 0)
    goto failed;
  int fd = open(slave, O_RDWR);
  if (fd < 0 || ioctl(fd, TIOCSCTTY, 0) < 0)
    goto failed;
  ioctl(fd, TIOCSWINSZ, size);

# if defined(IUTF8)
  /* so that line editing in the kernel erases whole characters */
  struct termios tio;
  if (tcgetattr(fd, &tio) == 0) {
    tio.c_iflag |= IUTF8;
    tcsetattr(fd, TCSANOW, &tio);
  }
# endif

  /* `report` may be 0..2, which the slave is about to take */
  report = fcntl(report, F_DUPFD_CLOEXEC, 3);
  if (report < 0 || dup2(fd, 0) < 0 || dup2(fd, 1) < 0 || dup2(fd, 2) < 0)
    goto failed;
  if (dup2(report, 3) < 0 || fcntl(3, F_SETFD, FD_CLOEXEC) < 0)
    goto failed;
  report = 3;
  close_from(4);

  if (cwd && chdir(cwd) != 0) {
    /* the program still runs, from wherever the app is */
  }
  execvp(argv[0], argv);

failed:;
  int ec = errno;
  ssize_t written = write(report, &ec, sizeof(ec));
  NQ_UNUSED_PARAM(written);
  _exit(127);
}

int NQPtySpawn(NQPty** pty, const NQPtyOptions* options)
{
  if (!pty || !options || !options->program)
    return -NQ_EINVAL;
  *pty = NULL;

  size_t argc = 0;
  while (options->args && options->args[argc])
    argc++;

  NQPty* thiz = NQZalloc(sizeof(*thiz));
  char** argv = NQMalloc((argc + 2) * sizeof(*argv));
  if (!thiz || !argv) {
    NQFree(thiz);
    NQFree(argv);
    return -NQ_ENOMEM;
  }
  argv[0] = (char*)options->program;
  for (size_t i = 0; i < argc; i++)
    argv[i + 1] = (char*)options->args[i];
  argv[argc + 1] = NULL;

  int ret;
  int report[2] = {-1, -1};
  int master = posix_openpt(O_RDWR | O_NOCTTY);
  char slave[128];
  if (master < 0 || grantpt(master) != 0 || unlockpt(master) != 0) {
    ret = -errno;
    goto fail;
  }
  ret = ptsname_r(master, slave, sizeof(slave));
  if (ret != 0) {
    ret = -ret;
    goto fail;
  }
  ret = set_fd_flags(master);
  if (ret != 0)
    goto fail;

  struct winsize size;
  NQMemset(&size, 0, sizeof(size));
  int cols = options->cols, rows = options->rows;
  default_size(&cols, &rows);
  size.ws_col = (unsigned short)cols;
  size.ws_row = (unsigned short)rows;

  if (pipe(report) != 0 || fcntl(report[0], F_SETFD, FD_CLOEXEC) < 0 || fcntl(report[1], F_SETFD, FD_CLOEXEC) < 0) {
    ret = -errno;
    goto fail;
  }

  pid_t pid = fork();
  if (pid < 0) {
    ret = -errno;
    goto fail;
  }
  if (pid == 0)
    run_child(slave, &size, options->cwd, argv, report[1]); /* does not return */

  close(report[1]);
  report[1] = -1;

  /* the end of file means exec went through */
  int ec = 0;
  ssize_t got;
  do {
    got = read(report[0], &ec, sizeof(ec));
  } while (got < 0 && errno == EINTR);
  close(report[0]);
  report[0] = -1;
  if (got == (ssize_t)sizeof(ec)) {
    while (waitpid(pid, NULL, 0) < 0 && errno == EINTR) {}
    ret = -ec;
    goto fail;
  }

  thiz->fd = master;
  thiz->pid = pid;
  NQFree(argv);
  *pty = thiz;
  return 0;

fail:
  if (report[0] >= 0)
    close(report[0]);
  if (report[1] >= 0)
    close(report[1]);
  if (master >= 0)
    close(master);
  NQFree(argv);
  NQFree(thiz);
  return ret;
}

int NQPtyRead(NQPty* pty, void* buffer, size_t size, int timeoutMs)
{
  struct pollfd pfd = {pty->fd, POLLIN, 0};
  int ready = poll(&pfd, 1, timeoutMs);
  if (ready == 0 || (ready < 0 && errno == EINTR))
    return -NQ_ETIMEDOUT;
  if (ready < 0)
    return -errno;

  ssize_t n = read(pty->fd, buffer, size);
  if (n >= 0)
    return (int)n;
  if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
    return -NQ_ETIMEDOUT;
  /* Linux's way of saying the last slave was closed */
  return errno == EIO ? 0 : -errno;
}

int NQPtyWrite(NQPty* pty, const void* data, size_t size)
{
  ssize_t n = write(pty->fd, data, size);
  if (n >= 0)
    return (int)n;
  if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
    return 0;
  return -errno;
}

int NQPtyResize(NQPty* pty, int cols, int rows)
{
  struct winsize size;
  NQMemset(&size, 0, sizeof(size));
  default_size(&cols, &rows);
  size.ws_col = (unsigned short)cols;
  size.ws_row = (unsigned short)rows;
  return ioctl(pty->fd, TIOCSWINSZ, &size) == 0 ? 0 : -errno;
}

int64_t NQPtyGetPid(NQPty* pty)
{
  return pty->pid;
}

int NQPtyWait(NQPty* pty, int* exitCode)
{
  if (!pty->exited) {
    int status;
    pid_t pid = waitpid(pty->pid, &status, WNOHANG);
    if (pid < 0)
      return errno == EINTR ? 0 : -errno;
    if (pid == 0)
      return 0;
    if (!WIFEXITED(status) && !WIFSIGNALED(status))
      return 0;
    pty->exited = 1;
    pty->exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
  }
  if (exitCode)
    *exitCode = pty->exitCode;
  return 1;
}

/* Waits a moment for the program to end; returns 1 when it has */
static int wait_ended(NQPty* pty)
{
  for (int waited = 0; waited < CLOSE_WAIT_MS; waited += CLOSE_STEP_MS) {
    if (NQPtyWait(pty, NULL) != 0)
      return 1;
    usleep(CLOSE_STEP_MS * 1000);
  }
  return NQPtyWait(pty, NULL) != 0;
}

void NQPtyClose(NQPty* pty)
{
  if (!pty)
    return;

  close(pty->fd);
  if (NQPtyWait(pty, NULL) == 0) {
    /* it leads a process group of its own, as a session leader does */
    kill(-pty->pid, SIGHUP);
    if (!wait_ended(pty)) {
      kill(-pty->pid, SIGKILL);
      wait_ended(pty);
    }
  }
  NQFree(pty);
}

#elif defined(NQ_PTY_WINDOWS)

# include <windows.h>

/* The pseudo console functions are looked up at run time: they are missing
 * before Windows 10 1809, as are their names in older headers */
# define PSEUDOCONSOLE_ATTRIBUTE 0x00020016

typedef void* PseudoConsole;
typedef HRESULT (WINAPI *CreatePseudoConsoleFunc)(COORD, HANDLE, HANDLE, DWORD, PseudoConsole*);
typedef HRESULT (WINAPI *ResizePseudoConsoleFunc)(PseudoConsole, COORD);
typedef void (WINAPI *ClosePseudoConsoleFunc)(PseudoConsole);

/* How often NQPtyRead looks at the pipe: anonymous pipes cannot be waited
 * on, and the pipe stays open after the program ends */
# define READ_POLL_MS 8

/* The longest command line CreateProcessW takes, and environment variable */
# define MAX_COMMAND 32768

struct NQPty {
  HANDLE process;
  DWORD pid;
  HANDLE input;   /* written to */
  HANDLE output;  /* read from */
  PseudoConsole console;
  ResizePseudoConsoleFunc resize;
  ClosePseudoConsoleFunc close;
};

static FARPROC kernel32_function(const char* name)
{
  HMODULE module = GetModuleHandleW(L"kernel32.dll");
  return module ? GetProcAddress(module, name) : NULL;
}

/* `in` as UTF-16 into a string to free with NQFree, or NULL */
static wchar_t* to_wide(const char* in)
{
  int n = MultiByteToWideChar(CP_UTF8, 0, in, -1, NULL, 0);
  wchar_t* out = n > 0 ? NQMalloc((size_t)n * sizeof(*out)) : NULL;
  if (out)
    MultiByteToWideChar(CP_UTF8, 0, in, -1, out, n);
  return out;
}

static int wide_result(const wchar_t* in, char* out, size_t size)
{
  int n = WideCharToMultiByte(CP_UTF8, 0, in, -1, out, (int)size, NULL, NULL);
  return n > 0 ? n - 1 : -(int)GetLastError();
}

int NQGetDefaultShell(char* out, size_t size)
{
  static const wchar_t* const shells[] = {L"pwsh.exe", L"powershell.exe"};
  wchar_t path[MAX_PATH];

  for (size_t i = 0; i < sizeof(shells) / sizeof(shells[0]); i++) {
    DWORD n = SearchPathW(NULL, shells[i], NULL, MAX_PATH, path, NULL);
    if (n > 0 && n < MAX_PATH)
      return wide_result(path, out, size);
  }
  DWORD n = GetEnvironmentVariableW(L"ComSpec", path, MAX_PATH);
  if (n > 0 && n < MAX_PATH)
    return wide_result(path, out, size);
  return NQSnprintf(out, size, "cmd.exe");
}

/* Appends `arg` to the command line at `line`, quoted the way
 * CommandLineToArgvW reads it. Returns the new length. */
static size_t append_arg(char* line, size_t len, size_t size, const char* arg)
{
  size_t arg_len = NQStrlen(arg);
  /* every character escaped, two quotes, a space and the NUL */
  if (len + 2 * arg_len + 4 > size)
    return (size_t)-1;

  if (len)
    line[len++] = ' ';
  line[len++] = '"';
  size_t slashes = 0;
  for (const char* p = arg; *p; p++) {
    if (*p == '\\') {
      slashes++;
    } else if (*p == '"') {
      for (size_t i = 0; i <= slashes; i++)
        line[len++] = '\\';
      slashes = 0;
    } else {
      slashes = 0;
    }
    line[len++] = *p;
  }
  for (size_t i = 0; i < slashes; i++)
    line[len++] = '\\';
  line[len++] = '"';
  line[len] = '\0';
  return len;
}

static wchar_t* build_command(const NQPtyOptions* options)
{
  char* line = NQMalloc(MAX_COMMAND);
  if (!line)
    return NULL;
  line[0] = '\0';
  size_t len = append_arg(line, 0, MAX_COMMAND, options->program);
  for (size_t i = 0; len != (size_t)-1 && options->args && options->args[i]; i++)
    len = append_arg(line, len, MAX_COMMAND, options->args[i]);
  wchar_t* wide = len != (size_t)-1 ? to_wide(line) : NULL;
  NQFree(line);
  return wide;
}

static void close_if_open(HANDLE* h)
{
  if (*h) {
    CloseHandle(*h);
    *h = NULL;
  }
}

/* Runs the program on `thiz->console` */
static int start_process(NQPty* thiz, const NQPtyOptions* options)
{
  int ret;
  wchar_t* command = build_command(options);
  wchar_t* cwd = options->cwd ? to_wide(options->cwd) : NULL;
  LPPROC_THREAD_ATTRIBUTE_LIST list = NULL;
  SIZE_T list_size = 0;

  InitializeProcThreadAttributeList(NULL, 1, 0, &list_size);
  list = NQMalloc(list_size);
  if (!command || !list || (options->cwd && !cwd)) {
    ret = -NQ_ENOMEM;
    goto done_free;
  }
  if (!InitializeProcThreadAttributeList(list, 1, 0, &list_size)) {
    ret = -(int)GetLastError();
    goto done_free;
  }
  if (!UpdateProcThreadAttribute(list, 0, PSEUDOCONSOLE_ATTRIBUTE, thiz->console, sizeof(thiz->console), NULL, NULL)) {
    ret = -(int)GetLastError();
    goto done_list;
  }

  STARTUPINFOEXW info;
  NQMemset(&info, 0, sizeof(info));
  info.StartupInfo.cb = sizeof(info);
  info.lpAttributeList = list;
  /* without it a redirected standard handle of ours gets past the console */
  info.StartupInfo.dwFlags = STARTF_USESTDHANDLES;

  PROCESS_INFORMATION process;
  NQMemset(&process, 0, sizeof(process));
  if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT,
                      NULL, cwd, &info.StartupInfo, &process)) {
    ret = -(int)GetLastError();
    goto done_list;
  }
  CloseHandle(process.hThread);
  thiz->process = process.hProcess;
  thiz->pid = process.dwProcessId;
  ret = 0;

done_list:
  DeleteProcThreadAttributeList(list);
done_free:
  NQFree(list);
  NQFree(cwd);
  NQFree(command);
  return ret;
}

int NQPtySpawn(NQPty** pty, const NQPtyOptions* options)
{
  if (!pty || !options || !options->program)
    return -NQ_EINVAL;
  *pty = NULL;

  CreatePseudoConsoleFunc create = (CreatePseudoConsoleFunc)(void (*)(void))kernel32_function("CreatePseudoConsole");
  ResizePseudoConsoleFunc resize = (ResizePseudoConsoleFunc)(void (*)(void))kernel32_function("ResizePseudoConsole");
  ClosePseudoConsoleFunc closeConsole = (ClosePseudoConsoleFunc)(void (*)(void))kernel32_function("ClosePseudoConsole");
  if (!create || !resize || !closeConsole)
    return -NQ_ENOTSUPP;

  NQPty* thiz = NQZalloc(sizeof(*thiz));
  if (!thiz)
    return -NQ_ENOMEM;
  thiz->resize = resize;
  thiz->close = closeConsole;

  int ret;
  HANDLE console_in = NULL, console_out = NULL;
  if (!CreatePipe(&console_in, &thiz->input, NULL, 0) || !CreatePipe(&thiz->output, &console_out, NULL, 0)) {
    ret = -(int)GetLastError();
    goto fail;
  }
  /* a full pipe must not hold the caller up */
  DWORD mode = PIPE_NOWAIT;
  if (!SetNamedPipeHandleState(thiz->input, &mode, NULL, NULL)) {
    ret = -(int)GetLastError();
    goto fail;
  }

  int cols = options->cols, rows = options->rows;
  default_size(&cols, &rows);
  COORD size = {(SHORT)cols, (SHORT)rows};
  HRESULT hr = create(size, console_in, console_out, 0, &thiz->console);
  if (FAILED(hr)) {
    ret = -(int)(hr & 0xffff);
    goto fail;
  }

  ret = start_process(thiz, options);
  if (ret != 0)
    goto fail;

  /* the console holds its own copies */
  CloseHandle(console_in);
  CloseHandle(console_out);
  *pty = thiz;
  return 0;

fail:
  if (console_in)
    CloseHandle(console_in);
  if (console_out)
    CloseHandle(console_out);
  close_if_open(&thiz->input);
  close_if_open(&thiz->output);
  if (thiz->console)
    closeConsole(thiz->console);
  NQFree(thiz);
  return ret;
}

int NQPtyRead(NQPty* pty, void* buffer, size_t size, int timeoutMs)
{
  int waited = 0;
  for (;;) {
    DWORD available = 0;
    if (!PeekNamedPipe(pty->output, NULL, 0, NULL, &available, NULL)) {
      DWORD ec = GetLastError();
      return ec == ERROR_BROKEN_PIPE ? 0 : -(int)ec;
    }
    if (available) {
      DWORD got = 0;
      DWORD want = available < size ? available : (DWORD)size;
      if (!ReadFile(pty->output, buffer, want, &got, NULL)) {
        DWORD ec = GetLastError();
        return ec == ERROR_BROKEN_PIPE ? 0 : -(int)ec;
      }
      return (int)got;
    }
    /* the pipe outlives the program, until the console is closed */
    if (WaitForSingleObject(pty->process, 0) != WAIT_TIMEOUT)
      return 0;
    if (waited >= timeoutMs)
      return -NQ_ETIMEDOUT;
    Sleep(READ_POLL_MS);
    waited += READ_POLL_MS;
  }
}

int NQPtyWrite(NQPty* pty, const void* data, size_t size)
{
  DWORD written = 0;
  if (!WriteFile(pty->input, data, (DWORD)size, &written, NULL))
    return -(int)GetLastError();
  return (int)written;
}

int NQPtyResize(NQPty* pty, int cols, int rows)
{
  default_size(&cols, &rows);
  COORD size = {(SHORT)cols, (SHORT)rows};
  HRESULT hr = pty->resize(pty->console, size);
  return FAILED(hr) ? -(int)(hr & 0xffff) : 0;
}

int64_t NQPtyGetPid(NQPty* pty)
{
  return pty->pid;
}

int NQPtyWait(NQPty* pty, int* exitCode)
{
  DWORD wait = WaitForSingleObject(pty->process, 0);
  if (wait == WAIT_TIMEOUT)
    return 0;
  if (wait != WAIT_OBJECT_0)
    return -(int)GetLastError();
  DWORD code = 0;
  if (!GetExitCodeProcess(pty->process, &code))
    return -(int)GetLastError();
  if (exitCode)
    *exitCode = (int)code;
  return 1;
}

void NQPtyClose(NQPty* pty)
{
  if (!pty)
    return;

  /* before Windows 11 24H2 closing the console waits for its output to be
   * read, so the pipes go first */
  close_if_open(&pty->input);
  close_if_open(&pty->output);
  pty->close(pty->console);
  if (WaitForSingleObject(pty->process, 0) == WAIT_TIMEOUT)
    TerminateProcess(pty->process, 1);
  CloseHandle(pty->process);
  NQFree(pty);
}

#else

int NQGetDefaultShell(char* out, size_t size)
{
  NQ_UNUSED_PARAM(out);
  NQ_UNUSED_PARAM(size);
  return -NQ_ENOTSUPP;
}

int NQPtySpawn(NQPty** pty, const NQPtyOptions* options)
{
  NQ_UNUSED_PARAM(pty);
  NQ_UNUSED_PARAM(options);
  return -NQ_ENOTSUPP;
}

int NQPtyRead(NQPty* pty, void* buffer, size_t size, int timeoutMs)
{
  NQ_UNUSED_PARAM(pty);
  NQ_UNUSED_PARAM(buffer);
  NQ_UNUSED_PARAM(size);
  NQ_UNUSED_PARAM(timeoutMs);
  return -NQ_ENOTSUPP;
}

int NQPtyWrite(NQPty* pty, const void* data, size_t size)
{
  NQ_UNUSED_PARAM(pty);
  NQ_UNUSED_PARAM(data);
  NQ_UNUSED_PARAM(size);
  return -NQ_ENOTSUPP;
}

int NQPtyResize(NQPty* pty, int cols, int rows)
{
  NQ_UNUSED_PARAM(pty);
  NQ_UNUSED_PARAM(cols);
  NQ_UNUSED_PARAM(rows);
  return -NQ_ENOTSUPP;
}

int64_t NQPtyGetPid(NQPty* pty)
{
  NQ_UNUSED_PARAM(pty);
  return -1;
}

int NQPtyWait(NQPty* pty, int* exitCode)
{
  NQ_UNUSED_PARAM(pty);
  NQ_UNUSED_PARAM(exitCode);
  return -NQ_ENOTSUPP;
}

void NQPtyClose(NQPty* pty)
{
  NQ_UNUSED_PARAM(pty);
}

#endif
