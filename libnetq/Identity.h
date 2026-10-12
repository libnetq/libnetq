/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_IDENTITY_H
#define _LIBNETQ_IDENTITY_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NQ_ACCOUNT_NAME_MAX 256

/* A user or a group. Windows has SIDs rather than numbers, so an ID there
 * is the last part of the SID (its RID); names are what identify accounts,
 * as RIDs from different authorities can be equal. */
typedef struct NQAccount {
  uint32_t id;
  char name[NQ_ACCOUNT_NAME_MAX]; /* UTF-8; empty when the ID has no name */
} NQAccount;

/* Who a process runs as, or would run as when logged in as a user.
 * `groups` starts with the primary group; on Windows real and effective
 * are the same. */
typedef struct NQIdentity {
  NQAccount realUser;
  NQAccount effectiveUser;
  NQAccount realGroup;
  NQAccount effectiveGroup;
  NQAccount* groups;
  size_t groupCount;
} NQIdentity;

/* These return 0 and an identity for NQIdentity_free, or the negated error
 * code with `identity` emptied: -NQ_ENOENT when there is no such user.
 * Windows has no lookup by number: NQGetUserIdentityById finds nobody. */
NQ_EXPORT int NQGetProcessIdentity(NQIdentity* identity);
NQ_EXPORT int NQGetUserIdentity(const char* name, NQIdentity* identity);
NQ_EXPORT int NQGetUserIdentityById(uint32_t id, NQIdentity* identity);
NQ_EXPORT void NQIdentity_free(NQIdentity* identity);

/* Copies the SELinux context of the process: returns its length, or the
 * negated error code, -NQ_ENOTSUPP when the system does not have SELinux. */
NQ_EXPORT int NQGetSecurityContext(char* buffer, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* _LIBNETQ_IDENTITY_H */
