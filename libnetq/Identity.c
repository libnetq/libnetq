/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Identity.h"

#include <libnetq/ErrorCode.h>
#include <libnetq/Malloc.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/string/String.h>

#if defined(NQ_OS_WINDOWS)
# include <windows.h>
# include <lm.h>
# include <libnetq/UTF.h>
#elif defined(NQ_OS_UNIX)
# include <errno.h>
# include <fcntl.h>
# include <grp.h>
# include <pwd.h>
# include <unistd.h>
# include <sys/types.h>
#endif

#define COPY_NAME(field, text) NQSnprintf(field, sizeof(field), "%s", text)

static int alloc_groups(NQIdentity* identity, size_t count)
{
  /* never zero: a user may have no groups, but the malloc must not look failed */
  identity->groups = (NQAccount*)NQMalloc((count ? count : 1) * sizeof(NQAccount));
  if (identity->groups == NULL)
    return -NQ_ENOMEM;
  NQMemset(identity->groups, 0, (count ? count : 1) * sizeof(NQAccount));
  return 0;
}

#if defined(NQ_OS_UNIX)

/* getpw*_r and getgr*_r want room for their strings; a name-service entry
 * can be far larger than sysconf suggests, so they are retried with more */
#define ENTRY_BUFFER_START 16384
#define ENTRY_BUFFER_MAX (16 * 1024 * 1024)

/* Looks a user up by `name`, or by `uid` without one. Returns 0, -NQ_ENOENT
 * or -NQ_ENOMEM. A failed lookup counts as no entry, as a database that
 * cannot be read says nothing about the user. */
static int find_user(const char* name, uid_t uid, uid_t* foundUid, gid_t* foundGid, char* foundName, size_t n)
{
  for (size_t size = ENTRY_BUFFER_START; size <= ENTRY_BUFFER_MAX; size *= 2) {
    char* buf = (char*)NQMalloc(size);
    if (buf == NULL)
      return -NQ_ENOMEM;
    struct passwd entry;
    struct passwd* found = NULL;
    int ret = name ? getpwnam_r(name, &entry, buf, size, &found) : getpwuid_r(uid, &entry, buf, size, &found);
    if (ret == 0 && found != NULL) {
      *foundUid = found->pw_uid;
      *foundGid = found->pw_gid;
      NQSnprintf(foundName, n, "%s", found->pw_name);
    }
    NQFree(buf);
    if (ret != ERANGE)
      return ret == 0 && found != NULL ? 0 : -NQ_ENOENT;
  }
  return -NQ_ENOENT;
}

static void user_account(uid_t uid, NQAccount* account)
{
  uid_t foundUid;
  gid_t foundGid;
  account->id = (uint32_t)uid;
  if (find_user(NULL, uid, &foundUid, &foundGid, account->name, sizeof(account->name)) != 0)
    account->name[0] = '\0';
}

static void group_account(gid_t gid, NQAccount* account)
{
  account->id = (uint32_t)gid;
  account->name[0] = '\0';
  for (size_t size = ENTRY_BUFFER_START; size <= ENTRY_BUFFER_MAX; size *= 2) {
    char* buf = (char*)NQMalloc(size);
    if (buf == NULL)
      return;
    struct group entry;
    struct group* found = NULL;
    int ret = getgrgid_r(gid, &entry, buf, size, &found);
    if (ret == 0 && found != NULL)
      COPY_NAME(account->name, found->gr_name);
    NQFree(buf);
    if (ret != ERANGE)
      return;
  }
}

static int set_groups(NQIdentity* identity, const gid_t* gids, size_t count)
{
  int ret = alloc_groups(identity, count);
  if (ret != 0)
    return ret;
  for (size_t i = 0; i < count; i++)
    group_account(gids[i], &identity->groups[i]);
  identity->groupCount = count;
  return 0;
}

