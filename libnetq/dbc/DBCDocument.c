/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/dbc/DBCDocument.h"

#include <libnetq/Assert.h>
#include <libnetq/String.h>
#include <libnetq/Log.h>
#include <libnetq/Limits.h>
#include <libnetq/MinMax.h>

typedef void DBCObject;
typedef uint32_t DBCClassId;

typedef struct DBCClass DBCClass;
typedef struct DBCContext DBCContext;
typedef struct DBCString DBCString;
typedef struct DBCArray DBCArray;
typedef struct DBCRanges DBCRanges;
typedef struct DBCSymbols DBCSymbols;
typedef struct DBCAttrDefines DBCAttrDefines;
typedef struct DBCAttrMap DBCAttrMap;
typedef union DBCAttrVariant DBCAttrVariant;

enum {
  kNQDBCDocumentIndex,
  kNQDBCNetNodeIndex,
  kNQDBCMessageIndex,
  kNQDBCSignalIndex,
  kNQDBCEnvVarIndex,
  kNQDBCSigGroupIndex,
  kNQDBCValTableIndex,
  kNQDBCSigTypeIndex,
};

#define DBC_OBJECT_MAX 8
NQ_ALLOW_UNUSED
static const char* s_desc[DBC_OBJECT_MAX] = {
  "Document",
  "Network Node",
  "Message",
  "Signal",
  "Environment Variable",
  "Signal Group",
  "Value Table",
  "Signal Type",
};

#define DBC_ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))

struct DBCString {
  char* characters;
};

static void DBCString_init(DBCContext* context, DBCString* thiz);
static void DBCString_finalize(DBCContext* context, DBCString* thiz);
static bool DBCString_setCharacters(DBCContext* context, DBCString* thiz, const char* characters);

struct DBCSymbols {
  size_t count;
  char** items;
};

static void DBCSymbols_init(DBCContext* context, DBCSymbols* thiz);
static bool DBCSymbols_set(DBCContext* context, DBCSymbols* thiz, const char** symbols, size_t count);
static void DBCSymbols_finalize(DBCContext* context, DBCSymbols* thiz);

#define kDBCArrayStartCapacity (32)
#define kDBCArrayMaxSize (NQ_UINT16_MAX)

struct DBCArray {
  DBCObject** objects;
  uint32_t size;
  uint32_t capacity;
};

static void DBCArray_init(DBCContext*, DBCArray*);
static void DBCArray_finalize(DBCContext*, DBCArray*);
static bool DBCArray_reserve(DBCContext*, DBCArray*, size_t newCapacity);
static bool DBCArray_append(DBCContext*, DBCArray*, DBCObject* object);

union DBCAttrVariant {
  int64_t vInt;
  double vFloat;
  DBCString vString;
};

struct NQDBCAttribute {
  NQDBCAttrProto* proto;
  DBCAttrVariant variant;
};

struct DBCAttrMap {
  NQDBCAttribute* data;
  uint16_t size;
  uint16_t capacity;
};

struct NQDBCAttrProto {
  uint32_t refCount;
  DBCClass* relateClass;

  char* name;
  struct NQDBCAttrValueInfo params;

  bool hasDefaultVariant;
  DBCAttrVariant defaultVariant;
};

static void DBCAttrMap_init(DBCContext*, DBCAttrMap*);
static void DBCAttrMap_finalize(DBCContext*, DBCAttrMap*);

struct DBCAttrDefines {
  NQDBCAttrProto** data;
  uint16_t size;
  uint16_t capacity;
};

static void DBCAttrDefines_init(DBCContext*, DBCAttrDefines*);
static void DBCAttrDefines_finalize(DBCContext*, DBCAttrDefines*);
static NQDBCAttrProto* DBCAttrDefines_find(DBCContext*, const DBCAttrDefines*, const char* name);
static bool DBCAttrDefines_add(DBCContext* context, DBCAttrDefines* thiz, NQDBCAttrProto* attr);
static bool DBCAttrDefines_remove(DBCContext* context, DBCAttrDefines* thiz, NQDBCAttrProto* attr);

struct DBCClass {
  DBCClassId id;
  NQDBCDocument* document;
  DBCContext* context;
};

#define kDBCRangesStartCapacity (8)
#define kDBCRangesMaxSize (64)

struct DBCRanges {
  uint16_t count;
  uint16_t capacity;
  union {
    NQDBCRange rangeInline;
    NQDBCRange* rangePointer;
  };
};

static void DBCRanges_init(DBCContext*, DBCRanges*);
static void DBCRanges_finalize(DBCContext*, DBCRanges*);
static bool DBCRanges_add(DBCContext*, DBCRanges*, uint32_t first, uint32_t second);

struct DBCContext {
  void* allocator;
  NQDBCAllocFn* alloc;
  NQDBCFreeFn* free;
  DBCAttrDefines attrProtos;
  struct DBCClass classes[DBC_OBJECT_MAX];
};

static inline void* DBCAlloc(DBCContext* context, size_t size)
{
  return context->alloc(context->allocator, size);
}

static inline void DBCFree(DBCContext* context, void* ptr)
{
  context->free(context->allocator, ptr);
}

/* DBCString */

void DBCString_init(DBCContext* context, DBCString* thiz)
{
  thiz->characters = NULL;
}

void DBCString_finalize(DBCContext* context, DBCString* thiz)
{
  if (thiz->characters != NULL)
    DBCFree(context, thiz->characters);
}

bool DBCString_setCharacters(DBCContext* context, DBCString* thiz, const char* characters)
{
  char* str;
  if (characters == NULL)
    str = NULL;
  else {
    size_t lenz = NQStrlen(characters) + 1;
    str = DBCAlloc(context, lenz);
    if (str == NULL)
      return false;
    NQMemcpy(str, characters, lenz);
  }

  if (thiz->characters != NULL)
    DBCFree(context, thiz->characters);

  thiz->characters = str;
  return true;
}

/* DBCSymbols */

void DBCSymbols_init(DBCContext* context, DBCSymbols* thiz)
{
  thiz->items = NULL;
  thiz->count = 0;
}

bool DBCSymbols_set(DBCContext* context, DBCSymbols* thiz, const char** symbols, size_t count)
{
  char c;
  size_t i;

  size_t totalInBytes = sizeof(char*);
  for (i = 0; i < count; i++) {
    totalInBytes += sizeof(char*);
    totalInBytes += NQStrlen(symbols[i]) + 1;
  }

  char* dst = (char*)DBCAlloc(context, totalInBytes);
  if (dst == NULL)
    return false;

  if (thiz->items != NULL)
    DBCFree(context, thiz->items);

  thiz->count = count;
  thiz->items = (char**)dst;
  dst += sizeof(char*) * (count + 1);

  for (i = 0; i < count; i++) {
    const char* src = symbols[i];
    thiz->items[i] = dst;
    do {
      c = *dst++ = *src++;
    } while (c != '\0');
  }

  thiz->items[count] = NULL;
  NQ_ASSERT(((char*)thiz->items + totalInBytes) == dst);
  return true;
}

NQ_ALLOW_UNUSED
void DBCSymbols_finalize(DBCContext* context, DBCSymbols* thiz)
{
  if (thiz->items != NULL)
    DBCFree(context, thiz->items);
}

struct DBCObjectBase {
  DBCClassId classId;
  uint32_t refCount;
  const char* name;
  DBCContext* context;
};

struct DBCObjectChild {
  DBCClassId classId;
  uint32_t refCount;
  const char* name;
  DBCContext* context;
  DBCObject* parent;
};

struct DBCObjectAttr {
  DBCClassId classId;
  uint32_t refCount;
  const char* name;
  DBCContext* context;
  DBCObject* parent;
  DBCString comment;
  DBCAttrMap attributes;
};

struct NQDBCValTable {
  struct DBCObjectBase base;

  uint32_t size;
  uint32_t* value;
  char** description;
};

struct NQDBCSigType {
  struct DBCObjectBase base;

  NQDBCSigInfo info;
  double defaultValue;
  NQDBCValTable* valTable;
};

struct NQDBCSigGroup {
  struct DBCObjectChild base;

  uint32_t repetitions;
  DBCArray signals;
};

struct NQDBCNetNode {
  struct DBCObjectAttr base;
};

struct NQDBCMessage {
  struct DBCObjectAttr base;

  uint32_t id;
  uint32_t sizeInBytes;

  DBCArray groups;
  DBCArray signals;
  DBCArray transmitters;

  bool isPseudo;
};

struct NQDBCSignal {
  struct DBCObjectAttr base;

  NQDBCSigValueType valueType;

  struct {
    NQDBCSignal* signal; // "M"
    DBCRanges ranges;  // "m"
  } multiplexer;

  uint32_t startBit;
  NQDBCSigInfo info;
  DBCArray receivers;

  NQDBCValTable* valTable;
  NQDBCSigGroup* sigGroup;
  NQDBCSigType* sigType;
};

struct NQDBCEnvVar {
  struct DBCObjectAttr base;

  NQDBCEnvInfo info;
  uint32_t dataSize;
  NQDBCValTable* valTable;
  DBCArray accessNodes;
};

struct NQDBCDocument {
  struct DBCObjectAttr base;

  DBCString version;
  DBCSymbols symbols;

  bool hasBitTiming;
  NQDBCBitTiming bitTiming;

  DBCArray netNodes;
  DBCArray messages; // DBCAvlTree
  DBCArray envVars;
  DBCArray valTables;
  DBCArray sigTypes;
  
  DBCContext context;
};

static NQDBCNetNode* NQDBCNetNode_create(DBCContext*, const char* name);
static NQDBCMessage* NQDBCMessage_create(DBCContext*, const char* name, uint32_t id, uint32_t sizeInBytes);
static NQDBCSignal* NQDBCSignal_create(DBCContext*, const char* name, uint32_t startBit, const NQDBCSigInfo* info);
static NQDBCEnvVar* NQDBCEnvVar_create(DBCContext*, const char* name, const NQDBCEnvInfo* info);
static NQDBCSigGroup* NQDBCSigGroup_create(DBCContext*, const char* name, uint32_t repetitions);
static NQDBCValTable* NQDBCValTable_create(DBCContext*, const char* name, const NQDBCValDesc* items, size_t size);
static NQDBCSigType* NQDBCSigType_create(DBCContext*, const char* name, const NQDBCSigInfo* info, double defaultValue);

