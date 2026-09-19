/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/Context.h"

#include <libnetq/List.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Assert.h>
// Module
#include <libnetq/Module.h>
#include <libnetq/Library.h>
#include <libnetq/fs/Path.h>
#include <libnetq/Mutex.h>
#include <libnetq/Malloc.h>
#include <libnetq/string/Sprintf.h>
#include <libnetq/Executable.h>
#include <libnetq/string/StringUtil.h>

typedef struct NQModuleEntry NQModule;
struct NQModuleEntry {
  NQMutex mutex;
  uint32_t refCount;
  NQListHead list;
  NQLibrary* library;
  NQModuleInitCallback init;
  NQModuleExitCallback exit;
  char path[1];
};

typedef struct NQModuleCollection NQModuleCollection;
struct NQModuleCollection {
  NQMutex mutex;
  NQPath* modulesPath;
  NQListHead modulesList;
};

#define MODULE_NAME_LENGTH 32

// !!TODO: Notifier Mutex
struct NQContext {
  NQModuleCollection modules;
  NQWebNotifierEntry* firstNotifiere;
  NQWebNotifierEntry* lastNotifiere;
};

static NQContext s_context = {
  .modules = {
    .mutex = NQ_MUTEX_INIT(s_context.modules.mutex),
    .modulesPath = NULL,
    .modulesList = NQ_LISTHEAD_INIT(s_context.modules.modulesList),
  },
  .firstNotifiere = NULL,
  .lastNotifiere = NULL,
};

NQContext* NQContext_instance(void)
{
  return &s_context;
}

static struct NQModuleEntry* allocModuleEntry(const char* path)
{
  size_t nlen = NQStrlen(path);
  struct NQModuleEntry* entry = (struct NQModuleEntry*)NQMalloc(sizeof(*entry) + nlen);
  if (entry == NULL)
    return NULL;
  NQMutex_init(&entry->mutex);
  entry->refCount = 1;
  NQListHead_init(&entry->list);
  entry->library = NULL;
  entry->init = NULL;
  entry->exit = NULL;
  memcpy(entry->path, path, nlen + 1);
  return entry;
}

static int openModuleEntry(NQContext* context, struct NQModuleEntry* entry)
{
  entry->library = NQLibraryOpen(entry->path);
  if (entry->library == NULL) {
    return -NQ_EACCES;
  }

  entry->init = (NQModuleInitCallback)NQLibraryGetSymbol(entry->library, NQ_MODULE_INIT_STR);
  entry->exit = (NQModuleExitCallback)NQLibraryGetSymbol(entry->library, NQ_MODULE_EXIT_STR);

  if (entry->init == NULL && entry->exit == NULL) {
    NQ_LOGE("Unable to find " NQ_MODULE_INIT_STR " or " NQ_MODULE_EXIT_STR " in %s", entry->path);
    NQLibraryClose(entry->library);
    entry->library = NULL;
    return -NQ_EACCES;
  }

  if (entry->init != NULL) {
    int ret = entry->init(context);
    if (ret != 0) {
      NQLibraryClose(entry->library);
      entry->library = NULL;
      entry->init = NULL;
      entry->exit = NULL;
      return ret;
    }
  }

  return 0;
}

static void freeModuleEntry(struct NQModuleEntry* entry)
{
  NQMutex_finalize(&entry->mutex);
  NQFree(entry);
}

static struct NQModuleEntry* findModuleEntry(NQModuleCollection* thiz, const char* name)
{
  NQListHead* iter = thiz->modulesList.next;
  while (iter != &thiz->modulesList) {
    struct NQModuleEntry* entry = NQ_CONTAINER_OF(iter, struct NQModuleEntry, list);
    if (!NQStrcmp(entry->path, name))
      return entry;
    iter = iter->next;
  }
  return NULL;
}