static int process_identity(NQIdentity* identity)
{
  gid_t egid = getegid();
  user_account(getuid(), &identity->realUser);
  user_account(geteuid(), &identity->effectiveUser);
  group_account(getgid(), &identity->realGroup);
  group_account(egid, &identity->effectiveGroup);

  int max = getgroups(0, NULL);
  if (max < 0)
    return -NQGetLastError();
  gid_t* gids = (gid_t*)NQMalloc(((size_t)max + 1) * sizeof(gid_t));
  if (gids == NULL)
    return -NQ_ENOMEM;
  int count = getgroups(max, gids + 1);
  if (count < 0) {
    int error = NQGetLastError();
    NQFree(gids);
    return -error;
  }

  /* the effective group first, without its copy among the others and
   * without repeats next to each other */
  gids[0] = egid;
  size_t kept = 1;
  for (int i = 1; i <= count; i++) {
    if (gids[i] != egid && gids[i] != gids[kept - 1])
      gids[kept++] = gids[i];
  }
  int ret = set_groups(identity, gids, kept);
  NQFree(gids);
  return ret;
}

/* The groups of a user the way a login would make them, `primary` first */
static int user_groups(NQIdentity* identity, const char* name, gid_t primary)
{
  int count = 16;
  for (;;) {
    gid_t* gids = (gid_t*)NQMalloc((size_t)count * sizeof(gid_t));
    if (gids == NULL)
      return -NQ_ENOMEM;
    int want = count;
# if defined(NQ_OS_DARWIN)
    int ret = getgrouplist(name, (int)primary, (int*)gids, &count);
# else
    int ret = getgrouplist(name, primary, gids, &count);
# endif
    if (ret >= 0) {
      ret = set_groups(identity, gids, (size_t)count);
      NQFree(gids);
      return ret;
    }
    NQFree(gids);
    /* some systems do not say how many there are */
    if (count <= want)
      count = want * 2;
    if (count > 1024 * 1024)
      return -NQ_ENOMEM;
  }
}

/* A user found by `name`, or by `uid` without one. Several names can share
 * a UID: what is shown is the first one, and its primary group decides the
 * list, but the group of the name asked for is the user's own. */
static int user_identity(const char* name, uid_t uid, NQIdentity* identity)
{
  char found[NQ_ACCOUNT_NAME_MAX];
  uid_t foundUid;
  gid_t foundGid;
  int ret = find_user(name, uid, &foundUid, &foundGid, found, sizeof(found));
  if (ret != 0)
    return ret;

  gid_t primary = foundGid;
  uid_t sameUid;
  char shown[NQ_ACCOUNT_NAME_MAX];
  if (find_user(NULL, foundUid, &sameUid, &primary, shown, sizeof(shown)) != 0)
    primary = foundGid;

  user_account(foundUid, &identity->realUser);
  identity->effectiveUser = identity->realUser;
  group_account(foundGid, &identity->realGroup);
  identity->effectiveGroup = identity->realGroup;
  return user_groups(identity, name ? name : found, primary);
}

#elif defined(NQ_OS_WINDOWS)

static void wide_to_name(const WCHAR* wide, char* buffer, size_t n)
{
  const uint16_t* start = (const uint16_t*)wide;
  const uint16_t* end = start + lstrlenW(wide);
  NQUnicodeInfo info;
  buffer[0] = '\0';
  if (!NQCalculateUTF16Info(start, end, &info) || info.utf8Size >= n)
    return;
  NQConvertUTF16ToUTF8(start, end, (uint8_t*)buffer, (uint8_t*)buffer + n, NULL);
  buffer[info.utf8Size] = '\0';
}

/* Returns the UTF-16 copy of `name` for NQFree, NULL when it is not UTF-8 */
static WCHAR* name_to_wide(const char* name)
{
  NQUnicodeInfo info;
  if (!NQConvertUTF8ToUTF16((const uint8_t*)name, NULL, NULL, NULL, &info))
    return NULL;
  WCHAR* wide = (WCHAR*)NQMalloc((info.utf16Size + 1) * sizeof(WCHAR));
  if (wide == NULL)
    return NULL;
  NQConvertUTF8ToUTF16((const uint8_t*)name, (const uint8_t*)(name + info.utf8Size), (uint16_t*)wide,
                       (uint16_t*)wide + info.utf16Size, NULL);
  wide[info.utf16Size] = 0;
  return wide;
}

static void sid_account(PSID sid, NQAccount* account)
{
  WCHAR name[NQ_ACCOUNT_NAME_MAX], domain[NQ_ACCOUNT_NAME_MAX];
  DWORD nameLen = NQ_ARRAY_LENGTH(name), domainLen = NQ_ARRAY_LENGTH(domain);
  SID_NAME_USE use;
  UCHAR parts = *GetSidSubAuthorityCount(sid);
  account->id = parts ? (uint32_t)*GetSidSubAuthority(sid, parts - 1u) : 0;
  account->name[0] = '\0';
  if (LookupAccountSidW(NULL, sid, name, &nameLen, domain, &domainLen, &use))
    wide_to_name(name, account->name, sizeof(account->name));
}