static void NQDBCDocument_deinit(NQDBCDocument*);
static void NQDBCNetNode_deinit(NQDBCNetNode*);
static void NQDBCMessage_deinit(NQDBCMessage*);
static void NQDBCSignal_deinit(NQDBCSignal*);
static void NQDBCEnvVar_deinit(NQDBCEnvVar*);
static void NQDBCSigGroup_deinit(NQDBCSigGroup*);
static void NQDBCValTable_deinit(NQDBCValTable*);
static void NQDBCSigType_deinit(NQDBCSigType*);

/* DBCRanges */

void DBCRanges_init(DBCContext* context, DBCRanges* thiz)
{
  thiz->count = 0;
  thiz->capacity = 0;
  thiz->rangeInline.first = 0;
  thiz->rangeInline.second = 0;
}

void DBCRanges_finalize(DBCContext* context, DBCRanges* thiz)
{
  if (thiz->count > 1)
    DBCFree(context, thiz->rangePointer);
}

bool DBCRanges_add(DBCContext* context, DBCRanges* thiz, uint32_t first, uint32_t second)
{
  if (thiz->count >= kDBCRangesMaxSize)
    return false;

  if (second < first) {
    uint32_t temp = first;
    first = second;
    second = temp;
  }

  if (thiz->capacity == 0) {
    if (thiz->count == 0) {
      thiz->rangeInline.first = first;
      thiz->rangeInline.second = second;
    }
    else {
      NQ_ASSERT(thiz->count == 1);
      NQDBCRange* newPointer = DBCAlloc(context, sizeof(NQDBCRange) * kDBCRangesStartCapacity);
      if (newPointer == NULL)
        return false;
      newPointer[0] = thiz->rangeInline;
      newPointer[1].first = first;
      newPointer[1].second = second;
      thiz->rangePointer = newPointer;
      thiz->capacity = kDBCRangesStartCapacity;
    }
  }
  else {
    if (thiz->capacity <= thiz->count) {
      size_t newCapacity = (size_t)thiz->capacity * 3 / 2;
      if (kDBCRangesMaxSize < newCapacity)
        newCapacity = kDBCArrayMaxSize;

      NQDBCRange* newPointer = DBCAlloc(context, sizeof(NQDBCRange) * newCapacity);
      if (newPointer == NULL)
        return false;

      if (thiz->count > 0)
        NQMemcpy(newPointer, thiz->rangePointer, sizeof(NQDBCRange) * thiz->count);

      DBCFree(context, thiz->rangePointer);
      thiz->rangePointer = newPointer;
      thiz->capacity = newCapacity;
    }
    thiz->rangePointer[thiz->count].first = first;
    thiz->rangePointer[thiz->count].second = second;
  }

  thiz->count++;
  return true;
}

static bool DBCAttrSetInt(NQDBCAttrProto* proto, DBCAttrVariant* variant, int64_t value)
{
  switch (proto->params.type) {
  case kNQDBCAttrValueInt:
    if (proto->params.vInt.first <= value && value <= proto->params.vInt.second) {
      variant->vInt = value;
      return true;
    }
    if (proto->params.vInt.first == 0 && 0 == proto->params.vInt.second) {
      variant->vInt = value;
      return true;
    }
    break;

  case kNQDBCAttrValueHex:
    if (proto->params.vHex.first <= value && value <= proto->params.vHex.second) {
      variant->vInt = value;
      return true;
    }
    if (proto->params.vHex.first == 0 && 0 == proto->params.vHex.second) {
      variant->vInt = value;
      return true;
    }
    break;

  case kNQDBCAttrValueFloat:
    variant->vFloat = (double)value;
    return true;

  case kNQDBCAttrValueEnum:
    if (0 <= value && value < (int64_t)proto->params.vEnum.count) {
      variant->vInt = value;
      return true;
    }
    break;

  default:
    break;
  }

  return false;
}

static bool DBCAttrSetFloat(NQDBCAttrProto* proto, DBCAttrVariant* variant, double value)
{
  switch (proto->params.type) {
  case kNQDBCAttrValueFloat:
    if (proto->params.vFloat.first <= value && value <= proto->params.vFloat.second) {
      variant->vFloat = value;
      return true;
    }
    if (proto->params.vFloat.first == 0.0 && 0.0 == proto->params.vFloat.second) {
      variant->vFloat = value;
      return true;
    }
    break;

  default:
    break;
  }

  return false;
}

static bool DBCAttrSetString(NQDBCAttrProto* proto, DBCAttrVariant* variant, const char* value)
{
  switch (proto->params.type) {
  case kNQDBCAttrValueString:
    return DBCString_setCharacters(proto->relateClass->context, &variant->vString, value);

  case kNQDBCAttrValueEnum:  {
    uint16_t i;
    for (i = 0; i < proto->params.vEnum.count; i++) {
      if (!NQStrcmp(proto->params.vEnum.data[i], value)) {
        variant->vInt = i;
        return true;
      }
    }
    if (*value == '\0') {
      NQ_LOGW("An empty string is treated as 0 %s of '%s' Attribute", NQDBCAttrValueType_toCString(proto->params.type), proto->name);
      variant->vInt = 0;
      return true;
    }
    break;
  }

  default:
    break;
  }

  NQ_LOGE("Can't set '%s' string to %s '%s' Attribute", value, NQDBCAttrValueType_toCString(proto->params.type), proto->name);
  return false;
}

static NQDBCAttrProto* NQDBCAttrProto_create(DBCClass* clazz, const char* name, const struct NQDBCAttrValueInfo* params)
{
  NQDBCAttrProto* thiz;
  const size_t classSize = DBC_ALIGN_UP(sizeof(*thiz), sizeof(void*));

  size_t payloadSize = 0;

  if (params->type == kNQDBCAttrValueEnum) {
    for (size_t i = 0; i < params->vEnum.count; i++) {
      const char* desc = params->vEnum.data[i];
      payloadSize += NQStrlen(desc) + 1;
    }
    payloadSize += params->vEnum.count * sizeof(char*);
  }

  size_t nlenz = NQStrlen(name) + 1;
  thiz = (NQDBCAttrProto*)DBCAlloc(clazz->context, classSize + payloadSize + nlenz);
  if (thiz == NULL)
    return NULL;

  thiz->refCount = 1;
  thiz->relateClass = clazz;

  thiz->params = *params;
  thiz->hasDefaultVariant = false;
  NQMemset(&thiz->defaultVariant, 0, sizeof(thiz->defaultVariant));

  char* str = (char*)thiz + classSize;
  if (params->type == kNQDBCAttrValueEnum) {
    thiz->params.vEnum.data = (const char**)str;
    str += params->vEnum.count * sizeof(char*);

    for (size_t i = 0; i < params->vEnum.count; i++) {
      const char* desc = params->vEnum.data[i];
      size_t nz = NQStrlen(desc) + 1;
      thiz->params.vEnum.data[i] = str;
      NQMemcpy(str, desc, nz);
      str += nz;
    }
  }

  NQMemcpy(str, name, nlenz);
  thiz->name = str;
  str += nlenz;

  NQ_ASSERT(str == ((char*)thiz + classSize + payloadSize + nlenz));

  return thiz;
}

static void NQDBCAttrProto_retain(DBCContext* context, NQDBCAttrProto* thiz)
{
  thiz->refCount++;
}

static void NQDBCAttrProto_release(DBCContext* context, NQDBCAttrProto* thiz)
{
  if (--thiz->refCount == 0) {
    if (thiz->params.type == kNQDBCAttrValueString)
      DBCString_finalize(context, &thiz->defaultVariant.vString);
    DBCFree(context, thiz);
  }
}

bool NQDBCAttrProto_isRelateDocument(const NQDBCAttrProto* thiz)
{
  return thiz->relateClass->id == kNQDBCDocumentIndex;
}

bool NQDBCAttrProto_isRelateNetNode(const NQDBCAttrProto* thiz)
{
  return thiz->relateClass->id == kNQDBCNetNodeIndex;
}

bool NQDBCAttrProto_isRelateMessage(const NQDBCAttrProto* thiz)
{
  return thiz->relateClass->id == kNQDBCMessageIndex;
}

bool NQDBCAttrProto_isRelateSignal(const NQDBCAttrProto* thiz)
{
  return thiz->relateClass->id == kNQDBCSignalIndex;
}

bool NQDBCAttrProto_isRelateEnvVar(const NQDBCAttrProto* thiz)
{
  return thiz->relateClass->id == kNQDBCEnvVarIndex;
}

bool NQDBCAttrProto_hasDefault(const NQDBCAttrProto* thiz)
{
  return thiz->hasDefaultVariant;
}

bool NQDBCAttrProto_setDefaultInt(NQDBCAttrProto* thiz, int64_t value)
{
  if (!DBCAttrSetInt(thiz, &thiz->defaultVariant, value))
    return false;
  thiz->hasDefaultVariant = true;
  return true;
}

bool NQDBCAttrProto_setDefaultFloat(NQDBCAttrProto* thiz, double value)
{
  if (!DBCAttrSetFloat(thiz, &thiz->defaultVariant, value))
    return false;
  thiz->hasDefaultVariant = true;
  return true;
}

bool NQDBCAttrProto_setDefaultString(NQDBCAttrProto* thiz, const char* value)
{
  if (!DBCAttrSetString(thiz, &thiz->defaultVariant, value))
    return false;
  thiz->hasDefaultVariant = true;
  return true;
}

static void NQDBCAttribute_init(DBCContext* context, NQDBCAttribute* thiz, NQDBCAttrProto* proto)
{
  thiz->proto = proto;
  NQDBCAttrProto_retain(context, proto);

  switch (proto->params.type) {
  case kNQDBCAttrValueInt:
  case kNQDBCAttrValueHex:
  case kNQDBCAttrValueEnum:
    thiz->variant.vInt = 0;
    break;
  case kNQDBCAttrValueFloat:
    thiz->variant.vFloat = 0;
    break;
  case kNQDBCAttrValueString:
    DBCString_init(context, &thiz->variant.vString);
    break;
  default:
    NQ_ASSERT_NOT_REACHED();
    break;
  }
}