int NQContext_loadModule(NQContext* thiz, const char* format, ...)
{
  int ret;
  va_list args;
  struct NQModuleEntry* entry;

  char modulePath[256];

  ret = NQGetExecutablePath(modulePath, sizeof(modulePath));
  if (ret < 0)
    return ret;

  const char* filename = NQGetFilename(modulePath);
  if (filename == NULL)
    return -NQ_EIO;

  size_t pathLength = filename - modulePath;
  if (pathLength + MODULE_NAME_LENGTH + sizeof(NQ_LIBRARY_EXT) > sizeof(modulePath))
    return -NQ_EIO;

  va_start(args, format);
  ret = vsnprintf(modulePath + pathLength, sizeof(modulePath) - pathLength, format, args);
  va_end(args);

  if (ret >= MODULE_NAME_LENGTH - sizeof(NQ_LIBRARY_EXT)) {
    return -NQ_ENAMETOOLONG;
  }

  pathLength += ret;
  if (pathLength + sizeof(NQ_LIBRARY_EXT) >= sizeof(modulePath))
    return -NQ_EIO;

  memcpy(modulePath + pathLength, NQ_LIBRARY_EXT, sizeof(NQ_LIBRARY_EXT));
  NQ_ASSERT(pathLength + sizeof(NQ_LIBRARY_EXT) - 1 == NQStrlen(modulePath));

  NQMutex_lock(&thiz->modules.mutex);
  entry = findModuleEntry(&thiz->modules, modulePath);
  if (entry != NULL)
    entry->refCount++;
  NQMutex_unlock(&thiz->modules.mutex);

  if (entry == NULL) {
    struct NQModuleEntry* newEntry = allocModuleEntry(modulePath);
    if (newEntry == NULL)
      return -NQ_EIO;

    NQMutex_lock(&thiz->modules.mutex);
    entry = findModuleEntry(&thiz->modules, modulePath);
    if (entry != NULL)
      entry->refCount++;
    else
      NQListHead_addBack(&thiz->modules.modulesList, &newEntry->list);
    NQMutex_unlock(&thiz->modules.mutex);

    if (entry == NULL)
      entry = newEntry;
    else
      freeModuleEntry(newEntry);
  }

  ret = 0;
  NQMutex_lock(&entry->mutex);
  if (entry->library == NULL)
    ret = openModuleEntry(thiz, entry);
  NQMutex_unlock(&entry->mutex);

  if (ret != 0) {
    bool requeredExit = false;

    NQMutex_lock(&thiz->modules.mutex);
    if (--entry->refCount == 0) {
      NQListHead_remove(&entry->list);
      requeredExit = true;
    }
    NQMutex_unlock(&thiz->modules.mutex);

    if (requeredExit) {
      if (entry->exit != NULL)
        entry->exit(thiz);
      if (entry->library != NULL)
        NQLibraryClose(entry->library);
      freeModuleEntry(entry);
    }
  }

  return ret;
}

void NQContext_notifyAll(NQContext* thiz, unsigned event, void* data)
{
  NQWebNotifierEntry* iter = thiz->firstNotifiere;
  while (iter != NULL) {
    int ret = iter->callback(iter, event, data);
    NQ_UNUSED_PARAM(ret);
    iter = iter->next;
  }
}

void NQContext_registerNotifier(NQContext* thiz, NQWebNotifierEntry* notifier)
{
  NQ_ASSERT(notifier->next == NULL);
  notifier->next = NULL;
  if (thiz->lastNotifiere == NULL) {
    thiz->firstNotifiere = notifier;
    thiz->lastNotifiere = notifier;
  }
  else {
    thiz->lastNotifiere->next = notifier;
    thiz->lastNotifiere = notifier;
  }
}

void NQContext_unregisterNotifier(NQContext* thiz, NQWebNotifierEntry* notifier)
{
  NQWebNotifierEntry* prev = NULL;
  NQWebNotifierEntry** iter = &thiz->firstNotifiere;
  while ((*iter) != NULL) {
    if ((*iter) == notifier) {
      (*iter) = notifier->next;
      if (notifier->next != NULL) {
        notifier->next = NULL;
        NQ_ASSERT(thiz->lastNotifiere != notifier);
      }
      else {
        NQ_ASSERT(thiz->lastNotifiere == notifier);
        thiz->lastNotifiere = prev;
      }
      return;
    }
    prev = *iter;
    iter = &prev->next;
  }
  NQ_ASSERT_NOT_REACHED();
}

void NQContext_unregisterAllNotifiers(NQContext* thiz)
{
  thiz->lastNotifiere = NULL;
  while (thiz->firstNotifiere != NULL) {
    NQWebNotifierEntry* tmp = thiz->firstNotifiere;
    thiz->firstNotifiere = tmp->next;
    tmp->next = NULL;
  }
}