/* Returns a buffer for NQFree with one class of what a token holds */
static void* token_information(HANDLE token, TOKEN_INFORMATION_CLASS what)
{
  DWORD size = 0;
  GetTokenInformation(token, what, NULL, 0, &size);
  if (size == 0)
    return NULL;
  void* info = NQMalloc(size);
  if (info != NULL && !GetTokenInformation(token, what, info, size, &size)) {
    NQFree(info);
    info = NULL;
  }
  return info;
}

static int process_identity(NQIdentity* identity)
{
  HANDLE token;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    return -NQGetLastError();

  TOKEN_USER* user = (TOKEN_USER*)token_information(token, TokenUser);
  TOKEN_PRIMARY_GROUP* primary = (TOKEN_PRIMARY_GROUP*)token_information(token, TokenPrimaryGroup);
  TOKEN_GROUPS* groups = (TOKEN_GROUPS*)token_information(token, TokenGroups);
  int ret = user && primary && groups ? 0 : -NQ_ENOMEM;
  if (ret == 0)
    ret = alloc_groups(identity, (size_t)groups->GroupCount + 1);
  if (ret == 0) {
    sid_account(user->User.Sid, &identity->realUser);
    identity->effectiveUser = identity->realUser;
    sid_account(primary->PrimaryGroup, &identity->realGroup);
    identity->effectiveGroup = identity->realGroup;

    identity->groups[0] = identity->realGroup;
    identity->groupCount = 1;
    for (DWORD i = 0; i < groups->GroupCount; i++) {
      PSID sid = groups->Groups[i].Sid;
      /* the logon session is in the list too, but is no group anyone chose */
      if ((groups->Groups[i].Attributes & SE_GROUP_LOGON_ID) || EqualSid(sid, primary->PrimaryGroup))
        continue;
      sid_account(sid, &identity->groups[identity->groupCount++]);
    }
  }
  NQFree(user);
  NQFree(primary);
  NQFree(groups);
  CloseHandle(token);
  return ret;
}

/* Finds an account: returns a SID for NQFree, or NULL */
static PSID find_sid(const WCHAR* name, SID_NAME_USE* use)
{
  WCHAR domain[NQ_ACCOUNT_NAME_MAX];
  DWORD sidSize = 0, domainLen = NQ_ARRAY_LENGTH(domain);
  LookupAccountNameW(NULL, name, NULL, &sidSize, domain, &domainLen, use);
  if (sidSize == 0)
    return NULL;
  PSID sid = (PSID)NQMalloc(sidSize);
  domainLen = NQ_ARRAY_LENGTH(domain);
  if (sid != NULL && !LookupAccountNameW(NULL, name, sid, &sidSize, domain, &domainLen, use)) {
    NQFree(sid);
    sid = NULL;
  }
  return sid;
}

typedef DWORD(WINAPI* NetUserGetLocalGroupsFn)(LPCWSTR, LPCWSTR, DWORD, DWORD, LPBYTE*, DWORD, LPDWORD, LPDWORD);
typedef DWORD(WINAPI* NetApiBufferFreeFn)(LPVOID);

/* The local groups of a user; none when they cannot be read. netapi32 is
 * loaded for this, so that programs need not link it for a rare question. */
static int local_groups(NQIdentity* identity, const WCHAR* name)
{
  int ret = alloc_groups(identity, 0);
  HMODULE netapi = ret == 0 ? LoadLibraryW(L"netapi32.dll") : NULL;
  if (netapi == NULL)
    return ret;

  NetUserGetLocalGroupsFn get = (NetUserGetLocalGroupsFn)(void*)GetProcAddress(netapi, "NetUserGetLocalGroups");
  NetApiBufferFreeFn release = (NetApiBufferFreeFn)(void*)GetProcAddress(netapi, "NetApiBufferFree");
  LOCALGROUP_USERS_INFO_0* found = NULL;
  DWORD count = 0, total = 0;
  if (get && release
      && get(NULL, name, 0, LG_INCLUDE_INDIRECT, (LPBYTE*)&found, MAX_PREFERRED_LENGTH, &count, &total) == NERR_Success) {
    NQFree(identity->groups);
    identity->groups = NULL;
    ret = alloc_groups(identity, count);
    for (DWORD i = 0; ret == 0 && i < count; i++) {
      SID_NAME_USE use;
      PSID sid = find_sid(found[i].lgrui0_name, &use);
      if (sid == NULL)
        continue;
      sid_account(sid, &identity->groups[identity->groupCount++]);
      NQFree(sid);
    }
  }
  if (found && release)
    release(found);
  FreeLibrary(netapi);
  return ret;
}