static void NQDBCAttribute_finalize(DBCContext* context, NQDBCAttribute* thiz)
{
  if (thiz->proto->params.type == kNQDBCAttrValueString)
    DBCString_finalize(context, &thiz->variant.vString);
  NQDBCAttrProto_release(context, thiz->proto);
}

NQDBCAttrValueType NQDBCAttribute_type(const NQDBCAttribute* thiz)
{
  return thiz->proto->params.type;
}

const char* NQDBCAttribute_name(const NQDBCAttribute* thiz)
{
  return thiz->proto->name;
}

int64_t NQDBCAttribute_asIntValue(const NQDBCAttribute* thiz)
{
  switch (thiz->proto->params.type) {
  case kNQDBCAttrValueInt:
  case kNQDBCAttrValueHex:
    return thiz->variant.vInt;
  default:
    NQ_ASSERT_NOT_REACHED();
  }
  return 0;
}

double NQDBCAttribute_asFloatValue(const NQDBCAttribute* thiz)
{
  NQ_ASSERT(thiz->proto->params.type == kNQDBCAttrValueFloat);
  return thiz->variant.vFloat;
}

const char* NQDBCAttribute_asStringValue(const NQDBCAttribute* thiz)
{
  switch (thiz->proto->params.type) {
  case kNQDBCAttrValueString:
    return thiz->variant.vString.characters;
  case kNQDBCAttrValueEnum:
    return thiz->proto->params.vEnum.data[thiz->variant.vInt];
  default:
    NQ_ASSERT_NOT_REACHED();
  }
  return NQCStrEmpty();
}

void DBCAttrDefines_init(DBCContext* context, DBCAttrDefines* thiz)
{
  thiz->data = NULL;
  thiz->size = 0;
  thiz->capacity = 0;
}

void DBCAttrDefines_finalize(DBCContext* context, DBCAttrDefines* thiz)
{
  uint16_t i;
  for (i = 0; i < thiz->size; i++) {
    NQDBCAttrProto_release(context, thiz->data[i]);
  }
  DBCFree(context, thiz->data);
}

NQDBCAttrProto* DBCAttrDefines_find(DBCContext* context, const DBCAttrDefines* thiz, const char* name)
{
  uint16_t i;
  for (i = 0; i < thiz->size; i++) {
    if (!NQStrcmp(thiz->data[i]->name, name))
      return thiz->data[i];
  }
  return NULL;
}

static bool DBCAttrDefines_reserve(DBCContext* context, DBCAttrDefines* thiz, size_t newCapacity)
{
  if (kDBCArrayMaxSize < newCapacity)
    return false;

  if (thiz->capacity < newCapacity) {
    NQDBCAttrProto** newData = (NQDBCAttrProto**)DBCAlloc(context, newCapacity * sizeof(*thiz->data));
    if (newData == NULL)
      return false;

    if (thiz->size != 0) {
      NQMemcpy(newData, thiz->data, thiz->size * sizeof(*thiz->data));
      DBCFree(context, thiz->data);
    }

    thiz->data = newData;
    thiz->capacity = (uint16_t)newCapacity;
  }

  return true;
}

bool DBCAttrDefines_add(DBCContext* context, DBCAttrDefines* thiz, NQDBCAttrProto* attr)
{
  if (thiz->capacity <= thiz->size) {
    if (kDBCArrayMaxSize <= thiz->capacity)
      return false;

    size_t newCapacity;
    if (thiz->capacity == 0)
      newCapacity = kDBCArrayStartCapacity;
    else
      newCapacity = NQGetMin((size_t)thiz->capacity * 3 / 2, kDBCArrayMaxSize);

    if (!DBCAttrDefines_reserve(context, thiz, (uint16_t)newCapacity))
      return false;
  }

  NQDBCAttrProto_retain(context, attr);
  thiz->data[thiz->size++] = attr;

  return true;
}

NQ_ALLOW_UNUSED
bool DBCAttrDefines_remove(DBCContext* context, DBCAttrDefines* thiz, NQDBCAttrProto* attr)
{
  for (uint16_t i = 0; i < thiz->size; i++) {
    if (thiz->data[i] == attr) {
      NQDBCAttrProto_release(context, attr);
      while ((i + 1) < thiz->size) {
        thiz->data[i] = thiz->data[i + 1];
        i++;
      }
      thiz->size--;
      return true;
    }
  }
  return false;
}

void DBCAttrMap_init(DBCContext* context, DBCAttrMap* thiz)
{
  thiz->data = NULL;
  thiz->size = 0;
  thiz->capacity = 0;
}

void DBCAttrMap_finalize(DBCContext* context, DBCAttrMap* thiz)
{
  uint16_t i;
  for (i = 0; i < thiz->size; i++) {
    NQDBCAttribute_finalize(context, &thiz->data[i]);
  }
  DBCFree(context, thiz->data);
}

static bool DBCAttrMap_reserve(DBCContext* context, DBCAttrMap* thiz, size_t newCapacity)
{
  if (kDBCArrayMaxSize < newCapacity)
    return false;

  if (thiz->capacity < newCapacity) {
    NQDBCAttribute* newData = (NQDBCAttribute*)DBCAlloc(context, newCapacity * sizeof(*thiz->data));
    if (newData == NULL)
      return false;

    if (thiz->size != 0) {
      NQMemcpy(newData, thiz->data, thiz->size * sizeof(*thiz->data));
      DBCFree(context, thiz->data);
    }

    thiz->data = newData;
    thiz->capacity = (uint16_t)newCapacity;
  }

  return true;
}

static bool DBCAttrMap_add(DBCContext* context, DBCAttrMap* thiz, NQDBCAttribute* attr)
{
  if (thiz->capacity <= thiz->size) {
    if (kDBCArrayMaxSize <= thiz->capacity) {
      return false;
    }

    size_t newCapacity;
    if (thiz->capacity == 0)
      newCapacity = kDBCArrayStartCapacity;
    else
      newCapacity = NQGetMin((size_t)thiz->capacity * 3 / 2, kDBCArrayMaxSize);

    if (!DBCAttrMap_reserve(context, thiz, (uint16_t)newCapacity)) {
      return false;
    }
  }

  NQMemcpy(&thiz->data[thiz->size], attr, sizeof(*attr));
  thiz->size++;
  return true;
}

static NQDBCAttribute* DBCAttrMap_find(DBCContext* context, const DBCAttrMap* thiz, const char* name)
{
  uint16_t i;
  for (i = 0; i < thiz->size; i++) {
    if (!NQStrcmp(thiz->data[i].proto->name, name))
      return &thiz->data[i];
  }
  return NULL;
}

static inline void DBCObjectBase_init(DBCContext* context, struct DBCObjectBase* thiz, DBCClassId id)
{
  thiz->name = NULL;
  thiz->classId = id;
  thiz->refCount = 1;
  thiz->context = context;
}

static inline void DBCObjectChild_init(DBCContext* context, struct DBCObjectChild* thiz, DBCClassId id)
{
  DBCObjectBase_init(context, (struct DBCObjectBase*)thiz, id);
  thiz->parent = NULL;
}

static inline void DBCObjectAttr_init(DBCContext* context, struct DBCObjectAttr* thiz, DBCClassId id)
{
  DBCObjectChild_init(context, (struct DBCObjectChild*)thiz, id);
  DBCString_init(context, &thiz->comment);
  DBCAttrMap_init(context, &thiz->attributes);
}

static void DBCObjectAttr_finalize(DBCContext* context, struct DBCObjectAttr* thiz)
{
  DBCString_finalize(context, &thiz->comment);
  DBCAttrMap_finalize(context, &thiz->attributes);
}

static bool DBCObjectAttr_getAttrInt(const struct DBCObjectAttr* thiz, const char* name, int64_t* result)
{
  NQDBCAttrProto* proto;
  DBCAttrVariant* variant;

  NQDBCAttribute* attr = DBCAttrMap_find(thiz->context, &thiz->attributes, name);
  if (attr != NULL) {
    proto = attr->proto;
    variant = &attr->variant;
  }
  else {
    proto = DBCAttrDefines_find(thiz->context, &thiz->context->attrProtos, name);
    variant = (proto != NULL && proto->hasDefaultVariant && proto->relateClass->id == thiz->classId) ? &proto->defaultVariant : NULL;
  }

  bool success = false;
  int64_t value = 0;

  if (variant != NULL) {
    switch (proto->params.type) {
    case kNQDBCAttrValueInt:
    case kNQDBCAttrValueHex:
      success = true;
      value = variant->vInt;
      break;
    }
  }

  if (success && result != NULL)
    *result = value;

  return success;
}

static bool DBCObjectAttr_setAttrInt(struct DBCObjectAttr* thiz, const char* name, int64_t value)
{
  NQDBCAttribute* attr = DBCAttrMap_find(thiz->context, &thiz->attributes, name);
  if (attr != NULL) {
    return DBCAttrSetInt(attr->proto, &attr->variant, value);
  }

  NQDBCAttrProto* proto = DBCAttrDefines_find(thiz->context, &thiz->context->attrProtos, name);
  if (proto == NULL)
    return false;

  if (thiz->classId != proto->relateClass->id)
    return false;

  NQDBCAttribute temp;
  NQDBCAttribute_init(thiz->context, &temp, proto);
  if (!DBCAttrSetInt(temp.proto, &temp.variant, value)) {
    NQDBCAttribute_finalize(thiz->context, &temp);
    return false;
  }

  return DBCAttrMap_add(thiz->context, &thiz->attributes, &temp);
}

static bool DBCObjectAttr_getAttrFloat(const struct DBCObjectAttr* thiz, const char* name, double* result)
{
  NQDBCAttrProto* proto;
  DBCAttrVariant* variant;

  NQDBCAttribute* attr = DBCAttrMap_find(thiz->context, &thiz->attributes, name);
  if (attr != NULL) {
    proto = attr->proto;
    variant = &attr->variant;
  }
  else {
    proto = DBCAttrDefines_find(thiz->context, &thiz->context->attrProtos, name);
    variant = (proto != NULL && proto->hasDefaultVariant && proto->relateClass->id == thiz->classId) ? &proto->defaultVariant : NULL;
  }

  bool success = false;
  double value = 0;

  if (variant != NULL) {
    switch (proto->params.type) {
    case kNQDBCAttrValueInt:
    case kNQDBCAttrValueHex:
    case kNQDBCAttrValueEnum:
      success = true;
      value = (double)variant->vInt;
      break;
    case kNQDBCAttrValueFloat:
      success = true;
      value = variant->vFloat;
      break;
    }
  }

  if (success && result != NULL)
    *result = value;

  return success;
}

static bool DBCObjectAttr_setAttrFloat(struct DBCObjectAttr* thiz, const char* name, double value)
{
  NQDBCAttribute* attr = DBCAttrMap_find(thiz->context, &thiz->attributes, name);
  if (attr != NULL) {
    return DBCAttrSetFloat(attr->proto, &attr->variant, value);
  }

  NQDBCAttrProto* proto = DBCAttrDefines_find(thiz->context, &thiz->context->attrProtos, name);
  if (proto == NULL)
    return false;

  if (thiz->classId != proto->relateClass->id)
    return false;

  NQDBCAttribute temp;
  NQDBCAttribute_init(thiz->context, &temp, proto);
  if (!DBCAttrSetFloat(temp.proto, &temp.variant, value)) {
    NQDBCAttribute_finalize(thiz->context, &temp);
    return false;
  }

  return DBCAttrMap_add(thiz->context, &thiz->attributes, &temp);
}

static bool DBCObjectAttr_getAttrString(const struct DBCObjectAttr* thiz, const char* name, const char** result)
{
  NQDBCAttrProto* proto;
  DBCAttrVariant* variant;

  NQDBCAttribute* attr = DBCAttrMap_find(thiz->context, &thiz->attributes, name);
  if (attr != NULL) {
    proto = attr->proto;
    variant = &attr->variant;
  }
  else {
    proto = DBCAttrDefines_find(thiz->context, &thiz->context->attrProtos, name);
    variant = (proto != NULL && proto->hasDefaultVariant && proto->relateClass->id == thiz->classId) ? &proto->defaultVariant : NULL;
  }

  bool success = false;
  const char* value = NQCStrEmpty();

  if (variant != NULL) {
    switch (proto->params.type) {
    case kNQDBCAttrValueEnum:
      success = true;
      value = proto->params.vEnum.data[variant->vInt];
      break;
    case kNQDBCAttrValueString:
      success = true;
      value = variant->vString.characters;
      break;
    }
  }

  if (success && result != NULL)
    *result = value;

  return success;
}

static bool DBCObjectAttr_setAttrString(struct DBCObjectAttr* thiz, const char* name, const char* value)
{
  NQDBCAttribute* attr = DBCAttrMap_find(thiz->context, &thiz->attributes, name);
  if (attr != NULL) {
    return DBCAttrSetString(attr->proto, &attr->variant, value);
  }

  NQDBCAttrProto* proto = DBCAttrDefines_find(thiz->context, &thiz->context->attrProtos, name);
  if (proto == NULL)
    return false;

  if (thiz->classId != proto->relateClass->id)
    return false;

  NQDBCAttribute temp;
  NQDBCAttribute_init(thiz->context, &temp, proto);
  if (!DBCAttrSetString(temp.proto, &temp.variant, value)) {
    NQDBCAttribute_finalize(thiz->context, &temp);
    return false;
  }

  return DBCAttrMap_add(thiz->context, &thiz->attributes, &temp);
}

static inline void DBCObject_retain(DBCObject* o)
{
  ((struct DBCObjectBase*)o)->refCount++;
}

static inline void DBCObject_release(DBCObject* o)
{
  struct DBCObjectAttr* object = (struct DBCObjectAttr*)o;

  if (--object->refCount != 0)
    return;

  DBCContext* context = object->context;
  void* allocator = context->allocator;
  NQDBCFreeFn* freeFn = context->free;

  switch (object->classId) {
  case kNQDBCDocumentIndex:
    NQDBCDocument_deinit((NQDBCDocument*)o);
    break;

  case kNQDBCNetNodeIndex:
    NQDBCNetNode_deinit((NQDBCNetNode*)o);
    break;

  case kNQDBCMessageIndex:
    NQDBCMessage_deinit((NQDBCMessage*)o);
    break;

  case kNQDBCSignalIndex:
    NQDBCSignal_deinit((NQDBCSignal*)o);
    break;

  case kNQDBCEnvVarIndex:
    NQDBCEnvVar_deinit((NQDBCEnvVar*)o);
    break;

  case kNQDBCSigGroupIndex:
    NQDBCSigGroup_deinit((NQDBCSigGroup*)o);
    break;

  case kNQDBCValTableIndex:
    NQDBCValTable_deinit((NQDBCValTable*)o);
    break;

  case kNQDBCSigTypeIndex:
    NQDBCSigType_deinit((NQDBCSigType*)o);
    break;

  default:
    NQ_ASSERT_NOT_REACHED();
    break;
  }

  freeFn(allocator, o);
}

static inline NQDBCNetNode* toNQDBCNetNode(DBCObject* o)
{
  NQ_ASSERT(((struct DBCObjectBase*)o)->classId == kNQDBCNetNodeIndex);
  return (NQDBCNetNode*)o;
}

static inline NQDBCMessage* toNQDBCMessage(DBCObject* o)
{
  NQ_ASSERT(((struct DBCObjectBase*)o)->classId == kNQDBCMessageIndex);
  return (NQDBCMessage*)o;
}

static inline NQDBCSignal* toNQDBCSignal(DBCObject* o)
{
  NQ_ASSERT(((struct DBCObjectBase*)o)->classId == kNQDBCSignalIndex);
  return (NQDBCSignal*)o;
}

static inline NQDBCEnvVar* toNQDBCEnvVar(DBCObject* o)
{
  NQ_ASSERT(((struct DBCObjectBase*)o)->classId == kNQDBCEnvVarIndex);
  return (NQDBCEnvVar*)o;
}

static inline NQDBCSigType* toNQDBCSigType(DBCObject* o)
{
  NQ_ASSERT(((struct DBCObjectBase*)o)->classId == kNQDBCSigTypeIndex);
  return (NQDBCSigType*)o;
}

static inline NQDBCSigGroup* toNQDBCSigGroup(DBCObject* o)
{
  NQ_ASSERT(((struct DBCObjectBase*)o)->classId == kNQDBCSigGroupIndex);
  return (NQDBCSigGroup*)o;
}

static inline NQDBCValTable* toNQDBCValTable(DBCObject* o)
{
  NQ_ASSERT(((struct DBCObjectBase*)o)->classId == kNQDBCValTableIndex);
  return (NQDBCValTable*)o;
}

void DBCArray_init(DBCContext* context, DBCArray* thiz)
{
  thiz->objects = NULL;
  thiz->size = 0;
  thiz->capacity = 0;
}

void DBCArray_finalize(DBCContext* context, DBCArray* thiz)
{
  if (thiz->objects != NULL) {
    size_t index;
    for (index = 0; index < thiz->size; index++)
      DBCObject_release(thiz->objects[index]);
    DBCFree(context, thiz->objects);
  }
}

bool DBCArray_reserve(DBCContext* context, DBCArray* thiz, size_t newCapacity)
{
  if (kDBCArrayMaxSize < newCapacity)
    return false;

  if (thiz->capacity < newCapacity) {
    DBCObject** newObjects = (DBCObject**)DBCAlloc(context, newCapacity * sizeof(*thiz->objects));
    if (newObjects == NULL)
      return false;

    if (thiz->size != 0) {
      NQMemcpy(newObjects, thiz->objects, thiz->size * sizeof(*thiz->objects));
      DBCFree(context, thiz->objects);
    }

    thiz->objects = newObjects;
    thiz->capacity = (uint32_t)newCapacity;
  }

  return true;
}

static inline NQDBCNetNode* DBCArray_at(DBCContext* context, DBCArray* thiz, size_t index)
{
  NQ_ASSERT(index < thiz->size);
  return thiz->objects[index];
}

bool DBCArray_append(DBCContext* context, DBCArray* thiz, DBCObject* object)
{
  if (thiz->capacity <= thiz->size) {
    if (kDBCArrayMaxSize == thiz->capacity)
      return false;

    uint32_t newCapacity;
    if (thiz->capacity == 0)
      newCapacity = kDBCArrayStartCapacity;
    else
      newCapacity = (uint32_t)((thiz->capacity * 3) / 2);

    if (newCapacity < thiz->capacity)
      newCapacity = kDBCArrayMaxSize;

    if (!DBCArray_reserve(context, thiz, newCapacity))
      return false;
  }

  thiz->objects[thiz->size++] = object;
  DBCObject_retain(object);
  return true;
}

NQDBCDocument* NQDBCDocument_create(const char* name)
{
  return NQDBCDocument_create2(name, NULL, &NQDBCAllocDefault, &NQDBCFreeDefault);
}

NQDBCDocument* NQDBCDocument_create2(const char* name, void* allocator, NQDBCAllocFn* alloc, NQDBCFreeFn* free)
{
  if (alloc == NULL) {
    return NULL;
  }
  if (free == NULL) {
    free = &NQDBCFreeNope;
  }

  NQ_ASSERT(alloc != NULL && free != NULL);

  size_t lenz = NQStrlen(name) + 1;
  NQDBCDocument* thiz = (NQDBCDocument*)alloc(allocator, sizeof(struct NQDBCDocument) + lenz);
  if (thiz == NULL)
    return NULL;

  DBCClassId id;
  DBCClass* clazz;
  DBCContext* context = &thiz->context;

  context->allocator = allocator;
  context->alloc = alloc;
  context->free = free;

  for (id = 0; id < DBC_OBJECT_MAX; id++) {
    clazz = &context->classes[id];
    clazz->id = id;
    clazz->document = thiz;
    clazz->context = context;
    // DBCAttrDefines_init(context, &clazz->attrDefines);
  }

  clazz = &context->classes[kNQDBCDocumentIndex];

  DBCObjectAttr_init(context, &thiz->base, clazz->id);

  char* str = (char*)thiz + sizeof(struct NQDBCDocument);
  NQMemcpy(str, name, lenz);
  thiz->base.name = str;

  DBCString_init(context, &thiz->version);
  DBCSymbols_init(context, &thiz->symbols);

  thiz->hasBitTiming = false;
  thiz->bitTiming.baudrate = 0;
  thiz->bitTiming.btr1 = 0;
  thiz->bitTiming.btr2 = 0;

  DBCAttrDefines_init(context, &thiz->context.attrProtos);

  DBCArray_init(context, &thiz->netNodes);
  DBCArray_init(context, &thiz->messages);
  DBCArray_init(context, &thiz->envVars);
  DBCArray_init(context, &thiz->valTables);
  DBCArray_init(context, &thiz->sigTypes);

  return thiz;
}