/* Is it the account this process runs as? */
static BOOL is_process_user(PSID sid)
{
  HANDLE token;
  BOOL same = FALSE;
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
    TOKEN_USER* user = (TOKEN_USER*)token_information(token, TokenUser);
    same = user && EqualSid(user->User.Sid, sid);
    NQFree(user);
    CloseHandle(token);
  }
  return same;
}

static int named_identity(const char* name, NQIdentity* identity)
{
  WCHAR* wide = name_to_wide(name);
  if (wide == NULL)
    return -NQ_ENOENT;

  SID_NAME_USE use = SidTypeUnknown;
  PSID sid = find_sid(wide, &use);
  int ret;
  if (sid == NULL || use != SidTypeUser) {
    ret = -NQ_ENOENT;
  } else if (is_process_user(sid)) {
    ret = process_identity(identity); /* the token knows more than the database */
  } else {
    sid_account(sid, &identity->realUser);
    identity->effectiveUser = identity->realUser;
    ret = local_groups(identity, wide);
    if (ret == 0 && identity->groupCount > 0) {
      identity->realGroup = identity->groups[0];
      identity->effectiveGroup = identity->realGroup;
    }
  }
  NQFree(sid);
  NQFree(wide);
  return ret;
}

#endif

static int finish(NQIdentity* identity, int ret)
{
  if (ret != 0)
    NQIdentity_free(identity);
  return ret;
}

int NQGetProcessIdentity(NQIdentity* identity)
{
  NQMemset(identity, 0, sizeof(*identity));
#if defined(NQ_OS_UNIX) || defined(NQ_OS_WINDOWS)
  return finish(identity, process_identity(identity));
#else
  return -NQ_ENOTSUPP;
#endif
}

int NQGetUserIdentity(const char* name, NQIdentity* identity)
{
  NQMemset(identity, 0, sizeof(*identity));
#if defined(NQ_OS_UNIX)
  return finish(identity, user_identity(name, 0, identity));
#elif defined(NQ_OS_WINDOWS)
  return finish(identity, named_identity(name, identity));
#else
  NQ_UNUSED_PARAM(name);
  return -NQ_ENOTSUPP;
#endif
}

int NQGetUserIdentityById(uint32_t id, NQIdentity* identity)
{
  NQMemset(identity, 0, sizeof(*identity));
#if defined(NQ_OS_UNIX)
  return finish(identity, user_identity(NULL, (uid_t)id, identity));
#elif defined(NQ_OS_WINDOWS)
  NQ_UNUSED_PARAM(id);
  return -NQ_ENOENT;
#else
  NQ_UNUSED_PARAM(id);
  return -NQ_ENOTSUPP;
#endif
}

void NQIdentity_free(NQIdentity* identity)
{
  NQFree(identity->groups);
  NQMemset(identity, 0, sizeof(*identity));
}

int NQGetSecurityContext(char* buffer, size_t n)
{
  if (n == 0)
    return -NQ_ENAMETOOLONG;
  buffer[0] = '\0';

#if defined(NQ_OS_LINUX)
  /* libselinux's test, without needing the library: selinuxfs is there */
  if (access("/sys/fs/selinux/enforce", F_OK) != 0)
    return -NQ_ENOTSUPP;
  int fd = open("/proc/self/attr/current", O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    return -NQGetLastError();
  ssize_t len = read(fd, buffer, n - 1);
  int error = len < 0 ? NQGetLastError() : 0;
  close(fd);
  if (len < 0)
    return -error;
  while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\0'))
    len--;
  buffer[len] = '\0';
  return (int)len;
#else
  return -NQ_ENOTSUPP;
#endif
}