static void NQDBCDocument_deinit(NQDBCDocument* thiz)
{
  DBCContext* context = &thiz->context;

  DBCArray_finalize(context, &thiz->sigTypes);
  DBCArray_finalize(context, &thiz->valTables);
  DBCArray_finalize(context, &thiz->envVars);
  DBCArray_finalize(context, &thiz->messages);
  DBCArray_finalize(context, &thiz->netNodes);

  DBCAttrDefines_finalize(context, &thiz->context.attrProtos);

  DBCSymbols_finalize(context, &thiz->symbols);
  DBCString_finalize(context, &thiz->version);

  DBCObjectAttr_finalize(context, &thiz->base);
}

void NQDBCDocument_release(NQDBCDocument* thiz)
{
  DBCObject_release(thiz);
}

const char* NQDBCDocument_name(const NQDBCDocument* thiz)
{
  return thiz->base.name;
}

const char* NQDBCDocument_comment(const NQDBCDocument* thiz)
{
  return thiz->base.comment.characters;
}

bool NQDBCDocument_setComment(NQDBCDocument* thiz, const char* comment)
{
  return DBCString_setCharacters(thiz->base.context, &thiz->base.comment, comment);
}

const NQDBCBitTiming* NQDBCDocument_bitTiming(const NQDBCDocument* thiz)
{
  return thiz->hasBitTiming ? &thiz->bitTiming : NULL;
}

NQDBCMessage* NQDBCDocument_messageAt(const NQDBCDocument* thiz, size_t index)
{
  NQ_ASSERT(index < thiz->messages.size);
  return toNQDBCMessage(thiz->messages.objects[index]);
}

size_t NQDBCDocument_messageCount(const NQDBCDocument* thiz)
{
  return thiz->messages.size;
}

NQDBCAttribute* NQDBCDocument_findAttribute(const NQDBCDocument* thiz, const char* name)
{
  return DBCAttrMap_find(thiz->base.context, &thiz->base.attributes, name);
}

size_t NQDBCDocument_attributeCount(const NQDBCDocument* thiz)
{
  return thiz->base.attributes.size;
}

NQDBCAttribute* NQDBCDocument_attributeAt(const NQDBCDocument* thiz, size_t index)
{
  return &thiz->base.attributes.data[index];
}

bool NQDBCDocument_getAttrInt(const NQDBCDocument* thiz, const char* name, int64_t* result)
{
  return DBCObjectAttr_getAttrInt(&thiz->base, name, result);
}

bool NQDBCDocument_setAttrInt(NQDBCDocument* thiz, const char* name, int64_t value)
{
  return DBCObjectAttr_setAttrInt(&thiz->base, name, value);
}

bool NQDBCDocument_getAttrFloat(const NQDBCDocument* thiz, const char* name, double* result)
{
  return DBCObjectAttr_getAttrFloat(&thiz->base, name, result);
}

bool NQDBCDocument_setAttrFloat(NQDBCDocument* thiz, const char* name, double value)
{
  return DBCObjectAttr_setAttrFloat(&thiz->base, name, value);
}

bool NQDBCDocument_getAttrString(const NQDBCDocument* thiz, const char* name, const char** result)
{
  return DBCObjectAttr_getAttrString(&thiz->base, name, result);
}

bool NQDBCDocument_setAttrString(NQDBCDocument* thiz, const char* name, const char* value)
{
  return DBCObjectAttr_setAttrString(&thiz->base, name, value);
}

NQDBCNetNode* NQDBCDocument_createNetNode(NQDBCDocument* thiz, const char* name)
{
  return NQDBCNetNode_create(thiz->base.context, name);
}

NQDBCMessage* NQDBCDocument_createMessage(NQDBCDocument* thiz, const char* name, uint32_t id, uint32_t sizeInBytes)
{
  return NQDBCMessage_create(thiz->base.context, name, id, sizeInBytes);
}

NQDBCSignal* NQDBCDocument_createSignal(NQDBCDocument* thiz, const char* name, uint32_t startBit, const NQDBCSigInfo* info)
{
  return NQDBCSignal_create(thiz->base.context, name, startBit, info);
}

NQDBCEnvVar* NQDBCDocument_createEnvVar(NQDBCDocument* thiz, const char* name, const NQDBCEnvInfo* info)
{
  return NQDBCEnvVar_create(thiz->base.context, name, info);
}

NQDBCSigGroup* NQDBCDocument_createSigGroup(NQDBCDocument* thiz, const char* name, uint32_t repetitions)
{
  return NQDBCSigGroup_create(thiz->base.context, name, repetitions);
}

NQDBCValTable* NQDBCDocument_createValTable(NQDBCDocument* thiz, const char* name, const NQDBCValDesc* items, size_t size)
{
  return NQDBCValTable_create(thiz->base.context, name, items, size);
}

NQDBCSigType* NQDBCDocument_createSigType(NQDBCDocument* thiz, const char* name, const NQDBCSigInfo* info, double defaultValue)
{
  return NQDBCSigType_create(thiz->base.context, name, info, defaultValue);
}

bool NQDBCDocument_addNetNode(NQDBCDocument* thiz, NQDBCNetNode* netNode)
{
  if (netNode == NULL || netNode->base.context != thiz->base.context)
    return false;

  if (netNode->base.parent != NULL)
    return false;

  if (netNode->base.name[0] == '\0')
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->netNodes, netNode))
    return false;

  netNode->base.parent = thiz;
  return true;
}

bool NQDBCDocument_addMessage(NQDBCDocument* thiz, NQDBCMessage* message)
{
  if (message == NULL || message->base.context != thiz->base.context)
    return false;

  if (message->base.parent != NULL)
    return false;

  if (message->base.name[0] == '\0')
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->messages, message))
    return false;

  message->base.parent = thiz;
  return true;
}

bool NQDBCDocument_addEnvVar(NQDBCDocument* thiz, NQDBCEnvVar* envVar)
{
  if (envVar == NULL || envVar->base.context != thiz->base.context)
    return false;

  if (envVar->base.parent != NULL)
    return false;

  if (envVar->base.name[0] == '\0')
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->envVars, envVar))
    return false;

  envVar->base.parent = thiz;
  return true;
}

bool NQDBCDocument_addValTable(NQDBCDocument* thiz, NQDBCValTable* valTable)
{
  if (valTable == NULL || valTable->base.context != thiz->base.context)
    return false;

  if (valTable->base.name[0] == '\0')
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->valTables, valTable))
    return false;

  return true;
}

bool NQDBCDocument_addSigType(NQDBCDocument* thiz, NQDBCSigType* sigType)
{
  if (sigType == NULL || sigType->base.context != thiz->base.context)
    return false;

  if (sigType->base.name[0] == '\0')
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->sigTypes, sigType))
    return false;

  return true;
}

static bool NQDBCDocument_defineAttr(NQDBCDocument* thiz, const char* name, DBCClassId classId, const struct NQDBCAttrValueInfo* params)
{
  DBCContext* context = thiz->base.context;

  if (DBCAttrDefines_find(context, &thiz->context.attrProtos, name) != NULL)
    return false;

  DBCClass* clazz = &context->classes[classId];
  NQDBCAttrProto* attr = NQDBCAttrProto_create(clazz, name, params);
  if (attr == NULL)
    return false;

  bool success = DBCAttrDefines_add(context, &thiz->context.attrProtos, attr);
  NQDBCAttrProto_release(context, attr);
  return success;
}

bool NQDBCDocument_defineDocumentAttr(NQDBCDocument* thiz, const char* name, const struct NQDBCAttrValueInfo* params)
{
  return NQDBCDocument_defineAttr(thiz, name, kNQDBCDocumentIndex, params);
}

bool NQDBCDocument_defineNetNodeAttr(NQDBCDocument* thiz, const char* name, const struct NQDBCAttrValueInfo* params)
{
  return NQDBCDocument_defineAttr(thiz, name, kNQDBCNetNodeIndex, params);
}

bool NQDBCDocument_defineMessageAttr(NQDBCDocument* thiz, const char* name, const struct NQDBCAttrValueInfo* params)
{
  return NQDBCDocument_defineAttr(thiz, name, kNQDBCMessageIndex, params);
}

bool NQDBCDocument_defineSignalAttr(NQDBCDocument* thiz, const char* name, const struct NQDBCAttrValueInfo* params)
{
  return NQDBCDocument_defineAttr(thiz, name, kNQDBCSignalIndex, params);
}

bool NQDBCDocument_defineEnvVarAttr(NQDBCDocument* thiz, const char* name, const struct NQDBCAttrValueInfo* params)
{
  return NQDBCDocument_defineAttr(thiz, name, kNQDBCEnvVarIndex, params);
}

NQDBCAttrProto* NQDBCDocument_findAttrProto(const NQDBCDocument* thiz, const char* name)
{
  const DBCAttrDefines* attrProtos = &thiz->context.attrProtos;
  for (size_t index = 0; index < attrProtos->size; index++) {
    NQDBCAttrProto* proto = attrProtos->data[index];
    if (!NQStrcmp(proto->name, name))
      return proto;
  }
  return NULL;
}

size_t NQDBCDocument_attrProtoCount(const NQDBCDocument* thiz)
{
  return thiz->context.attrProtos.size;
}

NQDBCAttrProto* NQDBCDocument_attrProtoAt(const NQDBCDocument* thiz, size_t index)
{
  return thiz->context.attrProtos.data[index];
}

NQDBCNetNode* NQDBCNetNode_create(DBCContext* context, const char* name)
{
  size_t lenz = NQStrlen(name) + 1;

  NQDBCNetNode* thiz = (NQDBCNetNode*)DBCAlloc(context, sizeof(struct NQDBCNetNode) + lenz);
  if (thiz == NULL)
    return NULL;

  DBCObjectAttr_init(context, &thiz->base, kNQDBCNetNodeIndex);

  char* str = (char*)thiz + sizeof(struct NQDBCNetNode);
  NQMemcpy(str, name, lenz);
  thiz->base.name = str;

  return thiz;
}

static void NQDBCNetNode_deinit(NQDBCNetNode* thiz)
{
  DBCContext* context = thiz->base.context;

  DBCObjectAttr_finalize(context, &thiz->base);
}

void NQDBCNetNode_release(NQDBCNetNode* thiz)
{
  DBCObject_release(thiz);
}

const char* NQDBCNetNode_name(const NQDBCNetNode* thiz)
{
  return thiz->base.name;
}

NQDBCAttribute* NQDBCNetNode_findAttribute(const NQDBCNetNode* thiz, const char* name)
{
  return DBCAttrMap_find(thiz->base.context, &thiz->base.attributes, name);
}

size_t NQDBCNetNode_attributeCount(const NQDBCNetNode* thiz)
{
  return thiz->base.attributes.size;
}

NQDBCAttribute* NQDBCNetNode_attributeAt(const NQDBCNetNode* thiz, size_t index)
{
  return &thiz->base.attributes.data[index];
}

bool NQDBCNetNode_setComment(NQDBCNetNode* thiz, const char* comment)
{
  return DBCString_setCharacters(thiz->base.context, &thiz->base.comment, comment);
}

bool NQDBCNetNode_getAttrInt(const NQDBCNetNode* thiz, const char* name, int64_t* result)
{
  return DBCObjectAttr_getAttrInt(&thiz->base, name, result);
}

bool NQDBCNetNode_setAttrInt(NQDBCNetNode* thiz, const char* name, int64_t value)
{
  return DBCObjectAttr_setAttrInt(&thiz->base, name, value);
}

bool NQDBCNetNode_getAttrFloat(const NQDBCNetNode* thiz, const char* name, double* result)
{
  return DBCObjectAttr_getAttrFloat(&thiz->base, name, result);
}

bool NQDBCNetNode_setAttrFloat(NQDBCNetNode* thiz, const char* name, double value)
{
  return DBCObjectAttr_setAttrFloat(&thiz->base, name, value);
}

bool NQDBCNetNode_getAttrString(const NQDBCNetNode* thiz, const char* name, const char** result)
{
  return DBCObjectAttr_getAttrString(&thiz->base, name, result);
}

bool NQDBCNetNode_setAttrString(NQDBCNetNode* thiz, const char* name, const char* value)
{
  return DBCObjectAttr_setAttrString(&thiz->base, name, value);
}

bool NQDBCMessage_setComment(NQDBCMessage* thiz, const char* comment)
{
  return DBCString_setCharacters(thiz->base.context, &thiz->base.comment, comment);
}

bool NQDBCMessage_getAttrInt(const NQDBCMessage* thiz, const char* name, int64_t* result)
{
  return DBCObjectAttr_getAttrInt(&thiz->base, name, result);
}

bool NQDBCMessage_setAttrInt(NQDBCMessage* thiz, const char* name, int64_t value)
{
  return DBCObjectAttr_setAttrInt(&thiz->base, name, value);
}

bool NQDBCMessage_getAttrFloat(const NQDBCMessage* thiz, const char* name, double* result)
{
  return DBCObjectAttr_getAttrFloat(&thiz->base, name, result);
}

bool NQDBCMessage_setAttrFloat(NQDBCMessage* thiz, const char* name, double value)
{
  return DBCObjectAttr_setAttrFloat(&thiz->base, name, value);
}

bool NQDBCMessage_getAttrString(const NQDBCMessage* thiz, const char* name, const char** result)
{
  return DBCObjectAttr_getAttrString(&thiz->base, name, result);
}

bool NQDBCMessage_setAttrString(NQDBCMessage* thiz, const char* name, const char* value)
{
  return DBCObjectAttr_setAttrString(&thiz->base, name, value);
}

bool NQDBCMessage_addTransmitter(NQDBCMessage* thiz, NQDBCNetNode* transmitter)
{
  return DBCArray_append(thiz->base.context, &thiz->transmitters, transmitter);
}

NQDBCSignal* NQDBCSignal_create(DBCContext* context, const char* name, uint32_t startBit, const NQDBCSigInfo* info)
{
  size_t nlenz = NQStrlen(name) + 1;
  size_t ulenz = NQStrlen(info->unit) + 1;

  NQDBCSignal* thiz = (NQDBCSignal*)DBCAlloc(context, sizeof(struct NQDBCSignal) + nlenz + ulenz);
  if (thiz == NULL)
    return NULL;

  DBCObjectAttr_init(context, &thiz->base, kNQDBCSignalIndex);

  char* str = (char*)thiz + sizeof(struct NQDBCSignal);
  NQMemcpy(str, name, nlenz);
  thiz->base.name = str;

  thiz->valueType = kNQDBCSigValueDouble;
  thiz->multiplexer.signal = NULL;
  DBCRanges_init(context, &thiz->multiplexer.ranges);

  thiz->startBit = startBit;

  thiz->info.sizeInBits = info->sizeInBits;
  thiz->info.byteOrder = info->byteOrder;
  thiz->info.isUnsigned = info->isUnsigned;
  thiz->info.factor = info->factor;
  thiz->info.offset = info->offset;
  thiz->info.minimum = info->minimum;
  thiz->info.maximum = info->maximum;

  str += nlenz;
  NQMemcpy(str, info->unit, ulenz);
  thiz->info.unit = str;

  thiz->valTable = NULL;
  thiz->sigGroup = NULL;
  thiz->sigType = NULL;

  DBCArray_init(context, &thiz->receivers);

  return thiz;
}

static void NQDBCSignal_deinit(NQDBCSignal* thiz)
{
  DBCContext* context = thiz->base.context;

  NQ_ASSERT(thiz->sigGroup == NULL);

  NQDBCSignal_setValTable(thiz, NULL);
  NQDBCSignal_setSignalType(thiz, NULL);
  NQDBCSignal_setMultiplexerSignal(thiz, NULL);
  DBCRanges_finalize(context, &thiz->multiplexer.ranges);
  DBCArray_finalize(context, &thiz->receivers);

  DBCObjectAttr_finalize(context, &thiz->base);
}

void NQDBCSignal_release(NQDBCSignal* thiz)
{
  DBCObject_release(thiz);
}

const char* NQDBCSignal_name(const NQDBCSignal* thiz)
{
  return thiz->base.name;
}

const char* NQDBCSignal_comment(const NQDBCSignal* thiz)
{
  return thiz->base.comment.characters;
}

NQDBCAttribute* NQDBCSignal_findAttribute(const NQDBCSignal* thiz, const char* name)
{
  return DBCAttrMap_find(thiz->base.context, &thiz->base.attributes, name);
}

size_t NQDBCSignal_attributeCount(const NQDBCSignal* thiz)
{
  return thiz->base.attributes.size;
}

NQDBCAttribute* NQDBCSignal_attributeAt(const NQDBCSignal* thiz, size_t index)
{
  return &thiz->base.attributes.data[index];
}

NQDBCSigGroup* NQDBCSignal_sigGroup(const NQDBCSignal* thiz)
{
  return thiz->sigGroup;
}

NQDBCSigValueType NQDBCSignal_valueType(const NQDBCSignal* thiz)
{
  return thiz->valueType;
}

uint32_t NQDBCSignal_startBit(const NQDBCSignal* thiz)
{
  return thiz->startBit;
}

uint32_t NQDBCSignal_sizeInBits(const NQDBCSignal* thiz)
{
  return thiz->info.sizeInBits;
}

NQDBCByteOrder NQDBCSignal_byteOrder(const NQDBCSignal* thiz)
{
  return thiz->info.byteOrder;
}

bool NQDBCSignal_isUnsigned(const NQDBCSignal* thiz)
{
  return thiz->info.isUnsigned;
}

#ifdef NQ_USE_FLOATING_POINT
double NQDBCSignal_factor(const NQDBCSignal* thiz)
{
  return thiz->info.factor;
}

double NQDBCSignal_offset(const NQDBCSignal* thiz)
{
  return thiz->info.offset;
}

double NQDBCSignal_minimum(const NQDBCSignal* thiz)
{
  return thiz->info.minimum;
}

double NQDBCSignal_maximum(const NQDBCSignal* thiz)
{
  return thiz->info.maximum;
}
#endif

const char* NQDBCSignal_unit(const NQDBCSignal* thiz)
{
  return thiz->info.unit;
}

size_t NQDBCSignal_receiverCount(const NQDBCSignal* thiz)
{
  return thiz->receivers.size;
}

NQDBCNetNode* NQDBCSignal_receiverAt(const NQDBCSignal* thiz, size_t index)
{
  if (index < thiz->receivers.size)
    return toNQDBCNetNode(thiz->receivers.objects[index]);
  return NULL;
}

bool NQDBCSignal_setComment(NQDBCSignal* thiz, const char* comment)
{
  return DBCString_setCharacters(thiz->base.context, &thiz->base.comment, comment);
}

bool NQDBCSignal_getAttrInt(const NQDBCSignal* thiz, const char* name, int64_t* result)
{
  return DBCObjectAttr_getAttrInt(&thiz->base, name, result);
}

bool NQDBCSignal_setAttrInt(NQDBCSignal* thiz, const char* name, int64_t value)
{
  return DBCObjectAttr_setAttrInt(&thiz->base, name, value);
}

bool NQDBCSignal_getAttrFloat(const NQDBCSignal* thiz, const char* name, double* result)
{
  return DBCObjectAttr_getAttrFloat(&thiz->base, name, result);
}

bool NQDBCSignal_setAttrFloat(NQDBCSignal* thiz, const char* name, double value)
{
  return DBCObjectAttr_setAttrFloat(&thiz->base, name, value);
}

bool NQDBCSignal_getAttrString(const NQDBCSignal* thiz, const char* name, const char** result)
{
  return DBCObjectAttr_getAttrString(&thiz->base, name, result);
}

bool NQDBCSignal_setAttrString(NQDBCSignal* thiz, const char* name, const char* value)
{
  return DBCObjectAttr_setAttrString(&thiz->base, name, value);
}

void NQDBCSignal_setValueType(NQDBCSignal* thiz, NQDBCSigValueType type)
{
  thiz->valueType = type;
}

bool NQDBCSignal_addReceiver(NQDBCSignal* thiz, NQDBCNetNode* receiver)
{
  return DBCArray_append(thiz->base.context, &thiz->receivers, receiver);
}

void NQDBCSignal_setValTable(NQDBCSignal* thiz, NQDBCValTable* valTable)
{
  if (thiz->valTable != NULL)
    DBCObject_release(thiz->valTable);

  if (valTable != NULL)
    DBCObject_retain(valTable);

  thiz->valTable = valTable;
}

void NQDBCSignal_setSignalType(NQDBCSignal* thiz, NQDBCSigType* sigType)
{
  if (thiz->sigType != NULL)
    DBCObject_release(thiz->sigType);

  if (sigType != NULL)
    DBCObject_retain(sigType);

  thiz->sigType = sigType;
}

bool NQDBCSignal_setMultiplexerSignal(NQDBCSignal* thiz, NQDBCSignal* multiplexerSignal)
{
  if (multiplexerSignal != NULL) {
    if (thiz->base.parent == NULL || thiz->base.parent != multiplexerSignal->base.parent)
      return false;
  }

  if (thiz->multiplexer.signal != NULL)
    DBCObject_release(thiz->multiplexer.signal);

  if (multiplexerSignal != NULL)
    DBCObject_retain(multiplexerSignal);

  thiz->multiplexer.signal = multiplexerSignal;
  return true;
}

bool NQDBCSignal_addMultiplexerRange(NQDBCSignal* thiz, uint32_t first, uint32_t second)
{
  return DBCRanges_add(thiz->base.context, &thiz->multiplexer.ranges, first, second);
}

NQDBCEnvVar* NQDBCEnvVar_create(DBCContext* context, const char* name, const NQDBCEnvInfo* info)
{
  size_t nlenz = NQStrlen(name) + 1;
  size_t ulenz = NQStrlen(info->unit) + 1;

  NQDBCEnvVar* thiz = (NQDBCEnvVar*)DBCAlloc(context, sizeof(struct NQDBCEnvVar) + nlenz + ulenz);
  if (thiz == NULL)
    return NULL;

  DBCObjectAttr_init(context, &thiz->base, kNQDBCEnvVarIndex);

  char* str = (char*)thiz + sizeof(struct NQDBCEnvVar);
  NQMemcpy(str, name, nlenz);
  thiz->base.name = str;

  thiz->info.id = info->id;
  thiz->info.valueType = info->valueType;
  thiz->info.accessType = info->accessType;
  thiz->info.minimum = info->minimum;
  thiz->info.maximum = info->maximum;
  thiz->info.initialValue = info->initialValue;

  str += nlenz;
  NQMemcpy(str, info->unit, ulenz);
  thiz->info.unit = str;

  thiz->dataSize = 0;
  thiz->valTable = NULL;

  DBCArray_init(context, &thiz->accessNodes);

  return thiz;
}

static void NQDBCEnvVar_deinit(NQDBCEnvVar* thiz)
{
  DBCContext* context = thiz->base.context;

  DBCArray_finalize(context, &thiz->accessNodes);
  if (thiz->valTable != NULL)
    DBCObject_release(thiz->valTable);

  DBCObjectAttr_finalize(context, &thiz->base);
}

void NQDBCEnvVar_release(NQDBCEnvVar* thiz)
{
  DBCObject_release(thiz);
}

bool NQDBCEnvVar_setComment(NQDBCEnvVar* thiz, const char* comment)
{
  return DBCString_setCharacters(thiz->base.context, &thiz->base.comment, comment);
}

NQDBCAttribute* NQDBCEnvVar_findAttribute(const NQDBCEnvVar* thiz, const char* name)
{
  return DBCAttrMap_find(thiz->base.context, &thiz->base.attributes, name);
}

size_t NQDBCEnvVar_attributeCount(const NQDBCEnvVar* thiz)
{
  return thiz->base.attributes.size;
}

NQDBCAttribute* NQDBCEnvVar_attributeAt(const NQDBCEnvVar* thiz, size_t index)
{
  return &thiz->base.attributes.data[index];
}

bool NQDBCEnvVar_getAttrInt(const NQDBCEnvVar* thiz, const char* name, int64_t* result)
{
  return DBCObjectAttr_getAttrInt(&thiz->base, name, result);
}

bool NQDBCEnvVar_setAttrInt(NQDBCEnvVar* thiz, const char* name, int64_t value)
{
  return DBCObjectAttr_setAttrInt(&thiz->base, name, value);
}

bool NQDBCEnvVar_getAttrFloat(const NQDBCEnvVar* thiz, const char* name, double* result)
{
  return DBCObjectAttr_getAttrFloat(&thiz->base, name, result);
}

bool NQDBCEnvVar_setAttrFloat(NQDBCEnvVar* thiz, const char* name, double value)
{
  return DBCObjectAttr_setAttrFloat(&thiz->base, name, value);
}

bool NQDBCEnvVar_getAttrString(const NQDBCEnvVar* thiz, const char* name, const char** result)
{
  return DBCObjectAttr_getAttrString(&thiz->base, name, result);
}

bool NQDBCEnvVar_setAttrString(NQDBCEnvVar* thiz, const char* name, const char* value)
{
  return DBCObjectAttr_setAttrString(&thiz->base, name, value);
}

bool NQDBCEnvVar_addAccessNode(NQDBCEnvVar* thiz, NQDBCNetNode* node)
{
  return DBCArray_append(thiz->base.context, &thiz->accessNodes, node);
}

bool NQDBCEnvVar_setDataSize(NQDBCEnvVar* thiz, uint32_t size)
{
  thiz->dataSize = size;
  return true;
}

void NQDBCEnvVar_setValTable(NQDBCEnvVar* thiz, NQDBCValTable* valTable)
{
  if (thiz->valTable != NULL)
    DBCObject_release(thiz->valTable);

  if (valTable != NULL)
    DBCObject_retain(valTable);

  thiz->valTable = valTable;
}

NQDBCSigGroup* NQDBCSigGroup_create(DBCContext* context, const char* name, uint32_t repetitions)
{
  size_t lenz = NQStrlen(name) + 1;

  NQDBCSigGroup* thiz = (NQDBCSigGroup*)DBCAlloc(context, sizeof(struct NQDBCSigGroup) + lenz);
  if (thiz == NULL)
    return NULL;

  DBCObjectChild_init(context, &thiz->base, kNQDBCSigGroupIndex);

  char* str = (char*)thiz + sizeof(struct NQDBCSigGroup);
  NQMemcpy(str, name, lenz);
  thiz->base.name = str;

  thiz->repetitions = repetitions;
  DBCArray_init(context, &thiz->signals);

  return thiz;
}

static void NQDBCSigGroup_deinit(NQDBCSigGroup* thiz)
{
  for (size_t i = 0; i < thiz->signals.size; i++) {
    NQDBCSignal* iter = toNQDBCSignal(thiz->signals.objects[i]);
    iter->sigGroup = NULL;
  }

  DBCArray_finalize(thiz->base.context, &thiz->signals);
}

void NQDBCSigGroup_release(NQDBCSigGroup* thiz)
{
  DBCObject_release(thiz);
}

const char* NQDBCSigGroup_name(const NQDBCSigGroup* thiz)
{
  return thiz->base.name;
}

size_t NQDBCSigGroup_signalCount(const NQDBCSigGroup* thiz)
{
  return thiz->signals.size;
}

NQDBCSignal* NQDBCSigGroup_signalAt(const NQDBCSigGroup* thiz, size_t index)
{
  return toNQDBCSignal(thiz->signals.objects[index]);
}

bool NQDBCSigGroup_addSignal(NQDBCSigGroup* thiz, NQDBCSignal* signal)
{
  if (signal == NULL || signal->base.context != thiz->base.context)
    return false;

  if (signal->base.parent != thiz->base.parent)
    return false;

  if (signal->sigGroup != NULL)
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->signals, signal))
    return false;

  signal->sigGroup = thiz;
  return true;
}

NQDBCMessage* NQDBCMessage_create(DBCContext* context, const char* name, uint32_t id, uint32_t sizeInBytes)
{
  size_t lenz = NQStrlen(name) + 1;

  NQDBCMessage* thiz = (NQDBCMessage*)DBCAlloc(context, sizeof(struct NQDBCMessage) + lenz);
  if (thiz == NULL)
    return NULL;

  DBCObjectAttr_init(context, &thiz->base, kNQDBCMessageIndex);

  char* str = (char*)thiz + sizeof(struct NQDBCMessage);
  NQMemcpy(str, name, lenz);
  thiz->base.name = str;

  thiz->id = id;
  thiz->sizeInBytes = sizeInBytes;

  DBCArray_init(context, &thiz->groups);
  DBCArray_init(context, &thiz->signals);
  DBCArray_init(context, &thiz->transmitters);

  thiz->isPseudo = NQStrcmp(name, "VECTOR__INDEPENDENT_SIG_MSG") == 0;

  return thiz;
}

static void NQDBCMessage_deinit(NQDBCMessage* thiz)
{
  DBCContext* context = thiz->base.context;

  DBCArray_finalize(context, &thiz->groups);
  DBCArray_finalize(context, &thiz->signals);
  DBCArray_finalize(context, &thiz->transmitters);

  DBCObjectAttr_finalize(context, &thiz->base);
}

void NQDBCMessage_release(NQDBCMessage* thiz)
{
  DBCObject_release(thiz);
}

const char* NQDBCMessage_name(const NQDBCMessage* thiz)
{
  return thiz->base.name;
}

NQDBCAttribute* NQDBCMessage_findAttribute(const NQDBCMessage* thiz, const char* name)
{
  return DBCAttrMap_find(thiz->base.context, &thiz->base.attributes, name);
}

size_t NQDBCMessage_attributeCount(const NQDBCMessage* thiz)
{
  return thiz->base.attributes.size;
}

NQDBCAttribute* NQDBCMessage_attributeAt(const NQDBCMessage* thiz, size_t index)
{
  return &thiz->base.attributes.data[index];
}

uint32_t NQDBCMessage_id(const NQDBCMessage* thiz)
{
  return thiz->id;
}

uint32_t NQDBCMessage_sizeInBytes(const NQDBCMessage* thiz)
{
  return thiz->sizeInBytes;
}

const char* NQDBCMessage_comment(const NQDBCMessage* thiz)
{
  return thiz->base.comment.characters;
}

bool NQDBCMessage_isPseudo(const NQDBCMessage* thiz)
{
  return thiz->isPseudo;
}

NQDBCSignal* NQDBCMessage_findSignal(NQDBCMessage* thiz, const char* name)
{
  size_t i;
  for (i = 0; i < thiz->signals.size; i++) {
    NQDBCSignal* iter = toNQDBCSignal(thiz->signals.objects[i]);
    if (!NQStrcmp(iter->base.name, name))
      return iter;
  }
  return NULL;
}

NQDBCSigGroup* NQDBCMessage_groupAt(const NQDBCMessage* thiz, size_t index)
{
  NQ_ASSERT(index < thiz->groups.size);
  return toNQDBCSigGroup(thiz->groups.objects[index]);
}

size_t NQDBCMessage_groupCount(const NQDBCMessage* thiz)
{
  return thiz->groups.size;
}

NQDBCSignal* NQDBCMessage_signalAt(const NQDBCMessage* thiz, size_t index)
{
  NQ_ASSERT(index < thiz->signals.size);
  return toNQDBCSignal(thiz->signals.objects[index]);
}

size_t NQDBCMessage_signalCount(const NQDBCMessage* thiz)
{
  return thiz->signals.size;
}

NQDBCNetNode* NQDBCMessage_senderAt(const NQDBCMessage* thiz, size_t index)
{
  NQ_ASSERT(index < thiz->transmitters.size);
  return toNQDBCNetNode(thiz->transmitters.objects[index]);
}

size_t NQDBCMessage_senderCount(const NQDBCMessage* thiz)
{
  return thiz->transmitters.size;
}

bool NQDBCMessage_addSignal(NQDBCMessage* thiz, NQDBCSignal* signal)
{
  if (signal == NULL || signal->base.context != thiz->base.context)
    return false;

  if (signal->base.parent != NULL)
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->signals, signal))
    return false;

  signal->base.parent = thiz;
  return true;
}

bool NQDBCMessage_addSigGroup(NQDBCMessage* thiz, NQDBCSigGroup* sigGroup)
{
  if (sigGroup == NULL || sigGroup->base.context != thiz->base.context)
    return false;

  if (sigGroup->base.parent != NULL)
    return false;

  if (!DBCArray_append(thiz->base.context, &thiz->groups, sigGroup))
    return false;

  sigGroup->base.parent = thiz;
  return true;
}

bool NQDBCMessage_setMultiplexerSignal(NQDBCMessage* thiz, NQDBCSignal* multiplexerSignal)
{
  size_t i;
  for (i = 0; i < thiz->signals.size; i++) {
    NQDBCSignal* iter = toNQDBCSignal(thiz->signals.objects[i]);
    if (!NQDBCSignal_setMultiplexerSignal(iter, multiplexerSignal))
      return false;
  }
  return true;
}

NQDBCNetNode* NQDBCDocument_findNetNode(NQDBCDocument* thiz, const char* name)
{
  size_t i;
  for (i = 0; i < thiz->netNodes.size; i++) {
    NQDBCNetNode* iter = toNQDBCNetNode(thiz->netNodes.objects[i]);
    if (!NQStrcmp(iter->base.name, name))
      return iter;
  }
  return NULL;
}

NQDBCMessage* NQDBCDocument_findMessage(NQDBCDocument* thiz, uint32_t id)
{
  size_t i;
  for (i = 0; i < thiz->messages.size; i++) {
    NQDBCMessage* iter = toNQDBCMessage(thiz->messages.objects[i]);
    if (iter->id == id)
      return iter;
  }
  return NULL;
}

NQDBCSignal* NQDBCDocument_findSignal(NQDBCDocument* thiz, uint32_t messageId, const char* signalName)
{
  NQDBCMessage* message = NQDBCDocument_findMessage(thiz, messageId);
  if (message == NULL)
    return NULL;
  
  return NQDBCMessage_findSignal(message, signalName);
}

NQDBCEnvVar* NQDBCDocument_findEnvVar(NQDBCDocument* thiz, const char* name)
{
  size_t i;
  for (i = 0; i < thiz->envVars.size; i++) {
    NQDBCEnvVar* iter = toNQDBCEnvVar(thiz->envVars.objects[i]);
    if (!NQStrcmp(iter->base.name, name))
      return iter;
  }
  return NULL;
}

NQDBCValTable* NQDBCDocument_findValTable(NQDBCDocument* thiz, const char* name)
{
  size_t i;
  for (i = 0; i < thiz->valTables.size; i++) {
    NQDBCValTable* iter = toNQDBCValTable(thiz->valTables.objects[i]);
    if (!NQStrcmp(iter->base.name, name))
      return iter;
  }
  return NULL;
}

NQDBCSigType* NQDBCDocument_findSigType(NQDBCDocument* thiz, const char* name)
{
  size_t i;
  for (i = 0; i < thiz->sigTypes.size; i++) {
    NQDBCSigType* iter = toNQDBCSigType(thiz->sigTypes.objects[i]);
    if (!NQStrcmp(iter->base.name, name))
      return iter;
  }
  return NULL;
}

const char* NQDBCDocument_version(const NQDBCDocument* thiz)
{
  return thiz->version.characters;
}

bool NQDBCDocument_setVersion(NQDBCDocument* thiz, const char* version)
{
  return DBCString_setCharacters(&thiz->context, &thiz->version, version);
}

const char* NQDBCDocument_symbolAt(const NQDBCDocument* thiz, size_t index)
{
  return thiz->symbols.items[index];
}

size_t NQDBCDocument_symbolCount(const NQDBCDocument* thiz)
{
  return thiz->symbols.count;
}

bool NQDBCDocument_setSymbols(NQDBCDocument* thiz, const char** symbols, size_t count)
{
  return DBCSymbols_set(thiz->base.context, &thiz->symbols, symbols, count);
}

void NQDBCDocument_setBitTiming(NQDBCDocument* thiz, const NQDBCBitTiming* bitTiming)
{
  if (bitTiming == NULL)
    thiz->hasBitTiming = false;
  else {
    thiz->hasBitTiming = true;
    thiz->bitTiming.baudrate = bitTiming->baudrate;
    thiz->bitTiming.btr1 = bitTiming->btr1;
    thiz->bitTiming.btr2 = bitTiming->btr2;
  }
}

NQDBCValTable* NQDBCValTable_create(DBCContext* context, const char* name, const NQDBCValDesc* items, size_t size)
{
  char c;
  size_t i;

  size_t totalInBytes = sizeof(struct NQDBCValTable);

  size_t nlenz = NQStrlen(name) + 1;
  totalInBytes += nlenz;

  for (i = 0; i < size; i++) {
    totalInBytes += sizeof(char*) + sizeof(uint32_t);
    totalInBytes += NQStrlen(items[i].description) + 1;
  }

  NQDBCValTable* thiz = (NQDBCValTable*)DBCAlloc(context, totalInBytes);
  if (thiz == NULL)
    return NULL;

  DBCObjectBase_init(context, &thiz->base, kNQDBCValTableIndex);

  char* str = (char*)thiz + sizeof(struct NQDBCValTable);

  char** description = (char**)str;
  thiz->description = description;
  str += sizeof(char*) * size;

  thiz->value = (uint32_t*)str;
  str += sizeof(uint32_t) * size;

  thiz->base.name = str;
  NQMemcpy(str, name, nlenz);
  str += nlenz;

  thiz->size = (uint32_t)size;
  for (i = 0; i < size; i++) {
    thiz->value[i] = items[i].value;
    thiz->description[i] = str;
    const char* s = items[i].description;
    do {
      c = *str++ = *s++;
    } while (c != '\0');
  }

  NQ_ASSERT(((char*)thiz + totalInBytes) == str);
  return thiz;
}

static void NQDBCValTable_deinit(NQDBCValTable* thiz)
{
}

void NQDBCValTable_release(NQDBCValTable* thiz)
{
  DBCObject_release(thiz);
}

NQDBCSigType* NQDBCSigType_create(DBCContext* context, const char* name, const NQDBCSigInfo* info, double defaultValue)
{
  size_t nlenz = NQStrlen(name) + 1;
  size_t ulenz = NQStrlen(info->unit) + 1;

  NQDBCSigType* thiz = (NQDBCSigType*)DBCAlloc(context, sizeof(*thiz) + nlenz + ulenz);
  if (thiz == NULL)
    return NULL;

  DBCObjectBase_init(context, &thiz->base, kNQDBCSigTypeIndex);

  char* str = (char*)thiz + sizeof(*thiz);
  NQMemcpy(str, name, nlenz);
  thiz->base.name = str;

  thiz->info.sizeInBits = info->sizeInBits;
  thiz->info.byteOrder = info->byteOrder;
  thiz->info.isUnsigned = info->isUnsigned;
  thiz->info.factor = info->factor;
  thiz->info.offset = info->offset;
  thiz->info.minimum = info->minimum;
  thiz->info.maximum = info->maximum;

  str += nlenz;
  NQMemcpy(str, info->unit, ulenz);
  thiz->info.unit = str;

  thiz->defaultValue = defaultValue;
  thiz->valTable = NULL;

  return thiz;
}

static void NQDBCSigType_deinit(NQDBCSigType* thiz)
{
  NQDBCSigType_setValTable(thiz, NULL);
}

void NQDBCSigType_release(NQDBCSigType* thiz)
{
  DBCObject_release(thiz);
}

void NQDBCSigType_setValTable(NQDBCSigType* thiz, NQDBCValTable* valTable)
{
  if (thiz->valTable != NULL)
    DBCObject_release(thiz->valTable);

  if (valTable != NULL)
    DBCObject_retain(valTable);

  thiz->valTable = valTable;
}

NQDBCByteOrder NQDBCSigType_byteOrder(const NQDBCSigType* thiz)
{
  return thiz->info.byteOrder;
}
