/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/wasm/Module.h"

#include <libnetq/BufferBuilder.h>
#include <libnetq/ByteBuffer.h>
#include <libnetq/Malloc.h>
#include <libnetq/string/StringRange.h>
#include <libnetq/Log.h>
#include <libnetq/Leb128.h>
#include <libnetq/Math.h>
#include <libnetq/Limits.h>
#include <libnetq/Assert.h>
#include <libnetq/UTF.h>
#include <libnetq/io/ReadWrapper.h>
#include <libnetq/io/WriteWrapper.h>
#include <libnetq/io/DataReader.h>

// Opcodes allowed in constant expressions (see Opcodes.def)
enum {
  NQ_WASM_OP_End = 0x0b,
  NQ_WASM_OP_GlobalGet = 0x23,
  NQ_WASM_OP_I32Const = 0x41,
  NQ_WASM_OP_I64Const = 0x42,
  NQ_WASM_OP_F32Const = 0x43,
  NQ_WASM_OP_F64Const = 0x44,
  NQ_WASM_OP_I32Add = 0x6a,
  NQ_WASM_OP_I32Sub = 0x6b,
  NQ_WASM_OP_I32Mul = 0x6c,
  NQ_WASM_OP_I64Add = 0x7c,
  NQ_WASM_OP_I64Sub = 0x7d,
  NQ_WASM_OP_I64Mul = 0x7e,
  NQ_WASM_OP_RefNull = 0xd0,
  NQ_WASM_OP_RefFunc = 0xd2,
  NQ_WASM_OP_SimdPrefix = 0xfd,
  NQ_WASM_OP_V128Const = 0x0c, // after the SIMD prefix
};

#if NQ_HAS_BUILTIN(__builtin_types_compatible_p)
NQ_STATIC_ASSERT(__builtin_types_compatible_p(NQWasmWriteCallback, NQWriteCallback), \
                 "NQWasmWriteCallback and NQWriteCallback are different types");
#endif

static bool readLeb128Uint32(NQDataReader* reader, uint32_t* value)
{
  uint8_t byte;
  NQLeb128Dec dec;
  NQLeb128Dec_init(&dec, false);

  bool done = false;
  do {
    if (!NQDataReader_readUint8(reader, &byte))
      return false;
    done = NQLeb128Dec_update(&dec, byte);
    if (sizeof(*value) < NQLeb128Dec_size(&dec))
      return false;
  } while (!done);

  *value = NQLeb128Dec_valueUint32(&dec);
  return true;
}

static bool readLeb128Uint64(NQDataReader* reader, uint64_t* value)
{
  uint8_t byte;
  NQLeb128Dec dec;
  NQLeb128Dec_init(&dec, false);

  bool done = false;
  do {
    if (!NQDataReader_readUint8(reader, &byte))
      return false;
    done = NQLeb128Dec_update(&dec, byte);
    if (sizeof(*value) < NQLeb128Dec_size(&dec))
      return false;
  } while (!done);

  *value = NQLeb128Dec_valueUint64(&dec);
  return true;
}

static bool readString(NQDataReader* thiz, NQStringRange* value)
{
  if (!readLeb128Uint32(thiz, &value->length))
    return false;

  value->characters = (char*)NQDataReader_currentData(thiz);
  return NQDataReader_skipAll(thiz, value->length);
}

typedef struct NQWasmUnknownSection NQWasmUnknownSection;
struct NQWasmUnknownSection {
  NQWasmSection base;
  uint32_t size;
  uint8_t data[1];
};

static bool writeLeb128Uint32(NQWriteWrapper* thiz, uint32_t value)
{
  NQLeb128Buffer buffer;
  size_t size = NQLeb128EncodeUint32(buffer, sizeof(buffer), value);
  return NQWriteWrapper_writeAll(thiz, buffer, size);
}

static bool writeLeb128Uint64(NQWriteWrapper* thiz, uint64_t value)
{
  NQLeb128Buffer buffer;
  size_t size = NQLeb128EncodeUint64(buffer, sizeof(buffer), value);
  return NQWriteWrapper_writeAll(thiz, buffer, size);
}

static bool writeString(NQWriteWrapper* thiz, const char* value)
{
  uint32_t length = (uint32_t)NQStrlen(value);
  if (!writeLeb128Uint32(thiz, length))
    return false;
  return NQWriteWrapper_writeAll(thiz, value, length);
}

static bool writeWasmSection(NQWriteWrapper* thiz, uint8_t sectionId, const void* data, uint32_t size)
{
  if (!NQWriteWrapper_writeUint8(thiz, sectionId))
    return false;
  if (!writeLeb128Uint32(thiz, size))
    return false;
  return NQWriteWrapper_writeAll(thiz, data, size);
}

static NQWasmUnknownSection* wasmUnknownSectionCreate(uint8_t sectionId, const uint8_t* data, uint32_t size)
{
  NQWasmUnknownSection* thiz = (NQWasmUnknownSection*)NQMalloc(sizeof(NQWasmUnknownSection) - sizeof(thiz->data) + size);
  if (thiz == NULL)
    return NULL;

  thiz->base.sectionId = sectionId;
  NQListHead_init(&thiz->base.list);
  thiz->size = size;
  memcpy(thiz->data, data, size);
  return thiz;
}

static void wasmUnknownSectionDestroy(NQWasmUnknownSection* thiz)
{
  NQListHead_remove(&thiz->base.list);
  NQFree(thiz);
}

static inline bool wasmUnknownSectionWriteTo(NQWasmUnknownSection* thiz, NQByteBuffer* tmpBuf, NQWriteWrapper* writer)
{
  return writeWasmSection(writer, thiz->base.sectionId, thiz->data, thiz->size);
}

static int writeByteBuffer(void* userdata, const void* data, size_t size)
{
  NQByteBuffer* buffer = (NQByteBuffer*)userdata;
  int n = (int)NQGetMin(NQ_INT32_MAX, size);
  return NQByteBuffer_append(buffer, (const uint8_t*)data, n) ? n : -1;
}

static NQWasmCustomSection* createCustomSection(const char* name, size_t nameLength, const void* data, size_t size)
{
  if (nameLength > NQ_UINT32_MAX || size > NQ_UINT32_MAX) {
    NQ_LOGE("Custom section is too big");
    return NULL;
  }
  if (!NQCalculateUTF8Info((const uint8_t*)name, (const uint8_t*)name + nameLength, NULL)) {
    NQ_LOGE("Custom section name is not valid UTF-8");
    return NULL;
  }

  NQWasmCustomSection* thiz = (NQWasmCustomSection*)NQMalloc(sizeof(*thiz) + nameLength + 1 + size);
  if (thiz == NULL) {
    NQ_LOGE("No memory");
    return NULL;
  }

  thiz->base.sectionId = NQ_WASM_SECTION_CUSTOM_ID;
  NQListHead_init(&thiz->base.list);

  char* ptr = (char*)thiz + sizeof(*thiz);
  memcpy(ptr, name, nameLength);
  ptr[nameLength] = '\0';
  thiz->name = ptr;
  thiz->nameLength = (uint32_t)nameLength;
  ptr += nameLength + 1;

  if (size != 0)
    memcpy(ptr, data, size);
  thiz->data = (const uint8_t*)ptr;
  thiz->size = (uint32_t)size;

  return thiz;
}

NQWasmCustomSection* NQWasmCustomSection_create(const char* name, const void* data, size_t size)
{
  return createCustomSection(name, NQStrlen(name), data, size);
}

NQWasmCustomSection* NQWasmCustomSection_fromMemory(const void* data, size_t size)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  NQStringRange name;
  if (!readString(&reader, &name)) {
    NQ_LOGE("Custom section name is wrong");
    return NULL;
  }

  return createCustomSection(name.characters, name.length, NQDataReader_currentData(&reader), NQDataReader_availableSize(&reader));
}

void NQWasmCustomSection_destroy(NQWasmCustomSection* thiz)
{
  NQListHead_remove(&thiz->base.list);
  NQFree(thiz);
}

static bool wasmCustomSectionWriteTo(const NQWasmCustomSection* thiz, NQByteBuffer* tmpBuf, NQWriteWrapper* writer)
{
  NQByteBuffer_resize(tmpBuf, 0);

  NQWriteWrapper tmpWriter;
  NQWriteWrapper_init(&tmpWriter, writeByteBuffer, tmpBuf);

  if (!writeLeb128Uint32(&tmpWriter, thiz->nameLength))
    return false;
  if (!NQWriteWrapper_writeAll(&tmpWriter, thiz->name, thiz->nameLength))
    return false;
  if (!NQWriteWrapper_writeAll(&tmpWriter, thiz->data, thiz->size))
    return false;

  return writeWasmSection(writer, thiz->base.sectionId, NQByteBuffer_data(tmpBuf), NQByteBuffer_size(tmpBuf));
}

bool NQWasmCustomSection_writeTo(const NQWasmCustomSection* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQByteBuffer buffer;
  NQByteBuffer_init(&buffer);

  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);

  bool res = wasmCustomSectionWriteTo(thiz, &buffer, &writer);
  NQByteBuffer_finalize(&buffer);
  return res;
}

static inline bool isFuncTypeValType(uint8_t valtype)
{
  switch (valtype) {
  case NQ_WASM_TYPE_I32:
  case NQ_WASM_TYPE_I64:
  case NQ_WASM_TYPE_F32:
  case NQ_WASM_TYPE_F64:
  case NQ_WASM_TYPE_V128:
  case NQ_WASM_TYPE_FUNCREF:
  case NQ_WASM_TYPE_EXTERNREF:
    return true;
  }
  return false;
}

static bool isFuncTypeValTypeArray(const uint8_t* types, uint32_t count)
{
  for (uint32_t i = 0; i < count; i++) {
    if (!isFuncTypeValType(types[i]))
      return false;
  }
  return true;
}

static struct NQWasmFuncType* createFuncType(const uint8_t* params, uint32_t paramCount, const uint8_t* results, uint32_t resultCount)
{
  struct NQWasmFuncType* thiz = (struct NQWasmFuncType*)NQMalloc(sizeof(*thiz) + paramCount + resultCount);
  if (thiz == NULL) {
    NQ_LOGE("No memory");
    return NULL;
  }

  NQListHead_init(&thiz->list);

  uint8_t* ptr = (uint8_t*)thiz + sizeof(*thiz);
  thiz->paramCount = paramCount;
  thiz->params = ptr;
  if (paramCount != 0)
    memcpy(ptr, params, paramCount);
  ptr += paramCount;

  thiz->resultCount = resultCount;
  thiz->results = ptr;
  if (resultCount != 0)
    memcpy(ptr, results, resultCount);

  return thiz;
}

static bool readValTypeVector(NQDataReader* reader, const uint8_t** types, uint32_t* count)
{
  if (!readLeb128Uint32(reader, count))
    return false;

  *types = (const uint8_t*)NQDataReader_currentData(reader);
  if (!NQDataReader_skipAll(reader, *count))
    return false;

  return isFuncTypeValTypeArray(*types, *count);
}

NQWasmTypeSection* NQWasmTypeSection_create(void)
{
  NQWasmTypeSection* thiz = (NQWasmTypeSection*)NQMalloc(sizeof(*thiz));
  if (thiz != NULL)
    NQWasmTypeSection_init(thiz);
  return thiz;
}

static inline void clearFuncTypeList(NQListHead* itemList)
{
  NQListHead* iter = itemList->next;
  while (iter != itemList) {
    NQWasmFuncType* item = NQ_CONTAINER_OF(iter, NQWasmFuncType, list);
    iter = iter->next;
    NQListHead_remove(&item->list);
    NQFree(item);
  }
}

NQWasmTypeSection* NQWasmTypeSection_fromMemory(const void* data, size_t size)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  uint32_t itemCount;
  if (!readLeb128Uint32(&reader, &itemCount)) {
    NQ_LOGE("Type vector count format is wrong");
    return NULL;
  }

  NQListHead itemList;
  NQListHead_init(&itemList);

  size_t itemIndex = 0;
  while (itemIndex < itemCount) {
    uint8_t form;
    if (!NQDataReader_readUint8(&reader, &form)) {
      NQ_LOGE("Unexpected end of type form reached");
      break;
    }
    if (form != NQ_WASM_TYPE_FUNC) {
      NQ_LOGE("Unsupported %02x type form", form);
      break;
    }

    const uint8_t* params;
    uint32_t paramCount;
    if (!readValTypeVector(&reader, &params, &paramCount)) {
      NQ_LOGE("Function type params are wrong");
      break;
    }

    const uint8_t* results;
    uint32_t resultCount;
    if (!readValTypeVector(&reader, &results, &resultCount)) {
      NQ_LOGE("Function type results are wrong");
      break;
    }

    struct NQWasmFuncType* item = createFuncType(params, paramCount, results, resultCount);
    if (item == NULL)
      break;

    NQListHead_addBack(&itemList, &item->list);
    itemIndex++;
  }

  NQWasmTypeSection* thiz = NULL;
  if (itemIndex == itemCount) {
    if (!NQDataReader_isEmpty(&reader))
      NQ_LOGE("Not the entire type section was processed");
    else
      thiz = NQWasmTypeSection_create();
    if (thiz != NULL) {
      NQListHead_swap(&thiz->itemList, &itemList);
      thiz->itemCount = itemCount;
    }
  }

  clearFuncTypeList(&itemList);
  return thiz;
}

void NQWasmTypeSection_init(NQWasmTypeSection* thiz)
{
  thiz->base.sectionId = NQ_WASM_SECTION_TYPE_ID;
  NQListHead_init(&thiz->base.list);
  NQListHead_init(&thiz->itemList);
  thiz->itemCount = 0;
}

void NQWasmTypeSection_finalize(NQWasmTypeSection* thiz)
{
  clearFuncTypeList(&thiz->itemList);
  NQListHead_remove(&thiz->base.list);
}

void NQWasmTypeSection_destroy(NQWasmTypeSection* thiz)
{
  NQWasmTypeSection_finalize(thiz);
  NQFree(thiz);
}

static bool writeTypeSectionContent(const NQWasmTypeSection* thiz, NQWriteWrapper* writer)
{
  if (!writeLeb128Uint32(writer, thiz->itemCount))
    return false;

  struct NQWasmFuncType* item = NQWasmTypeSection_firstItem(thiz);
  while (item != NULL) {
    if (!NQWriteWrapper_writeUint8(writer, NQ_WASM_TYPE_FUNC))
      return false;

    if (!writeLeb128Uint32(writer, item->paramCount))
      return false;
    if (!NQWriteWrapper_writeAll(writer, item->params, item->paramCount))
      return false;

    if (!writeLeb128Uint32(writer, item->resultCount))
      return false;
    if (!NQWriteWrapper_writeAll(writer, item->results, item->resultCount))
      return false;

    item = NQWasmTypeSection_nextItem(thiz, item);
  }

  return true;
}

static bool wasmTypeSectionWriteTo(const NQWasmTypeSection* thiz, NQByteBuffer* tmpBuf, NQWriteWrapper* writer)
{
  NQByteBuffer_resize(tmpBuf, 0);

  NQWriteWrapper tmpWriter;
  NQWriteWrapper_init(&tmpWriter, writeByteBuffer, tmpBuf);

  if (!writeTypeSectionContent(thiz, &tmpWriter))
    return false;

  return writeWasmSection(writer, thiz->base.sectionId, NQByteBuffer_data(tmpBuf), NQByteBuffer_size(tmpBuf));
}

bool NQWasmTypeSection_writeTo(const NQWasmTypeSection* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQByteBuffer buffer;
  NQByteBuffer_init(&buffer);

  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);

  bool res = wasmTypeSectionWriteTo(thiz, &buffer, &writer);
  NQByteBuffer_finalize(&buffer);
  return res;
}

bool NQWasmTypeSection_addFuncType(NQWasmTypeSection* thiz, const uint8_t* params, uint32_t paramCount, const uint8_t* results, uint32_t resultCount)
{
  if (!isFuncTypeValTypeArray(params, paramCount) || !isFuncTypeValTypeArray(results, resultCount)) {
    NQ_LOGE("Unsupported valtype of function type");
    return false;
  }

  NQWasmFuncType* item = createFuncType(params, paramCount, results, resultCount);
  if (item == NULL)
    return false;

  NQListHead_addBack(&thiz->itemList, &item->list);
  thiz->itemCount++;
  return true;
}

NQWasmFuncType* NQWasmTypeSection_getFuncType(const NQWasmTypeSection* thiz, uint32_t typeidx)
{
  NQWasmFuncType* item = NQWasmTypeSection_firstItem(thiz);
  while (item != NULL && typeidx != 0) {
    item = NQWasmTypeSection_nextItem(thiz, item);
    typeidx--;
  }
  return item;
}

NQWasmFunctionSection* NQWasmFunctionSection_create(void)
{
  NQWasmFunctionSection* thiz = (NQWasmFunctionSection*)NQMalloc(sizeof(*thiz));
  if (thiz != NULL)
    NQWasmFunctionSection_init(thiz);
  return thiz;
}

static bool reserveFunctionSection(NQWasmFunctionSection* thiz, uint32_t capacity)
{
  if (capacity <= thiz->capacity)
    return true;

  uint32_t* typeIndices = (uint32_t*)NQRealloc(thiz->typeIndices, (size_t)capacity * sizeof(uint32_t));
  if (typeIndices == NULL) {
    NQ_LOGE("No memory");
    return false;
  }

  thiz->typeIndices = typeIndices;
  thiz->capacity = capacity;
  return true;
}

NQWasmFunctionSection* NQWasmFunctionSection_fromMemory(const void* data, size_t size)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  uint32_t itemCount;
  if (!readLeb128Uint32(&reader, &itemCount)) {
    NQ_LOGE("Function vector count format is wrong");
    return NULL;
  }
  // Every typeidx takes at least one byte
  if (itemCount > NQDataReader_availableSize(&reader)) {
    NQ_LOGE("Function vector count is too big");
    return NULL;
  }

  NQWasmFunctionSection* thiz = NQWasmFunctionSection_create();
  if (thiz == NULL)
    return NULL;

  if (!reserveFunctionSection(thiz, itemCount)) {
    NQWasmFunctionSection_destroy(thiz);
    return NULL;
  }

  for (uint32_t i = 0; i < itemCount; i++) {
    if (!readLeb128Uint32(&reader, &thiz->typeIndices[i])) {
      NQ_LOGE("Function typeidx format is wrong");
      NQWasmFunctionSection_destroy(thiz);
      return NULL;
    }
  }
  thiz->itemCount = itemCount;

  if (!NQDataReader_isEmpty(&reader)) {
    NQ_LOGE("Not the entire function section was processed");
    NQWasmFunctionSection_destroy(thiz);
    return NULL;
  }

  return thiz;
}

void NQWasmFunctionSection_init(NQWasmFunctionSection* thiz)
{
  thiz->base.sectionId = NQ_WASM_SECTION_FUNCTION_ID;
  NQListHead_init(&thiz->base.list);
  thiz->typeIndices = NULL;
  thiz->itemCount = 0;
  thiz->capacity = 0;
}

void NQWasmFunctionSection_finalize(NQWasmFunctionSection* thiz)
{
  NQFree(thiz->typeIndices);
  thiz->typeIndices = NULL;
  thiz->itemCount = 0;
  thiz->capacity = 0;
  NQListHead_remove(&thiz->base.list);
}

void NQWasmFunctionSection_destroy(NQWasmFunctionSection* thiz)
{
  NQWasmFunctionSection_finalize(thiz);
  NQFree(thiz);
}

static bool writeFunctionSectionContent(const NQWasmFunctionSection* thiz, NQWriteWrapper* writer)
{
  if (!writeLeb128Uint32(writer, thiz->itemCount))
    return false;

  for (uint32_t i = 0; i < thiz->itemCount; i++) {
    if (!writeLeb128Uint32(writer, thiz->typeIndices[i]))
      return false;
  }

  return true;
}

static bool wasmFunctionSectionWriteTo(const NQWasmFunctionSection* thiz, NQByteBuffer* tmpBuf, NQWriteWrapper* writer)
{
  NQByteBuffer_resize(tmpBuf, 0);

  NQWriteWrapper tmpWriter;
  NQWriteWrapper_init(&tmpWriter, writeByteBuffer, tmpBuf);

  if (!writeFunctionSectionContent(thiz, &tmpWriter))
    return false;

  return writeWasmSection(writer, thiz->base.sectionId, NQByteBuffer_data(tmpBuf), NQByteBuffer_size(tmpBuf));
}

bool NQWasmFunctionSection_writeTo(const NQWasmFunctionSection* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQByteBuffer buffer;
  NQByteBuffer_init(&buffer);

  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);

  bool res = wasmFunctionSectionWriteTo(thiz, &buffer, &writer);
  NQByteBuffer_finalize(&buffer);
  return res;
}

bool NQWasmFunctionSection_addFunction(NQWasmFunctionSection* thiz, uint32_t typeidx)
{
  if (thiz->itemCount == thiz->capacity) {
    if (thiz->capacity == NQ_UINT32_MAX)
      return false;
    uint32_t capacity = thiz->capacity < 8 ? 8 : (thiz->capacity > NQ_UINT32_MAX / 2 ? NQ_UINT32_MAX : thiz->capacity * 2);
    if (!reserveFunctionSection(thiz, capacity))
      return false;
  }

  thiz->typeIndices[thiz->itemCount++] = typeidx;
  return true;
}

static inline bool isTableRefType(uint8_t reftype)
{
  return reftype == NQ_WASM_TYPE_FUNCREF || reftype == NQ_WASM_TYPE_EXTERNREF;
}

static bool isValidTable(uint8_t reftype, uint8_t tabletype, uint64_t minValue, uint64_t maxValue)
{
  if (!isTableRefType(reftype)) {
    NQ_LOGE("Unsupported %02x reftype of table", reftype);
    return false;
  }
  if (!NQWasmIsTableType(tabletype)) {
    NQ_LOGE("Unknown %02x limits of table", tabletype);
    return false;
  }
  if (!(tabletype & NQ_WASM_TABLETYPE_TABLE64) && (minValue > NQ_UINT32_MAX || maxValue > NQ_UINT32_MAX)) {
    NQ_LOGE("Limits of table32 are out of range");
    return false;
  }
  if ((tabletype & NQ_WASM_TABLETYPE_MAXVAL) && minValue > maxValue) {
    NQ_LOGE("Minimum value of table is bigger than maximum");
    return false;
  }
  return true;
}

NQWasmTableSection* NQWasmTableSection_create(void)
{
  NQWasmTableSection* thiz = (NQWasmTableSection*)NQMalloc(sizeof(*thiz));
  if (thiz != NULL)
    NQWasmTableSection_init(thiz);
  return thiz;
}

static bool reserveTableSection(NQWasmTableSection* thiz, uint32_t capacity)
{
  if (capacity <= thiz->capacity)
    return true;

  NQWasmTable* tables = (NQWasmTable*)NQRealloc(thiz->tables, (size_t)capacity * sizeof(NQWasmTable));
  if (tables == NULL) {
    NQ_LOGE("No memory");
    return false;
  }

  thiz->tables = tables;
  thiz->capacity = capacity;
  return true;
}

static bool readTable(NQDataReader* reader, NQWasmTable* table)
{
  if (!NQDataReader_readUint8(reader, &table->reftype)) {
    NQ_LOGE("Unexpected end of table reftype reached");
    return false;
  }
  if (!NQDataReader_readUint8(reader, &table->tabletype)) {
    NQ_LOGE("Unexpected end of table limits reached");
    return false;
  }
  if (!readLeb128Uint64(reader, &table->minValue)) {
    NQ_LOGE("Minimum value of table is wrong");
    return false;
  }
  if (table->tabletype & NQ_WASM_TABLETYPE_MAXVAL) {
    if (!readLeb128Uint64(reader, &table->maxValue)) {
      NQ_LOGE("Maximum value of table is wrong");
      return false;
    }
  }
  else {
    table->maxValue = 0;
  }

  return isValidTable(table->reftype, table->tabletype, table->minValue, table->maxValue);
}

NQWasmTableSection* NQWasmTableSection_fromMemory(const void* data, size_t size)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  uint32_t itemCount;
  if (!readLeb128Uint32(&reader, &itemCount)) {
    NQ_LOGE("Table vector count format is wrong");
    return NULL;
  }
  // Every table takes at least three bytes
  if (itemCount > NQDataReader_availableSize(&reader) / 3) {
    NQ_LOGE("Table vector count is too big");
    return NULL;
  }

  NQWasmTableSection* thiz = NQWasmTableSection_create();
  if (thiz == NULL)
    return NULL;

  if (!reserveTableSection(thiz, itemCount)) {
    NQWasmTableSection_destroy(thiz);
    return NULL;
  }

  for (uint32_t i = 0; i < itemCount; i++) {
    if (!readTable(&reader, &thiz->tables[i])) {
      NQWasmTableSection_destroy(thiz);
      return NULL;
    }
  }
  thiz->itemCount = itemCount;

  if (!NQDataReader_isEmpty(&reader)) {
    NQ_LOGE("Not the entire table section was processed");
    NQWasmTableSection_destroy(thiz);
    return NULL;
  }

  return thiz;
}

void NQWasmTableSection_init(NQWasmTableSection* thiz)
{
  thiz->base.sectionId = NQ_WASM_SECTION_TABLE_ID;
  NQListHead_init(&thiz->base.list);
  thiz->tables = NULL;
  thiz->itemCount = 0;
  thiz->capacity = 0;
}

void NQWasmTableSection_finalize(NQWasmTableSection* thiz)
{
  NQFree(thiz->tables);
  thiz->tables = NULL;
  thiz->itemCount = 0;
  thiz->capacity = 0;
  NQListHead_remove(&thiz->base.list);
}

void NQWasmTableSection_destroy(NQWasmTableSection* thiz)
{
  NQWasmTableSection_finalize(thiz);
  NQFree(thiz);
}

static bool writeTableSectionContent(const NQWasmTableSection* thiz, NQWriteWrapper* writer)
{
  if (!writeLeb128Uint32(writer, thiz->itemCount))
    return false;

  for (uint32_t i = 0; i < thiz->itemCount; i++) {
    const NQWasmTable* table = &thiz->tables[i];
    if (!NQWriteWrapper_writeUint8(writer, table->reftype))
      return false;
    if (!NQWriteWrapper_writeUint8(writer, table->tabletype))
      return false;
    if (!writeLeb128Uint64(writer, table->minValue))
      return false;
    if (table->tabletype & NQ_WASM_TABLETYPE_MAXVAL) {
      if (!writeLeb128Uint64(writer, table->maxValue))
        return false;
    }
  }

  return true;
}

static bool wasmTableSectionWriteTo(const NQWasmTableSection* thiz, NQByteBuffer* tmpBuf, NQWriteWrapper* writer)
{
  NQByteBuffer_resize(tmpBuf, 0);

  NQWriteWrapper tmpWriter;
  NQWriteWrapper_init(&tmpWriter, writeByteBuffer, tmpBuf);

  if (!writeTableSectionContent(thiz, &tmpWriter))
    return false;

  return writeWasmSection(writer, thiz->base.sectionId, NQByteBuffer_data(tmpBuf), NQByteBuffer_size(tmpBuf));
}

bool NQWasmTableSection_writeTo(const NQWasmTableSection* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQByteBuffer buffer;
  NQByteBuffer_init(&buffer);

  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);

  bool res = wasmTableSectionWriteTo(thiz, &buffer, &writer);
  NQByteBuffer_finalize(&buffer);
  return res;
}

bool NQWasmTableSection_addTable(NQWasmTableSection* thiz, uint8_t reftype, uint8_t tabletype, uint64_t minValue, uint64_t maxValue)
{
  if (!(tabletype & NQ_WASM_TABLETYPE_MAXVAL))
    maxValue = 0;
  if (!isValidTable(reftype, tabletype, minValue, maxValue))
    return false;

  if (thiz->itemCount == thiz->capacity) {
    if (thiz->capacity == NQ_UINT32_MAX)
      return false;
    uint32_t capacity = thiz->capacity < 4 ? 4 : (thiz->capacity > NQ_UINT32_MAX / 2 ? NQ_UINT32_MAX : thiz->capacity * 2);
    if (!reserveTableSection(thiz, capacity))
      return false;
  }

  NQWasmTable* table = &thiz->tables[thiz->itemCount++];
  table->reftype = reftype;
  table->tabletype = tabletype;
  table->minValue = minValue;
  table->maxValue = maxValue;
  return true;
}

static bool isValidMemory(uint8_t memtype, uint64_t minValue, uint64_t maxValue)
{
  if (!NQWasmIsMemType(memtype)) {
    NQ_LOGE("Unknown %02x limits of memory", memtype);
    return false;
  }

  uint64_t maxPages = (memtype & NQ_WASM_MEMTYPE_WASM64) ? NQ_WASM_MEMORY64_MAX_PAGES : NQ_WASM_MEMORY32_MAX_PAGES;
  if (minValue > maxPages) {
    NQ_LOGE("Minimum value of memory is out of range");
    return false;
  }
  if (memtype & NQ_WASM_MEMTYPE_MAXVAL) {
    if (maxValue > maxPages) {
      NQ_LOGE("Maximum value of memory is out of range");
      return false;
    }
    if (minValue > maxValue) {
      NQ_LOGE("Minimum value of memory is bigger than maximum");
      return false;
    }
  }
  else if (memtype & NQ_WASM_MEMTYPE_SHARED) {
    NQ_LOGE("Shared memory must have maximum value");
    return false;
  }
  return true;
}

NQWasmMemorySection* NQWasmMemorySection_create(void)
{
  NQWasmMemorySection* thiz = (NQWasmMemorySection*)NQMalloc(sizeof(*thiz));
  if (thiz != NULL)
    NQWasmMemorySection_init(thiz);
  return thiz;
}

static bool reserveMemorySection(NQWasmMemorySection* thiz, uint32_t capacity)
{
  if (capacity <= thiz->capacity)
    return true;

  NQWasmMemory* memories = (NQWasmMemory*)NQRealloc(thiz->memories, (size_t)capacity * sizeof(NQWasmMemory));
  if (memories == NULL) {
    NQ_LOGE("No memory");
    return false;
  }

  thiz->memories = memories;
  thiz->capacity = capacity;
  return true;
}

static bool readMemory(NQDataReader* reader, NQWasmMemory* memory)
{
  if (!NQDataReader_readUint8(reader, &memory->memtype)) {
    NQ_LOGE("Unexpected end of memtype reached");
    return false;
  }
  if (!readLeb128Uint64(reader, &memory->minValue)) {
    NQ_LOGE("Minimum value of memory is wrong");
    return false;
  }
  if (memory->memtype & NQ_WASM_MEMTYPE_MAXVAL) {
    if (!readLeb128Uint64(reader, &memory->maxValue)) {
      NQ_LOGE("Maximum value of memory is wrong");
      return false;
    }
  }
  else {
    memory->maxValue = 0;
  }

  return isValidMemory(memory->memtype, memory->minValue, memory->maxValue);
}

NQWasmMemorySection* NQWasmMemorySection_fromMemory(const void* data, size_t size)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  uint32_t itemCount;
  if (!readLeb128Uint32(&reader, &itemCount)) {
    NQ_LOGE("Memory vector count format is wrong");
    return NULL;
  }
  // Every memory takes at least two bytes
  if (itemCount > NQDataReader_availableSize(&reader) / 2) {
    NQ_LOGE("Memory vector count is too big");
    return NULL;
  }

  NQWasmMemorySection* thiz = NQWasmMemorySection_create();
  if (thiz == NULL)
    return NULL;

  if (!reserveMemorySection(thiz, itemCount)) {
    NQWasmMemorySection_destroy(thiz);
    return NULL;
  }

  for (uint32_t i = 0; i < itemCount; i++) {
    if (!readMemory(&reader, &thiz->memories[i])) {
      NQWasmMemorySection_destroy(thiz);
      return NULL;
    }
  }
  thiz->itemCount = itemCount;

  if (!NQDataReader_isEmpty(&reader)) {
    NQ_LOGE("Not the entire memory section was processed");
    NQWasmMemorySection_destroy(thiz);
    return NULL;
  }

  return thiz;
}

void NQWasmMemorySection_init(NQWasmMemorySection* thiz)
{
  thiz->base.sectionId = NQ_WASM_SECTION_MEMORY_ID;
  NQListHead_init(&thiz->base.list);
  thiz->memories = NULL;
  thiz->itemCount = 0;
  thiz->capacity = 0;
}

void NQWasmMemorySection_finalize(NQWasmMemorySection* thiz)
{
  NQFree(thiz->memories);
  thiz->memories = NULL;
  thiz->itemCount = 0;
  thiz->capacity = 0;
  NQListHead_remove(&thiz->base.list);
}

void NQWasmMemorySection_destroy(NQWasmMemorySection* thiz)
{
  NQWasmMemorySection_finalize(thiz);
  NQFree(thiz);
}

static bool writeMemorySectionContent(const NQWasmMemorySection* thiz, NQWriteWrapper* writer)
{
  if (!writeLeb128Uint32(writer, thiz->itemCount))
    return false;

  for (uint32_t i = 0; i < thiz->itemCount; i++) {
    const NQWasmMemory* memory = &thiz->memories[i];
    if (!NQWriteWrapper_writeUint8(writer, memory->memtype))
      return false;
    if (!writeLeb128Uint64(writer, memory->minValue))
      return false;
    if (memory->memtype & NQ_WASM_MEMTYPE_MAXVAL) {
      if (!writeLeb128Uint64(writer, memory->maxValue))
        return false;
    }
  }

  return true;
}

static bool wasmMemorySectionWriteTo(const NQWasmMemorySection* thiz, NQByteBuffer* tmpBuf, NQWriteWrapper* writer)
{
  NQByteBuffer_resize(tmpBuf, 0);

  NQWriteWrapper tmpWriter;
  NQWriteWrapper_init(&tmpWriter, writeByteBuffer, tmpBuf);

  if (!writeMemorySectionContent(thiz, &tmpWriter))
    return false;

  return writeWasmSection(writer, thiz->base.sectionId, NQByteBuffer_data(tmpBuf), NQByteBuffer_size(tmpBuf));
}

bool NQWasmMemorySection_writeTo(const NQWasmMemorySection* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQByteBuffer buffer;
  NQByteBuffer_init(&buffer);

  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);

  bool res = wasmMemorySectionWriteTo(thiz, &buffer, &writer);
  NQByteBuffer_finalize(&buffer);
  return res;
}

bool NQWasmMemorySection_addMemory(NQWasmMemorySection* thiz, uint8_t memtype, uint64_t minValue, uint64_t maxValue)
{
  if (!(memtype & NQ_WASM_MEMTYPE_MAXVAL))
    maxValue = 0;
  if (!isValidMemory(memtype, minValue, maxValue))
    return false;

  if (thiz->itemCount == thiz->capacity) {
    if (thiz->capacity == NQ_UINT32_MAX)
      return false;
    uint32_t capacity = thiz->capacity < 4 ? 4 : (thiz->capacity > NQ_UINT32_MAX / 2 ? NQ_UINT32_MAX : thiz->capacity * 2);
    if (!reserveMemorySection(thiz, capacity))
      return false;
  }

  NQWasmMemory* memory = &thiz->memories[thiz->itemCount++];
  memory->memtype = memtype;
  memory->minValue = minValue;
  memory->maxValue = maxValue;
  return true;
}

static bool skipLeb128(NQDataReader* reader, size_t maxSize)
{
  uint8_t byte;
  for (size_t i = 0; i < maxSize; i++) {
    if (!NQDataReader_readUint8(reader, &byte))
      return false;
    if (!(byte & 0x80))
      return true;
  }
  return false;
}

// Walks a constant expression up to and including its end opcode
static bool skipConstExpr(NQDataReader* reader)
{
  uint8_t opcode;
  uint32_t value;

  for (;;) {
    if (!NQDataReader_readUint8(reader, &opcode)) {
      NQ_LOGE("Unexpected end of constant expression reached");
      return false;
    }

    switch (opcode) {
    case NQ_WASM_OP_End:
      return true;

    case NQ_WASM_OP_I32Const:
      if (!skipLeb128(reader, 5))
        return false;
      break;

    case NQ_WASM_OP_I64Const:
      if (!skipLeb128(reader, 10))
        return false;
      break;

    case NQ_WASM_OP_F32Const:
      if (!NQDataReader_skipAll(reader, 4))
        return false;
      break;

    case NQ_WASM_OP_F64Const:
      if (!NQDataReader_skipAll(reader, 8))
        return false;
      break;

    case NQ_WASM_OP_GlobalGet:
    case NQ_WASM_OP_RefFunc:
      if (!readLeb128Uint32(reader, &value))
        return false;
      break;

    case NQ_WASM_OP_RefNull:
      if (!skipLeb128(reader, 5)) // heaptype is s33
        return false;
      break;

    case NQ_WASM_OP_I32Add:
    case NQ_WASM_OP_I32Sub:
    case NQ_WASM_OP_I32Mul:
    case NQ_WASM_OP_I64Add:
    case NQ_WASM_OP_I64Sub:
    case NQ_WASM_OP_I64Mul:
      break;

    case NQ_WASM_OP_SimdPrefix:
      if (!readLeb128Uint32(reader, &value))
        return false;
      if (value != NQ_WASM_OP_V128Const) {
        NQ_LOGE("Unsupported %02x:%u opcode in constant expression", opcode, value);
        return false;
      }
      if (!NQDataReader_skipAll(reader, 16))
        return false;
      break;

    default:
      NQ_LOGE("Unsupported %02x opcode in constant expression", opcode);
      return false;
    }
  }
}

static bool isValidGlobalType(uint8_t valtype, uint8_t mut)
{
  if (!isFuncTypeValType(valtype)) {
    NQ_LOGE("Unsupported %02x valtype of global", valtype);
    return false;
  }
  if (mut > 1) {
    NQ_LOGE("Unknown %02x mutability of global", mut);
    return false;
  }
  return true;
}

static struct NQWasmGlobal* createGlobal(uint8_t valtype, uint8_t mut, const void* expr, uint32_t exprSize)
{
  struct NQWasmGlobal* thiz = (struct NQWasmGlobal*)NQMalloc(sizeof(*thiz) + exprSize);
  if (thiz == NULL) {
    NQ_LOGE("No memory");
    return NULL;
  }

  NQListHead_init(&thiz->list);
  thiz->valtype = valtype;
  thiz->mut = mut;

  uint8_t* ptr = (uint8_t*)thiz + sizeof(*thiz);
  memcpy(ptr, expr, exprSize);
  thiz->expr = ptr;
  thiz->exprSize = exprSize;

  return thiz;
}

NQWasmGlobalSection* NQWasmGlobalSection_create(void)
{
  NQWasmGlobalSection* thiz = (NQWasmGlobalSection*)NQMalloc(sizeof(*thiz));
  if (thiz != NULL)
    NQWasmGlobalSection_init(thiz);
  return thiz;
}

static inline void clearGlobalList(NQListHead* itemList)
{
  NQListHead* iter = itemList->next;
  while (iter != itemList) {
    NQWasmGlobal* item = NQ_CONTAINER_OF(iter, NQWasmGlobal, list);
    iter = iter->next;
    NQListHead_remove(&item->list);
    NQFree(item);
  }
}

NQWasmGlobalSection* NQWasmGlobalSection_fromMemory(const void* data, size_t size)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  uint32_t itemCount;
  if (!readLeb128Uint32(&reader, &itemCount)) {
    NQ_LOGE("Global vector count format is wrong");
    return NULL;
  }

  NQListHead itemList;
  NQListHead_init(&itemList);

  size_t itemIndex = 0;
  while (itemIndex < itemCount) {
    uint8_t valtype;
    uint8_t mut;

    if (!NQDataReader_readUint8(&reader, &valtype)) {
      NQ_LOGE("Unexpected end of global valtype reached");
      break;
    }
    if (!NQDataReader_readUint8(&reader, &mut)) {
      NQ_LOGE("Unexpected end of global mutability reached");
      break;
    }
    if (!isValidGlobalType(valtype, mut))
      break;

    const uint8_t* expr = (const uint8_t*)NQDataReader_currentData(&reader);
    if (!skipConstExpr(&reader)) {
      NQ_LOGE("Global init expression is wrong");
      break;
    }

    uint32_t exprSize = (uint32_t)((const uint8_t*)NQDataReader_currentData(&reader) - expr);
    struct NQWasmGlobal* item = createGlobal(valtype, mut, expr, exprSize);
    if (item == NULL)
      break;

    NQListHead_addBack(&itemList, &item->list);
    itemIndex++;
  }

  NQWasmGlobalSection* thiz = NULL;
  if (itemIndex == itemCount) {
    if (!NQDataReader_isEmpty(&reader))
      NQ_LOGE("Not the entire global section was processed");
    else
      thiz = NQWasmGlobalSection_create();
    if (thiz != NULL) {
      NQListHead_swap(&thiz->itemList, &itemList);
      thiz->itemCount = itemCount;
    }
  }

  clearGlobalList(&itemList);
  return thiz;
}

void NQWasmGlobalSection_init(NQWasmGlobalSection* thiz)
{
  thiz->base.sectionId = NQ_WASM_SECTION_GLOBAL_ID;
  NQListHead_init(&thiz->base.list);
  NQListHead_init(&thiz->itemList);
  thiz->itemCount = 0;
}

void NQWasmGlobalSection_finalize(NQWasmGlobalSection* thiz)
{
  clearGlobalList(&thiz->itemList);
  NQListHead_remove(&thiz->base.list);
}

void NQWasmGlobalSection_destroy(NQWasmGlobalSection* thiz)
{
  NQWasmGlobalSection_finalize(thiz);
  NQFree(thiz);
}

static bool writeGlobalSectionContent(const NQWasmGlobalSection* thiz, NQWriteWrapper* writer)
{
  if (!writeLeb128Uint32(writer, thiz->itemCount))
    return false;

  struct NQWasmGlobal* item = NQWasmGlobalSection_firstItem(thiz);
  while (item != NULL) {
    if (!NQWriteWrapper_writeUint8(writer, item->valtype))
      return false;
    if (!NQWriteWrapper_writeUint8(writer, item->mut))
      return false;
    if (!NQWriteWrapper_writeAll(writer, item->expr, item->exprSize))
      return false;

    item = NQWasmGlobalSection_nextItem(thiz, item);
  }

  return true;
}

static bool wasmGlobalSectionWriteTo(const NQWasmGlobalSection* thiz, NQByteBuffer* tmpBuf, NQWriteWrapper* writer)
{
  NQByteBuffer_resize(tmpBuf, 0);

  NQWriteWrapper tmpWriter;
  NQWriteWrapper_init(&tmpWriter, writeByteBuffer, tmpBuf);

  if (!writeGlobalSectionContent(thiz, &tmpWriter))
    return false;

  return writeWasmSection(writer, thiz->base.sectionId, NQByteBuffer_data(tmpBuf), NQByteBuffer_size(tmpBuf));
}

bool NQWasmGlobalSection_writeTo(const NQWasmGlobalSection* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQByteBuffer buffer;
  NQByteBuffer_init(&buffer);

  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);

  bool res = wasmGlobalSectionWriteTo(thiz, &buffer, &writer);
  NQByteBuffer_finalize(&buffer);
  return res;
}

bool NQWasmGlobalSection_addGlobal(NQWasmGlobalSection* thiz, uint8_t valtype, uint8_t mut, const void* expr, size_t exprSize)
{
  if (!isValidGlobalType(valtype, mut))
    return false;

  NQDataReader reader;
  NQDataReader_init(&reader, expr, exprSize);
  if (!skipConstExpr(&reader) || !NQDataReader_isEmpty(&reader)) {
    NQ_LOGE("Global init expression is wrong");
    return false;
  }

  NQWasmGlobal* item = createGlobal(valtype, mut, expr, (uint32_t)exprSize);
  if (item == NULL)
    return false;

  NQListHead_addBack(&thiz->itemList, &item->list);
  thiz->itemCount++;
  return true;
}

NQWasmGlobal* NQWasmGlobalSection_getGlobal(const NQWasmGlobalSection* thiz, uint32_t index)
{
  NQWasmGlobal* item = NQWasmGlobalSection_firstItem(thiz);
  while (item != NULL && index != 0) {
    item = NQWasmGlobalSection_nextItem(thiz, item);
    index--;
  }
  return item;
}

NQWasmStartSection* NQWasmStartSection_create(uint32_t funcidx)
{
  NQWasmStartSection* thiz = (NQWasmStartSection*)NQMalloc(sizeof(*thiz));
  if (thiz != NULL)
    NQWasmStartSection_init(thiz, funcidx);
  return thiz;
}

NQWasmStartSection* NQWasmStartSection_fromMemory(const void* data, size_t size)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  uint32_t funcidx;
  if (!readLeb128Uint32(&reader, &funcidx)) {
    NQ_LOGE("Start funcidx format is wrong");
    return NULL;
  }
  if (!NQDataReader_isEmpty(&reader)) {
    NQ_LOGE("Not the entire start section was processed");
    return NULL;
  }

  return NQWasmStartSection_create(funcidx);
}

void NQWasmStartSection_init(NQWasmStartSection* thiz, uint32_t funcidx)
{
  thiz->base.sectionId = NQ_WASM_SECTION_START_ID;
  NQListHead_init(&thiz->base.list);
  thiz->funcidx = funcidx;
}

void NQWasmStartSection_finalize(NQWasmStartSection* thiz)
{
  NQListHead_remove(&thiz->base.list);
}

void NQWasmStartSection_destroy(NQWasmStartSection* thiz)
{
  NQWasmStartSection_finalize(thiz);
  NQFree(thiz);
}

static bool wasmStartSectionWriteTo(const NQWasmStartSection* thiz, NQWriteWrapper* writer)
{
  NQLeb128Buffer buffer;
  size_t size = NQLeb128EncodeUint32(buffer, sizeof(buffer), thiz->funcidx);
  return writeWasmSection(writer, thiz->base.sectionId, buffer, (uint32_t)size);
}

bool NQWasmStartSection_writeTo(const NQWasmStartSection* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);
  return wasmStartSectionWriteTo(thiz, &writer);
}

#define NQ_WASM_ELEMKIND_FUNCREF 0x00

static bool isValidConstExprList(const uint8_t* data, size_t size, uint32_t count)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  for (uint32_t i = 0; i < count; i++) {
    if (!skipConstExpr(&reader))
      return false;
  }
  return NQDataReader_isEmpty(&reader);
}

static bool isValidElement(const NQWasmElement* thiz)
{
  if (thiz->flags > NQ_WASM_ELEM_FLAGS_MAX) {
    NQ_LOGE("Unknown %u flags of element", thiz->flags);
    return false;
  }

  if (thiz->flags & NQ_WASM_ELEM_FLAG_EXPRS) {
    if (!isTableRefType(thiz->reftype)) {
      NQ_LOGE("Unsupported %02x reftype of element", thiz->reftype);
      return false;
    }
    if (!isValidConstExprList(thiz->exprs, thiz->exprsSize, thiz->itemCount)) {
      NQ_LOGE("Element init expressions are wrong");
      return false;
    }
  }
  else {
    if (thiz->reftype != NQ_WASM_TYPE_FUNCREF) {
      NQ_LOGE("Element of function indices must be funcref");
      return false;
    }
    if (thiz->itemCount > NQ_UINT32_MAX / sizeof(uint32_t) || (thiz->itemCount != 0 && thiz->funcIndices == NULL)) {
      NQ_LOGE("Element function indices are wrong");
      return false;
    }
  }

  if (NQWasmElement_isActive(thiz)) {
    if (!(thiz->flags & NQ_WASM_ELEM_FLAG_EXPLICIT_TABLE) && thiz->tableidx != 0) {
      NQ_LOGE("Element with implicit table must use table 0");
      return false;
    }
    if (!(thiz->flags & NQ_WASM_ELEM_FLAG_EXPLICIT_TABLE) && (thiz->flags & NQ_WASM_ELEM_FLAG_EXPRS) && thiz->reftype != NQ_WASM_TYPE_FUNCREF) {
      NQ_LOGE("Element with implicit reftype must be funcref");
      return false;
    }
    if (!isValidConstExprList(thiz->offset, thiz->offsetSize, 1)) {
      NQ_LOGE("Element offset expression is wrong");
      return false;
    }
  }
  else {
    if (thiz->tableidx != 0 || thiz->offsetSize != 0) {
      NQ_LOGE("Only active element can have table and offset");
      return false;
    }
  }

  return true;
}

// Expects validated element
static struct NQWasmElement* createElement(const NQWasmElement* src)
{
  bool hasExprs = src->flags & NQ_WASM_ELEM_FLAG_EXPRS;
  size_t funcIndicesSize = hasExprs ? 0 : (size_t)src->itemCount * sizeof(uint32_t);
  size_t exprsSize = hasExprs ? src->exprsSize : 0;

  struct NQWasmElement* thiz = (struct NQWasmElement*)NQMalloc(sizeof(*thiz) + funcIndicesSize + src->offsetSize + exprsSize);
  if (thiz == NULL) {
    NQ_LOGE("No memory");
    return NULL;
  }

  NQListHead_init(&thiz->list);
  thiz->flags = src->flags;
  thiz->reftype = src->reftype;
  thiz->tableidx = src->tableidx;
  thiz->itemCount = src->itemCount;

  uint8_t* ptr = (uint8_t*)thiz + sizeof(*thiz);

  thiz->funcIndices = hasExprs ? NULL : (const uint32_t*)ptr;
  if (funcIndicesSize != 0)
    memcpy(ptr, src->funcIndices, funcIndicesSize);
  ptr += funcIndicesSize;

  thiz->offsetSize = src->offsetSize;
  thiz->offset = src->offsetSize != 0 ? ptr : NULL;
  if (src->offsetSize != 0)
    memcpy(ptr, src->offset, src->offsetSize);
  ptr += src->offsetSize;

  thiz->exprsSize = (uint32_t)exprsSize;
  thiz->exprs = hasExprs ? ptr : NULL;
  if (exprsSize != 0)
    memcpy(ptr, src->exprs, exprsSize);

  return thiz;
}

static struct NQWasmElement* readElement(NQDataReader* reader)
{
  NQWasmElement element;
  memset(&element, 0, sizeof(element));
  element.reftype = NQ_WASM_TYPE_FUNCREF;

  uint32_t flags;
  if (!readLeb128Uint32(reader, &flags) || flags > NQ_WASM_ELEM_FLAGS_MAX) {
    NQ_LOGE("Element flags are wrong");
    return NULL;
  }
  element.flags = (uint8_t)flags;

  if (NQWasmElement_isActive(&element)) {
    if ((flags & NQ_WASM_ELEM_FLAG_EXPLICIT_TABLE) && !readLeb128Uint32(reader, &element.tableidx)) {
      NQ_LOGE("Element tableidx format is wrong");
      return NULL;
    }
    element.offset = (const uint8_t*)NQDataReader_currentData(reader);
    if (!skipConstExpr(reader)) {
      NQ_LOGE("Element offset expression is wrong");
      return NULL;
    }
    element.offsetSize = (uint32_t)((const uint8_t*)NQDataReader_currentData(reader) - element.offset);
  }

  // Only flags 0 and 4 have no elemkind/reftype byte
  if (flags & (NQ_WASM_ELEM_FLAG_PASSIVE | NQ_WASM_ELEM_FLAG_EXPLICIT_TABLE)) {
    uint8_t kind;
    if (!NQDataReader_readUint8(reader, &kind)) {
      NQ_LOGE("Unexpected end of element kind reached");
      return NULL;
    }
    if (flags & NQ_WASM_ELEM_FLAG_EXPRS)
      element.reftype = kind;
    else if (kind != NQ_WASM_ELEMKIND_FUNCREF) {
      NQ_LOGE("Unknown %02x elemkind of element", kind);
      return NULL;
    }
  }

  if (!readLeb128Uint32(reader, &element.itemCount)) {
    NQ_LOGE("Element vector count format is wrong");
    return NULL;
  }

  uint32_t* funcIndices = NULL;
  if (flags & NQ_WASM_ELEM_FLAG_EXPRS) {
    element.exprs = (const uint8_t*)NQDataReader_currentData(reader);
    for (uint32_t i = 0; i < element.itemCount; i++) {
      if (!skipConstExpr(reader)) {
        NQ_LOGE("Element init expression is wrong");
        return NULL;
      }
    }
    element.exprsSize = (uint32_t)((const uint8_t*)NQDataReader_currentData(reader) - element.exprs);
  }
  else if (element.itemCount != 0) {
    // Every funcidx takes at least one byte
    if (element.itemCount > NQDataReader_availableSize(reader)) {
      NQ_LOGE("Element vector count is too big");
      return NULL;
    }
    funcIndices = (uint32_t*)NQMalloc((size_t)element.itemCount * sizeof(uint32_t));
    if (funcIndices == NULL) {
      NQ_LOGE("No memory");
      return NULL;
    }
    for (uint32_t i = 0; i < element.itemCount; i++) {
      if (!readLeb128Uint32(reader, &funcIndices[i])) {
        NQ_LOGE("Element funcidx format is wrong");
        NQFree(funcIndices);
        return NULL;
      }
    }
    element.funcIndices = funcIndices;
  }

  struct NQWasmElement* item = isValidElement(&element) ? createElement(&element) : NULL;
  NQFree(funcIndices);
  return item;
}

NQWasmElementSection* NQWasmElementSection_create(void)
{
  NQWasmElementSection* thiz = (NQWasmElementSection*)NQMalloc(sizeof(*thiz));
  if (thiz != NULL)
    NQWasmElementSection_init(thiz);
  return thiz;
}

static inline void clearElementList(NQListHead* itemList)
{
  NQListHead* iter = itemList->next;
  while (iter != itemList) {
    NQWasmElement* item = NQ_CONTAINER_OF(iter, NQWasmElement, list);
    iter = iter->next;
    NQListHead_remove(&item->list);
    NQFree(item);
  }
}

NQWasmElementSection* NQWasmElementSection_fromMemory(const void* data, size_t size)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  uint32_t itemCount;
  if (!readLeb128Uint32(&reader, &itemCount)) {
    NQ_LOGE("Element section vector count format is wrong");
    return NULL;
  }

  NQListHead itemList;
  NQListHead_init(&itemList);

  size_t itemIndex = 0;
  while (itemIndex < itemCount) {
    struct NQWasmElement* item = readElement(&reader);
    if (item == NULL)
      break;

    NQListHead_addBack(&itemList, &item->list);
    itemIndex++;
  }

  NQWasmElementSection* thiz = NULL;
  if (itemIndex == itemCount) {
    if (!NQDataReader_isEmpty(&reader))
      NQ_LOGE("Not the entire element section was processed");
    else
      thiz = NQWasmElementSection_create();
    if (thiz != NULL) {
      NQListHead_swap(&thiz->itemList, &itemList);
      thiz->itemCount = itemCount;
    }
  }

  clearElementList(&itemList);
  return thiz;
}

void NQWasmElementSection_init(NQWasmElementSection* thiz)
{
  thiz->base.sectionId = NQ_WASM_SECTION_ELEMENT_ID;
  NQListHead_init(&thiz->base.list);
  NQListHead_init(&thiz->itemList);
  thiz->itemCount = 0;
}

void NQWasmElementSection_finalize(NQWasmElementSection* thiz)
{
  clearElementList(&thiz->itemList);
  NQListHead_remove(&thiz->base.list);
}

void NQWasmElementSection_destroy(NQWasmElementSection* thiz)
{
  NQWasmElementSection_finalize(thiz);
  NQFree(thiz);
}

static bool writeElementSectionContent(const NQWasmElementSection* thiz, NQWriteWrapper* writer)
{
  if (!writeLeb128Uint32(writer, thiz->itemCount))
    return false;

  struct NQWasmElement* item = NQWasmElementSection_firstItem(thiz);
  while (item != NULL) {
    if (!writeLeb128Uint32(writer, item->flags))
      return false;

    if (NQWasmElement_isActive(item)) {
      if ((item->flags & NQ_WASM_ELEM_FLAG_EXPLICIT_TABLE) && !writeLeb128Uint32(writer, item->tableidx))
        return false;
      if (!NQWriteWrapper_writeAll(writer, item->offset, item->offsetSize))
        return false;
    }

    if (item->flags & (NQ_WASM_ELEM_FLAG_PASSIVE | NQ_WASM_ELEM_FLAG_EXPLICIT_TABLE)) {
      uint8_t kind = (item->flags & NQ_WASM_ELEM_FLAG_EXPRS) ? item->reftype : NQ_WASM_ELEMKIND_FUNCREF;
      if (!NQWriteWrapper_writeUint8(writer, kind))
        return false;
    }

    if (!writeLeb128Uint32(writer, item->itemCount))
      return false;

    if (item->flags & NQ_WASM_ELEM_FLAG_EXPRS) {
      if (!NQWriteWrapper_writeAll(writer, item->exprs, item->exprsSize))
        return false;
    }
    else {
      for (uint32_t i = 0; i < item->itemCount; i++) {
        if (!writeLeb128Uint32(writer, item->funcIndices[i]))
          return false;
      }
    }

    item = NQWasmElementSection_nextItem(thiz, item);
  }

  return true;
}

static bool wasmElementSectionWriteTo(const NQWasmElementSection* thiz, NQByteBuffer* tmpBuf, NQWriteWrapper* writer)
{
  NQByteBuffer_resize(tmpBuf, 0);

  NQWriteWrapper tmpWriter;
  NQWriteWrapper_init(&tmpWriter, writeByteBuffer, tmpBuf);

  if (!writeElementSectionContent(thiz, &tmpWriter))
    return false;

  return writeWasmSection(writer, thiz->base.sectionId, NQByteBuffer_data(tmpBuf), NQByteBuffer_size(tmpBuf));
}

bool NQWasmElementSection_writeTo(const NQWasmElementSection* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQByteBuffer buffer;
  NQByteBuffer_init(&buffer);

  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);

  bool res = wasmElementSectionWriteTo(thiz, &buffer, &writer);
  NQByteBuffer_finalize(&buffer);
  return res;
}

bool NQWasmElementSection_addElement(NQWasmElementSection* thiz, const NQWasmElement* element)
{
  if (!isValidElement(element))
    return false;

  NQWasmElement* item = createElement(element);
  if (item == NULL)
    return false;

  NQListHead_addBack(&thiz->itemList, &item->list);
  thiz->itemCount++;
  return true;
}

NQWasmElement* NQWasmElementSection_getElement(const NQWasmElementSection* thiz, uint32_t index)
{
  NQWasmElement* item = NQWasmElementSection_firstItem(thiz);
  while (item != NULL && index != 0) {
    item = NQWasmElementSection_nextItem(thiz, item);
    index--;
  }
  return item;
}

static bool isValidCode(const NQWasmLocal* locals, uint32_t localGroupCount, const uint8_t* expr, size_t exprSize)
{
  uint64_t localCount = 0;
  for (uint32_t i = 0; i < localGroupCount; i++) {
    if (!isFuncTypeValType(locals[i].valtype)) {
      NQ_LOGE("Unsupported %02x valtype of local", locals[i].valtype);
      return false;
    }
    localCount += locals[i].count;
  }
  if (localCount > NQ_UINT32_MAX) {
    NQ_LOGE("Too many locals");
    return false;
  }
  if (exprSize == 0 || exprSize > NQ_UINT32_MAX || expr[exprSize - 1] != NQ_WASM_OP_End) {
    NQ_LOGE("Function body must end with end opcode");
    return false;
  }
  return true;
}

// Expects validated code
static bool initCode(NQWasmCode* thiz, const NQWasmLocal* locals, uint32_t localGroupCount, const uint8_t* expr, size_t exprSize)
{
  size_t localsSize = (size_t)localGroupCount * sizeof(NQWasmLocal);
  uint8_t* data = (uint8_t*)NQMalloc(localsSize + exprSize);
  if (data == NULL) {
    NQ_LOGE("No memory");
    return false;
  }

  if (localsSize != 0)
    memcpy(data, locals, localsSize);
  memcpy(data + localsSize, expr, exprSize);

  thiz->data = data;
  thiz->locals = (const NQWasmLocal*)data;
  thiz->localGroupCount = localGroupCount;
  thiz->expr = data + localsSize;
  thiz->exprSize = (uint32_t)exprSize;
  return true;
}

NQWasmCodeSection* NQWasmCodeSection_create(void)
{
  NQWasmCodeSection* thiz = (NQWasmCodeSection*)NQMalloc(sizeof(*thiz));
  if (thiz != NULL)
    NQWasmCodeSection_init(thiz);
  return thiz;
}

static bool reserveCodeSection(NQWasmCodeSection* thiz, uint32_t capacity)
{
  if (capacity <= thiz->capacity)
    return true;

  NQWasmCode* codes = (NQWasmCode*)NQRealloc(thiz->codes, (size_t)capacity * sizeof(NQWasmCode));
  if (codes == NULL) {
    NQ_LOGE("No memory");
    return false;
  }

  thiz->codes = codes;
  thiz->capacity = capacity;
  return true;
}

// Reads one entry: size, locals and body instructions
static bool readCode(NQDataReader* reader, NQWasmCode* code)
{
  uint32_t size;
  if (!readLeb128Uint32(reader, &size)) {
    NQ_LOGE("Code size format is wrong");
    return false;
  }

  NQDataReader codeReader;
  NQDataReader_init(&codeReader, NQDataReader_currentData(reader), size);
  if (!NQDataReader_skipAll(reader, size)) {
    NQ_LOGE("Code size is out of section");
    return false;
  }

  uint32_t localGroupCount;
  if (!readLeb128Uint32(&codeReader, &localGroupCount)) {
    NQ_LOGE("Locals vector count format is wrong");
    return false;
  }
  // Every local group takes at least two bytes
  if (localGroupCount > NQDataReader_availableSize(&codeReader) / 2) {
    NQ_LOGE("Locals vector count is too big");
    return false;
  }

  NQWasmLocal* locals = NULL;
  if (localGroupCount != 0) {
    locals = (NQWasmLocal*)NQMalloc((size_t)localGroupCount * sizeof(NQWasmLocal));
    if (locals == NULL) {
      NQ_LOGE("No memory");
      return false;
    }
  }

  for (uint32_t i = 0; i < localGroupCount; i++) {
    if (!readLeb128Uint32(&codeReader, &locals[i].count) || !NQDataReader_readUint8(&codeReader, &locals[i].valtype)) {
      NQ_LOGE("Local format is wrong");
      NQFree(locals);
      return false;
    }
  }

  const uint8_t* expr = (const uint8_t*)NQDataReader_currentData(&codeReader);
  size_t exprSize = NQDataReader_availableSize(&codeReader);

  bool res = isValidCode(locals, localGroupCount, expr, exprSize) && initCode(code, locals, localGroupCount, expr, exprSize);
  NQFree(locals);
  return res;
}

static inline void clearCodes(NQWasmCodeSection* thiz)
{
  for (uint32_t i = 0; i < thiz->itemCount; i++)
    NQFree(thiz->codes[i].data);
  NQFree(thiz->codes);
  thiz->codes = NULL;
  thiz->itemCount = 0;
  thiz->capacity = 0;
}

NQWasmCodeSection* NQWasmCodeSection_fromMemory(const void* data, size_t size)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  uint32_t itemCount;
  if (!readLeb128Uint32(&reader, &itemCount)) {
    NQ_LOGE("Code vector count format is wrong");
    return NULL;
  }
  // Every code takes at least three bytes: size, locals count and end
  if (itemCount > NQDataReader_availableSize(&reader) / 3) {
    NQ_LOGE("Code vector count is too big");
    return NULL;
  }

  NQWasmCodeSection* thiz = NQWasmCodeSection_create();
  if (thiz == NULL)
    return NULL;

  if (!reserveCodeSection(thiz, itemCount)) {
    NQWasmCodeSection_destroy(thiz);
    return NULL;
  }

  while (thiz->itemCount < itemCount) {
    if (!readCode(&reader, &thiz->codes[thiz->itemCount])) {
      NQ_LOGE("Function body %u is wrong", thiz->itemCount);
      NQWasmCodeSection_destroy(thiz);
      return NULL;
    }
    thiz->itemCount++;
  }

  if (!NQDataReader_isEmpty(&reader)) {
    NQ_LOGE("Not the entire code section was processed");
    NQWasmCodeSection_destroy(thiz);
    return NULL;
  }

  return thiz;
}

void NQWasmCodeSection_init(NQWasmCodeSection* thiz)
{
  thiz->base.sectionId = NQ_WASM_SECTION_CODE_ID;
  NQListHead_init(&thiz->base.list);
  thiz->codes = NULL;
  thiz->itemCount = 0;
  thiz->capacity = 0;
}

void NQWasmCodeSection_finalize(NQWasmCodeSection* thiz)
{
  clearCodes(thiz);
  NQListHead_remove(&thiz->base.list);
}

void NQWasmCodeSection_destroy(NQWasmCodeSection* thiz)
{
  NQWasmCodeSection_finalize(thiz);
  NQFree(thiz);
}

static inline uint32_t leb128Uint32Size(uint32_t value)
{
  uint32_t size = 1;
  while (value >= 0x80) {
    value >>= 7;
    size++;
  }
  return size;
}

static bool writeCodeSectionContent(const NQWasmCodeSection* thiz, NQWriteWrapper* writer)
{
  if (!writeLeb128Uint32(writer, thiz->itemCount))
    return false;

  for (uint32_t i = 0; i < thiz->itemCount; i++) {
    const NQWasmCode* code = &thiz->codes[i];

    uint64_t size = leb128Uint32Size(code->localGroupCount) + (uint64_t)code->exprSize;
    for (uint32_t j = 0; j < code->localGroupCount; j++)
      size += leb128Uint32Size(code->locals[j].count) + 1;
    if (size > NQ_UINT32_MAX)
      return false;

    if (!writeLeb128Uint32(writer, (uint32_t)size))
      return false;
    if (!writeLeb128Uint32(writer, code->localGroupCount))
      return false;
    for (uint32_t j = 0; j < code->localGroupCount; j++) {
      if (!writeLeb128Uint32(writer, code->locals[j].count))
        return false;
      if (!NQWriteWrapper_writeUint8(writer, code->locals[j].valtype))
        return false;
    }
    if (!NQWriteWrapper_writeAll(writer, code->expr, code->exprSize))
      return false;
  }

  return true;
}

static bool wasmCodeSectionWriteTo(const NQWasmCodeSection* thiz, NQByteBuffer* tmpBuf, NQWriteWrapper* writer)
{
  NQByteBuffer_resize(tmpBuf, 0);

  NQWriteWrapper tmpWriter;
  NQWriteWrapper_init(&tmpWriter, writeByteBuffer, tmpBuf);

  if (!writeCodeSectionContent(thiz, &tmpWriter))
    return false;

  return writeWasmSection(writer, thiz->base.sectionId, NQByteBuffer_data(tmpBuf), NQByteBuffer_size(tmpBuf));
}

bool NQWasmCodeSection_writeTo(const NQWasmCodeSection* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQByteBuffer buffer;
  NQByteBuffer_init(&buffer);

  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);

  bool res = wasmCodeSectionWriteTo(thiz, &buffer, &writer);
  NQByteBuffer_finalize(&buffer);
  return res;
}

bool NQWasmCodeSection_addCode(NQWasmCodeSection* thiz, const NQWasmLocal* locals, uint32_t localGroupCount, const void* expr, size_t exprSize)
{
  if (!isValidCode(locals, localGroupCount, (const uint8_t*)expr, exprSize))
    return false;

  if (thiz->itemCount == thiz->capacity) {
    if (thiz->capacity == NQ_UINT32_MAX)
      return false;
    uint32_t capacity = thiz->capacity < 8 ? 8 : (thiz->capacity > NQ_UINT32_MAX / 2 ? NQ_UINT32_MAX : thiz->capacity * 2);
    if (!reserveCodeSection(thiz, capacity))
      return false;
  }

  if (!initCode(&thiz->codes[thiz->itemCount], locals, localGroupCount, (const uint8_t*)expr, exprSize))
    return false;

  thiz->itemCount++;
  return true;
}

static bool isValidData(const NQWasmData* thiz)
{
  if (thiz->flags > NQ_WASM_DATA_FLAGS_MAX) {
    NQ_LOGE("Unknown %u flags of data", thiz->flags);
    return false;
  }
  if (thiz->size != 0 && thiz->bytes == NULL) {
    NQ_LOGE("Data bytes are missing");
    return false;
  }

  if (NQWasmData_isActive(thiz)) {
    if (!(thiz->flags & NQ_WASM_DATA_FLAG_EXPLICIT_MEMORY) && thiz->memidx != 0) {
      NQ_LOGE("Data with implicit memory must use memory 0");
      return false;
    }
    if (!isValidConstExprList(thiz->offset, thiz->offsetSize, 1)) {
      NQ_LOGE("Data offset expression is wrong");
      return false;
    }
  }
  else if (thiz->memidx != 0 || thiz->offsetSize != 0) {
    NQ_LOGE("Only active data can have memory and offset");
    return false;
  }

  return true;
}

// Expects validated data
static struct NQWasmData* createData(const NQWasmData* src)
{
  struct NQWasmData* thiz = (struct NQWasmData*)NQMalloc(sizeof(*thiz) + (size_t)src->offsetSize + src->size);
  if (thiz == NULL) {
    NQ_LOGE("No memory");
    return NULL;
  }

  NQListHead_init(&thiz->list);
  thiz->flags = src->flags;
  thiz->memidx = src->memidx;

  uint8_t* ptr = (uint8_t*)thiz + sizeof(*thiz);

  thiz->offsetSize = src->offsetSize;
  thiz->offset = src->offsetSize != 0 ? ptr : NULL;
  if (src->offsetSize != 0)
    memcpy(ptr, src->offset, src->offsetSize);
  ptr += src->offsetSize;

  thiz->size = src->size;
  thiz->bytes = ptr;
  if (src->size != 0)
    memcpy(ptr, src->bytes, src->size);

  return thiz;
}

static struct NQWasmData* readData(NQDataReader* reader)
{
  NQWasmData data;
  memset(&data, 0, sizeof(data));

  uint32_t flags;
  if (!readLeb128Uint32(reader, &flags) || flags > NQ_WASM_DATA_FLAGS_MAX) {
    NQ_LOGE("Data flags are wrong");
    return NULL;
  }
  data.flags = (uint8_t)flags;

  if (NQWasmData_isActive(&data)) {
    if ((flags & NQ_WASM_DATA_FLAG_EXPLICIT_MEMORY) && !readLeb128Uint32(reader, &data.memidx)) {
      NQ_LOGE("Data memidx format is wrong");
      return NULL;
    }
    data.offset = (const uint8_t*)NQDataReader_currentData(reader);
    if (!skipConstExpr(reader)) {
      NQ_LOGE("Data offset expression is wrong");
      return NULL;
    }
    data.offsetSize = (uint32_t)((const uint8_t*)NQDataReader_currentData(reader) - data.offset);
  }

  if (!readLeb128Uint32(reader, &data.size)) {
    NQ_LOGE("Data size format is wrong");
    return NULL;
  }
  data.bytes = (const uint8_t*)NQDataReader_currentData(reader);
  if (!NQDataReader_skipAll(reader, data.size)) {
    NQ_LOGE("Data size is out of section");
    return NULL;
  }

  return isValidData(&data) ? createData(&data) : NULL;
}

NQWasmDataSection* NQWasmDataSection_create(void)
{
  NQWasmDataSection* thiz = (NQWasmDataSection*)NQMalloc(sizeof(*thiz));
  if (thiz != NULL)
    NQWasmDataSection_init(thiz);
  return thiz;
}

static inline void clearDataList(NQListHead* itemList)
{
  NQListHead* iter = itemList->next;
  while (iter != itemList) {
    NQWasmData* item = NQ_CONTAINER_OF(iter, NQWasmData, list);
    iter = iter->next;
    NQListHead_remove(&item->list);
    NQFree(item);
  }
}

NQWasmDataSection* NQWasmDataSection_fromMemory(const void* data, size_t size)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  uint32_t itemCount;
  if (!readLeb128Uint32(&reader, &itemCount)) {
    NQ_LOGE("Data vector count format is wrong");
    return NULL;
  }

  NQListHead itemList;
  NQListHead_init(&itemList);

  size_t itemIndex = 0;
  while (itemIndex < itemCount) {
    struct NQWasmData* item = readData(&reader);
    if (item == NULL)
      break;

    NQListHead_addBack(&itemList, &item->list);
    itemIndex++;
  }

  NQWasmDataSection* thiz = NULL;
  if (itemIndex == itemCount) {
    if (!NQDataReader_isEmpty(&reader))
      NQ_LOGE("Not the entire data section was processed");
    else
      thiz = NQWasmDataSection_create();
    if (thiz != NULL) {
      NQListHead_swap(&thiz->itemList, &itemList);
      thiz->itemCount = itemCount;
    }
  }

  clearDataList(&itemList);
  return thiz;
}

void NQWasmDataSection_init(NQWasmDataSection* thiz)
{
  thiz->base.sectionId = NQ_WASM_SECTION_DATA_ID;
  NQListHead_init(&thiz->base.list);
  NQListHead_init(&thiz->itemList);
  thiz->itemCount = 0;
}

void NQWasmDataSection_finalize(NQWasmDataSection* thiz)
{
  clearDataList(&thiz->itemList);
  NQListHead_remove(&thiz->base.list);
}

void NQWasmDataSection_destroy(NQWasmDataSection* thiz)
{
  NQWasmDataSection_finalize(thiz);
  NQFree(thiz);
}

static bool writeDataSectionContent(const NQWasmDataSection* thiz, NQWriteWrapper* writer)
{
  if (!writeLeb128Uint32(writer, thiz->itemCount))
    return false;

  struct NQWasmData* item = NQWasmDataSection_firstItem(thiz);
  while (item != NULL) {
    if (!writeLeb128Uint32(writer, item->flags))
      return false;

    if (NQWasmData_isActive(item)) {
      if ((item->flags & NQ_WASM_DATA_FLAG_EXPLICIT_MEMORY) && !writeLeb128Uint32(writer, item->memidx))
        return false;
      if (!NQWriteWrapper_writeAll(writer, item->offset, item->offsetSize))
        return false;
    }

    if (!writeLeb128Uint32(writer, item->size))
      return false;
    if (!NQWriteWrapper_writeAll(writer, item->bytes, item->size))
      return false;

    item = NQWasmDataSection_nextItem(thiz, item);
  }

  return true;
}

static bool wasmDataSectionWriteTo(const NQWasmDataSection* thiz, NQByteBuffer* tmpBuf, NQWriteWrapper* writer)
{
  NQByteBuffer_resize(tmpBuf, 0);

  NQWriteWrapper tmpWriter;
  NQWriteWrapper_init(&tmpWriter, writeByteBuffer, tmpBuf);

  if (!writeDataSectionContent(thiz, &tmpWriter))
    return false;

  return writeWasmSection(writer, thiz->base.sectionId, NQByteBuffer_data(tmpBuf), NQByteBuffer_size(tmpBuf));
}

bool NQWasmDataSection_writeTo(const NQWasmDataSection* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQByteBuffer buffer;
  NQByteBuffer_init(&buffer);

  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);

  bool res = wasmDataSectionWriteTo(thiz, &buffer, &writer);
  NQByteBuffer_finalize(&buffer);
  return res;
}

bool NQWasmDataSection_addData(NQWasmDataSection* thiz, const NQWasmData* data)
{
  if (!isValidData(data))
    return false;

  NQWasmData* item = createData(data);
  if (item == NULL)
    return false;

  NQListHead_addBack(&thiz->itemList, &item->list);
  thiz->itemCount++;
  return true;
}

NQWasmData* NQWasmDataSection_getData(const NQWasmDataSection* thiz, uint32_t index)
{
  NQWasmData* item = NQWasmDataSection_firstItem(thiz);
  while (item != NULL && index != 0) {
    item = NQWasmDataSection_nextItem(thiz, item);
    index--;
  }
  return item;
}

NQWasmDataCountSection* NQWasmDataCountSection_create(uint32_t count)
{
  NQWasmDataCountSection* thiz = (NQWasmDataCountSection*)NQMalloc(sizeof(*thiz));
  if (thiz != NULL)
    NQWasmDataCountSection_init(thiz, count);
  return thiz;
}

NQWasmDataCountSection* NQWasmDataCountSection_fromMemory(const void* data, size_t size)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  uint32_t count;
  if (!readLeb128Uint32(&reader, &count)) {
    NQ_LOGE("Data count format is wrong");
    return NULL;
  }
  if (!NQDataReader_isEmpty(&reader)) {
    NQ_LOGE("Not the entire data count section was processed");
    return NULL;
  }

  return NQWasmDataCountSection_create(count);
}

void NQWasmDataCountSection_init(NQWasmDataCountSection* thiz, uint32_t count)
{
  thiz->base.sectionId = NQ_WASM_SECTION_DATA_COUNT_ID;
  NQListHead_init(&thiz->base.list);
  thiz->count = count;
}

void NQWasmDataCountSection_finalize(NQWasmDataCountSection* thiz)
{
  NQListHead_remove(&thiz->base.list);
}

void NQWasmDataCountSection_destroy(NQWasmDataCountSection* thiz)
{
  NQWasmDataCountSection_finalize(thiz);
  NQFree(thiz);
}

static bool wasmDataCountSectionWriteTo(const NQWasmDataCountSection* thiz, NQWriteWrapper* writer)
{
  NQLeb128Buffer buffer;
  size_t size = NQLeb128EncodeUint32(buffer, sizeof(buffer), thiz->count);
  return writeWasmSection(writer, thiz->base.sectionId, buffer, (uint32_t)size);
}

bool NQWasmDataCountSection_writeTo(const NQWasmDataCountSection* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);
  return wasmDataCountSectionWriteTo(thiz, &writer);
}

static struct NQWasmImportItem* createImportItem(uint8_t importId, const NQStringRange* moduleName, const NQStringRange* itemName)
{
  struct NQWasmImportItem* thiz = (struct NQWasmImportItem*)NQMalloc(sizeof(*thiz) + moduleName->length + itemName->length + 2);
  if (thiz == NULL) {
    NQ_LOGE("No memory");
    return NULL;
  }

  thiz->importId = importId;
  NQListHead_init(&thiz->list);

  char* ptr = (char*)thiz + sizeof(*thiz);
  thiz->module = ptr;
  memcpy(ptr, moduleName->characters, moduleName->length);
  ptr += moduleName->length;
  *ptr++ = '\0';

  thiz->name = ptr;
  memcpy(ptr, itemName->characters, itemName->length);
  ptr[itemName->length] = '\0';

  return thiz;
}

static struct NQWasmImportItem* createImportFunction(const NQStringRange* moduleName, const NQStringRange* itemName, uint32_t typeidx)
{
  struct NQWasmImportItem* thiz = createImportItem(NQ_WASM_IMPORT_FUNC_ID, moduleName, itemName);
  if (thiz == NULL)
    return NULL;

  thiz->function.typeidx = typeidx;

  return thiz;
}

static struct NQWasmImportItem* readImportFunction(const NQStringRange* moduleName, const NQStringRange* itemName, NQDataReader* reader)
{
  uint32_t typeidx;
  if (!readLeb128Uint32(reader, &typeidx)) {
    NQ_LOGE("Unexpected end of typeidx reached");
    return NULL;
  }
  return createImportFunction(moduleName, itemName, typeidx);
}

static struct NQWasmImportItem* createImportTable(const NQStringRange* moduleName, const NQStringRange* itemName, uint8_t elemtype, uint32_t minValue, uint32_t maxValue)
{
  struct NQWasmImportItem* thiz = createImportItem(NQ_WASM_IMPORT_TABLE_ID, moduleName, itemName);
  if (thiz == NULL)
    return NULL;

  thiz->table.elemtype = elemtype;
  thiz->table.minValue = minValue;
  thiz->table.maxValue = maxValue;

  return thiz;
}

static struct NQWasmImportItem* readImportTable(const NQStringRange* moduleName, const NQStringRange* itemName, NQDataReader* reader)
{
  uint8_t elemtype;
  uint32_t minValue;
  uint32_t maxValue;

  if (!NQDataReader_readUint8(reader, &elemtype)) {
    NQ_LOGE("Unexpected end of elemtype reached");
    return NULL;
  }
  if (!NQWasmIsElemType(elemtype)) {
    NQ_LOGE("Unknown %02x elemtype of import global", elemtype);
    return NULL;
  }
  if (!readLeb128Uint32(reader, &minValue)) {
    NQ_LOGE("Minimum value of import table is wrong");
    return NULL;
  }
  if (!readLeb128Uint32(reader, &maxValue)) {
    NQ_LOGE("Maximum value of import table is wrong");
    return NULL;
  }

  return createImportTable(moduleName, itemName, elemtype, minValue, maxValue);
}

static struct NQWasmImportItem* createImportMemory(const NQStringRange* moduleName, const NQStringRange* itemName, uint8_t memtype, uint64_t minValue, uint64_t maxValue)
{
  struct NQWasmImportItem* thiz = createImportItem(NQ_WASM_IMPORT_MEM_ID, moduleName, itemName);
  if (thiz == NULL)
    return NULL;

  thiz->memory.memtype = memtype;
  thiz->memory.minValue = minValue;
  thiz->memory.maxValue = maxValue;

  return thiz;
}

static struct NQWasmImportItem* readImportMemory(const NQStringRange* moduleName, const NQStringRange* itemName, NQDataReader* reader)
{
  uint8_t memtype;
  uint64_t minValue;
  uint64_t maxValue;

  if (!NQDataReader_readUint8(reader, &memtype)) {
    NQ_LOGE("Unexpected end of memtype reached");
    return NULL;
  }
  if (!NQWasmIsMemType(memtype)) {
    NQ_LOGE("Unknown %02x memtype of import memory", memtype);
    return NULL;
  }
  if (!readLeb128Uint64(reader, &minValue)) {
    NQ_LOGE("Minimum value of import memory is wrong");
    return NULL;
  }
  if (memtype & NQ_WASM_MEMTYPE_MAXVAL) {
    if (!readLeb128Uint64(reader, &maxValue)) {
      NQ_LOGE("Maximum value of import memory is wrong");
      return NULL;
    }
  }
  else {
    maxValue = 0;
  }

  return createImportMemory(moduleName, itemName, memtype, minValue, maxValue);
}

static struct NQWasmImportItem* createImportGlobal(const NQStringRange* moduleName, const NQStringRange* itemName, uint8_t valtype, uint8_t mut)
{
  struct NQWasmImportItem* thiz = createImportItem(NQ_WASM_IMPORT_GLOBAL_ID, moduleName, itemName);
  if (thiz == NULL)
    return NULL;

  thiz->global.valtype = valtype;
  thiz->global.mut = mut;

  return thiz;
}

static struct NQWasmImportItem* readImportGlobal(const NQStringRange* moduleName, const NQStringRange* itemName, NQDataReader* reader)
{
  uint8_t valtype;
  uint8_t mut;

  if (!NQDataReader_readUint8(reader, &valtype)) {
    NQ_LOGE("Unexpected end of numtype reached");
    return NULL;
  }
  if (!NQWasmIsValType(valtype)) {
    NQ_LOGE("Unknown %02x numtype of import global", valtype);
    return NULL;
  }
  if (!NQDataReader_readUint8(reader, &mut)) {
    NQ_LOGE("Unexpected end of numtype reached");
    return NULL;
  }

  return createImportGlobal(moduleName, itemName, valtype, mut);
}

NQWasmImportSection* NQWasmImportSection_create(void)
{
  NQWasmImportSection* thiz = (NQWasmImportSection*)NQMalloc(sizeof(*thiz));
  if (thiz != NULL)
    NQWasmImportSection_init(thiz);
  return thiz;
}

static inline void clearImportItemList(NQListHead* itemList)
{
  NQListHead* iter = itemList->next;
  while (iter != itemList) {
    NQWasmImportItem* item = NQ_CONTAINER_OF(iter, NQWasmImportItem, list);
    iter = iter->next;
    NQListHead_remove(&item->list);
    NQFree(item);
  }
}

NQWasmImportSection* NQWasmImportSection_fromMemory(const void* data, size_t size)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  uint32_t itemCount;
  if (!readLeb128Uint32(&reader, &itemCount)) {
    NQ_LOGE("Import vector count format is wrong");
    return false;
  }

  NQListHead itemList;
  NQListHead_init(&itemList);

  size_t itemIndex = 0;
  while (itemIndex < itemCount) {
    struct NQWasmImportItem* item = NULL;

    NQStringRange moduleName;
    if (!readString(&reader, &moduleName)) {
      NQ_LOGE("Import module name is wrong");
      break;
    }

    NQStringRange itemName;
    if (!readString(&reader, &itemName)) {
      NQ_LOGE("Import item name is wrong");
      break;
    }

    uint8_t importId;
    if (!NQDataReader_readUint8(&reader, &importId)) {
      NQ_LOGE("Import desc format is wrong");
      break;
    }

    switch (importId) {
    case NQ_WASM_IMPORT_FUNC_ID:
      item = readImportFunction(&moduleName, &itemName, &reader);
      break;

    case NQ_WASM_IMPORT_TABLE_ID:
      item = readImportTable(&moduleName, &itemName, &reader);
      break;

    case NQ_WASM_IMPORT_MEM_ID:
      item = readImportMemory(&moduleName, &itemName, &reader);
      break;

    case NQ_WASM_IMPORT_GLOBAL_ID:
      item = readImportGlobal(&moduleName, &itemName, &reader);
      break;

    default:
      NQ_LOGE("Unknown import id - %u", importId);
      break;
    }

    if (item == NULL)
      break;

    NQListHead_addBack(&itemList, &item->list);
    itemIndex++;
  }

  NQWasmImportSection* thiz = NULL;
  if (itemIndex == itemCount) {
    if (!NQDataReader_isEmpty(&reader))
      NQ_LOGE("Not the entire import section was processed");
    else
      thiz = NQWasmImportSection_create();
    if (thiz != NULL) {
      NQListHead_swap(&thiz->itemList, &itemList);
      thiz->itemCount = itemCount;
    }
  }

  clearImportItemList(&itemList);
  return thiz;
}

void NQWasmImportSection_init(NQWasmImportSection* thiz)
{
  thiz->base.sectionId = NQ_WASM_SECTION_IMPORT_ID;
  NQListHead_init(&thiz->base.list);
  NQListHead_init(&thiz->itemList);
  thiz->itemCount = 0;
}

void NQWasmImportSection_finalize(NQWasmImportSection* thiz)
{
  clearImportItemList(&thiz->itemList);
  NQListHead_remove(&thiz->base.list);
}

void NQWasmImportSection_destroy(NQWasmImportSection* thiz)
{
  NQWasmImportSection_finalize(thiz);
  NQFree(thiz);
}

static bool writeImportSectionContent(const NQWasmImportSection* thiz, NQWriteWrapper* writer)
{
  if (!writeLeb128Uint32(writer, thiz->itemCount))
    return false;

  struct NQWasmImportItem* item = NQWasmImportSection_firstItem(thiz);
  while (item != NULL) {
    if (!writeString(writer, item->module))
      return false;

    if (!writeString(writer, item->name))
      return false;

    if (!NQWriteWrapper_writeUint8(writer, item->importId))
      return false;

    switch (item->importId) {
    case NQ_WASM_IMPORT_FUNC_ID:
      if (!writeLeb128Uint32(writer, item->function.typeidx))
        return false;
      break;

    case NQ_WASM_IMPORT_TABLE_ID:
      if (!NQWriteWrapper_writeUint8(writer, item->table.elemtype))
        return false;
      if (!writeLeb128Uint32(writer, item->table.minValue))
        return false;
      if (!writeLeb128Uint32(writer, item->table.maxValue))
        return false;
      break;

    case NQ_WASM_IMPORT_MEM_ID:
      if (!NQWriteWrapper_writeUint8(writer, item->memory.memtype))
        return false;
      if (!writeLeb128Uint64(writer, item->memory.minValue))
        return false;
      if (item->memory.memtype & NQ_WASM_MEMTYPE_MAXVAL) {
        if (!writeLeb128Uint64(writer, item->memory.maxValue))
          return false;
      }
      break;

    case NQ_WASM_IMPORT_GLOBAL_ID:
      if (!NQWriteWrapper_writeUint8(writer, item->global.valtype))
        return false;
      if (!NQWriteWrapper_writeUint8(writer, item->global.mut))
        return false;
      break;

    default:
      NQ_ASSERT_NOT_REACHED();
      return false;
    }

    item = NQWasmImportSection_nextItem(thiz, item);
  }

  return true;
}

static bool wasmImportSectionWriteTo(const NQWasmImportSection* thiz, NQByteBuffer* tmpBuf, NQWriteWrapper* writer)
{
  NQByteBuffer_resize(tmpBuf, 0);

  NQWriteWrapper tmpWriter;
  NQWriteWrapper_init(&tmpWriter, writeByteBuffer, tmpBuf);

  if (!writeImportSectionContent(thiz, &tmpWriter))
    return false;

  return writeWasmSection(writer, thiz->base.sectionId, NQByteBuffer_data(tmpBuf), NQByteBuffer_size(tmpBuf));
}

bool NQWasmImportSection_writeTo(const NQWasmImportSection* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQByteBuffer buffer;
  NQByteBuffer_init(&buffer);

  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);

  bool res = wasmImportSectionWriteTo(thiz, &buffer, &writer);
  NQByteBuffer_finalize(&buffer);
  return res;
}

bool NQWasmImportSection_addMemory(NQWasmImportSection* thiz, const char* module, const char* name, uint8_t memtype, uint64_t minValue, uint64_t maxValue)
{
  NQStringRange moduleName;
  moduleName.characters = module;
  moduleName.length = NQStrlen(module);

  NQStringRange itemName;
  itemName.characters = name;
  itemName.length = NQStrlen(name);

  NQWasmImportItem* item = createImportMemory(&moduleName, &itemName, memtype, minValue, maxValue);
  if (item == NULL)
    return false;

  NQListHead_addBack(&thiz->itemList, &item->list);
  thiz->itemCount++;
  return thiz;
}

static inline bool isWasmExportId(uint8_t exportId)
{
  return exportId <= NQ_WASM_EXPORT_GLOBAL_ID;
}

static struct NQWasmExportItem* createExportItem(uint8_t exportId, const NQStringRange* itemName, uint32_t index)
{
  struct NQWasmExportItem* thiz = (struct NQWasmExportItem*)NQMalloc(sizeof(*thiz) + itemName->length + 1);
  if (thiz == NULL) {
    NQ_LOGE("No memory");
    return NULL;
  }

  thiz->exportId = exportId;
  thiz->index = index;
  NQListHead_init(&thiz->list);

  char* ptr = (char*)thiz + sizeof(*thiz);
  thiz->name = ptr;
  memcpy(ptr, itemName->characters, itemName->length);
  ptr[itemName->length] = '\0';

  return thiz;
}

NQWasmExportSection* NQWasmExportSection_create(void)
{
  NQWasmExportSection* thiz = (NQWasmExportSection*)NQMalloc(sizeof(*thiz));
  if (thiz != NULL)
    NQWasmExportSection_init(thiz);
  return thiz;
}

static inline void clearExportItemList(NQListHead* itemList)
{
  NQListHead* iter = itemList->next;
  while (iter != itemList) {
    NQWasmExportItem* item = NQ_CONTAINER_OF(iter, NQWasmExportItem, list);
    iter = iter->next;
    NQListHead_remove(&item->list);
    NQFree(item);
  }
}

NQWasmExportSection* NQWasmExportSection_fromMemory(const void* data, size_t size)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  uint32_t itemCount;
  if (!readLeb128Uint32(&reader, &itemCount)) {
    NQ_LOGE("Export vector count format is wrong");
    return NULL;
  }

  NQListHead itemList;
  NQListHead_init(&itemList);

  size_t itemIndex = 0;
  while (itemIndex < itemCount) {
    NQStringRange itemName;
    if (!readString(&reader, &itemName)) {
      NQ_LOGE("Export item name is wrong");
      break;
    }

    uint8_t exportId;
    if (!NQDataReader_readUint8(&reader, &exportId)) {
      NQ_LOGE("Export desc format is wrong");
      break;
    }
    if (!isWasmExportId(exportId)) {
      NQ_LOGE("Unknown export id - %u", exportId);
      break;
    }

    uint32_t index;
    if (!readLeb128Uint32(&reader, &index)) {
      NQ_LOGE("Export index format is wrong");
      break;
    }

    struct NQWasmExportItem* item = createExportItem(exportId, &itemName, index);
    if (item == NULL)
      break;

    NQListHead_addBack(&itemList, &item->list);
    itemIndex++;
  }

  NQWasmExportSection* thiz = NULL;
  if (itemIndex == itemCount) {
    if (!NQDataReader_isEmpty(&reader))
      NQ_LOGE("Not the entire export section was processed");
    else
      thiz = NQWasmExportSection_create();
    if (thiz != NULL) {
      NQListHead_swap(&thiz->itemList, &itemList);
      thiz->itemCount = itemCount;
    }
  }

  clearExportItemList(&itemList);
  return thiz;
}

void NQWasmExportSection_init(NQWasmExportSection* thiz)
{
  thiz->base.sectionId = NQ_WASM_SECTION_EXPORT_ID;
  NQListHead_init(&thiz->base.list);
  NQListHead_init(&thiz->itemList);
  thiz->itemCount = 0;
}

void NQWasmExportSection_finalize(NQWasmExportSection* thiz)
{
  clearExportItemList(&thiz->itemList);
  NQListHead_remove(&thiz->base.list);
}

void NQWasmExportSection_destroy(NQWasmExportSection* thiz)
{
  NQWasmExportSection_finalize(thiz);
  NQFree(thiz);
}

static bool writeExportSectionContent(const NQWasmExportSection* thiz, NQWriteWrapper* writer)
{
  if (!writeLeb128Uint32(writer, thiz->itemCount))
    return false;

  struct NQWasmExportItem* item = NQWasmExportSection_firstItem(thiz);
  while (item != NULL) {
    if (!writeString(writer, item->name))
      return false;

    if (!NQWriteWrapper_writeUint8(writer, item->exportId))
      return false;

    if (!writeLeb128Uint32(writer, item->index))
      return false;

    item = NQWasmExportSection_nextItem(thiz, item);
  }

  return true;
}

static bool wasmExportSectionWriteTo(const NQWasmExportSection* thiz, NQByteBuffer* tmpBuf, NQWriteWrapper* writer)
{
  NQByteBuffer_resize(tmpBuf, 0);

  NQWriteWrapper tmpWriter;
  NQWriteWrapper_init(&tmpWriter, writeByteBuffer, tmpBuf);

  if (!writeExportSectionContent(thiz, &tmpWriter))
    return false;

  return writeWasmSection(writer, thiz->base.sectionId, NQByteBuffer_data(tmpBuf), NQByteBuffer_size(tmpBuf));
}

bool NQWasmExportSection_writeTo(const NQWasmExportSection* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQByteBuffer buffer;
  NQByteBuffer_init(&buffer);

  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);

  bool res = wasmExportSectionWriteTo(thiz, &buffer, &writer);
  NQByteBuffer_finalize(&buffer);
  return res;
}

bool NQWasmExportSection_addItem(NQWasmExportSection* thiz, const char* name, uint8_t exportId, uint32_t index)
{
  if (!isWasmExportId(exportId)) {
    NQ_LOGE("Unknown export id - %u", exportId);
    return false;
  }

  NQStringRange itemName;
  itemName.characters = name;
  itemName.length = NQStrlen(name);

  NQWasmExportItem* item = createExportItem(exportId, &itemName, index);
  if (item == NULL)
    return false;

  NQListHead_addBack(&thiz->itemList, &item->list);
  thiz->itemCount++;
  return true;
}

NQWasmExportItem* NQWasmExportSection_findItem(const NQWasmExportSection* thiz, const char* name)
{
  NQWasmExportItem* item = NQWasmExportSection_firstItem(thiz);
  while (item != NULL) {
    if (strcmp(item->name, name) == 0)
      return item;
    item = NQWasmExportSection_nextItem(thiz, item);
  }
  return NULL;
}

NQWasmSection* NQWasmSection_fromMemory(uint8_t sectionId, const void* data, size_t size)
{
  switch (sectionId) {
  case NQ_WASM_SECTION_CUSTOM_ID:
    return (NQWasmSection*)NQWasmCustomSection_fromMemory(data, size);
  case NQ_WASM_SECTION_TYPE_ID:
    return (NQWasmSection*)NQWasmTypeSection_fromMemory(data, size);
  case NQ_WASM_SECTION_IMPORT_ID:
    return (NQWasmSection*)NQWasmImportSection_fromMemory(data, size);
  case NQ_WASM_SECTION_EXPORT_ID:
    return (NQWasmSection*)NQWasmExportSection_fromMemory(data, size);
  case NQ_WASM_SECTION_FUNCTION_ID:
    return (NQWasmSection*)NQWasmFunctionSection_fromMemory(data, size);
  case NQ_WASM_SECTION_TABLE_ID:
    return (NQWasmSection*)NQWasmTableSection_fromMemory(data, size);
  case NQ_WASM_SECTION_MEMORY_ID:
    return (NQWasmSection*)NQWasmMemorySection_fromMemory(data, size);
  case NQ_WASM_SECTION_GLOBAL_ID:
    return (NQWasmSection*)NQWasmGlobalSection_fromMemory(data, size);
  case NQ_WASM_SECTION_START_ID:
    return (NQWasmSection*)NQWasmStartSection_fromMemory(data, size);
  case NQ_WASM_SECTION_ELEMENT_ID:
    return (NQWasmSection*)NQWasmElementSection_fromMemory(data, size);
  case NQ_WASM_SECTION_CODE_ID:
    return (NQWasmSection*)NQWasmCodeSection_fromMemory(data, size);
  case NQ_WASM_SECTION_DATA_ID:
    return (NQWasmSection*)NQWasmDataSection_fromMemory(data, size);
  case NQ_WASM_SECTION_DATA_COUNT_ID:
    return (NQWasmSection*)NQWasmDataCountSection_fromMemory(data, size);
  default:
    NQ_LOGW("Unknown section id %i", sectionId);
    break;
  }

  return (NQWasmSection*)wasmUnknownSectionCreate(sectionId, data, size);
}

void NQWasmSection_destroy(NQWasmSection* thiz)
{
  switch (thiz->sectionId) {
  case NQ_WASM_SECTION_CUSTOM_ID:
    NQWasmCustomSection_destroy((NQWasmCustomSection*)thiz);
    return;
  case NQ_WASM_SECTION_TYPE_ID:
    NQWasmTypeSection_destroy((NQWasmTypeSection*)thiz);
    return;
  case NQ_WASM_SECTION_IMPORT_ID:
    NQWasmImportSection_destroy((NQWasmImportSection*)thiz);
    return;
  case NQ_WASM_SECTION_EXPORT_ID:
    NQWasmExportSection_destroy((NQWasmExportSection*)thiz);
    return;
  case NQ_WASM_SECTION_FUNCTION_ID:
    NQWasmFunctionSection_destroy((NQWasmFunctionSection*)thiz);
    return;
  case NQ_WASM_SECTION_TABLE_ID:
    NQWasmTableSection_destroy((NQWasmTableSection*)thiz);
    return;
  case NQ_WASM_SECTION_MEMORY_ID:
    NQWasmMemorySection_destroy((NQWasmMemorySection*)thiz);
    return;
  case NQ_WASM_SECTION_GLOBAL_ID:
    NQWasmGlobalSection_destroy((NQWasmGlobalSection*)thiz);
    return;
  case NQ_WASM_SECTION_START_ID:
    NQWasmStartSection_destroy((NQWasmStartSection*)thiz);
    return;
  case NQ_WASM_SECTION_ELEMENT_ID:
    NQWasmElementSection_destroy((NQWasmElementSection*)thiz);
    return;
  case NQ_WASM_SECTION_CODE_ID:
    NQWasmCodeSection_destroy((NQWasmCodeSection*)thiz);
    return;
  case NQ_WASM_SECTION_DATA_ID:
    NQWasmDataSection_destroy((NQWasmDataSection*)thiz);
    return;
  case NQ_WASM_SECTION_DATA_COUNT_ID:
    NQWasmDataCountSection_destroy((NQWasmDataCountSection*)thiz);
    return;
  }

  wasmUnknownSectionDestroy((NQWasmUnknownSection*)thiz);
}

static bool wasmSectionWriteTo(NQWasmSection* thiz, NQByteBuffer* tmpBuf, NQWriteWrapper* writer)
{
  switch (thiz->sectionId) {
  case NQ_WASM_SECTION_CUSTOM_ID:
    return wasmCustomSectionWriteTo((NQWasmCustomSection*)thiz, tmpBuf, writer);
  case NQ_WASM_SECTION_TYPE_ID:
    return wasmTypeSectionWriteTo((NQWasmTypeSection*)thiz, tmpBuf, writer);
  case NQ_WASM_SECTION_IMPORT_ID:
    return wasmImportSectionWriteTo((NQWasmImportSection*)thiz, tmpBuf, writer);
  case NQ_WASM_SECTION_EXPORT_ID:
    return wasmExportSectionWriteTo((NQWasmExportSection*)thiz, tmpBuf, writer);
  case NQ_WASM_SECTION_FUNCTION_ID:
    return wasmFunctionSectionWriteTo((NQWasmFunctionSection*)thiz, tmpBuf, writer);
  case NQ_WASM_SECTION_TABLE_ID:
    return wasmTableSectionWriteTo((NQWasmTableSection*)thiz, tmpBuf, writer);
  case NQ_WASM_SECTION_MEMORY_ID:
    return wasmMemorySectionWriteTo((NQWasmMemorySection*)thiz, tmpBuf, writer);
  case NQ_WASM_SECTION_GLOBAL_ID:
    return wasmGlobalSectionWriteTo((NQWasmGlobalSection*)thiz, tmpBuf, writer);
  case NQ_WASM_SECTION_START_ID:
    return wasmStartSectionWriteTo((NQWasmStartSection*)thiz, writer);
  case NQ_WASM_SECTION_ELEMENT_ID:
    return wasmElementSectionWriteTo((NQWasmElementSection*)thiz, tmpBuf, writer);
  case NQ_WASM_SECTION_CODE_ID:
    return wasmCodeSectionWriteTo((NQWasmCodeSection*)thiz, tmpBuf, writer);
  case NQ_WASM_SECTION_DATA_ID:
    return wasmDataSectionWriteTo((NQWasmDataSection*)thiz, tmpBuf, writer);
  case NQ_WASM_SECTION_DATA_COUNT_ID:
    return wasmDataCountSectionWriteTo((NQWasmDataCountSection*)thiz, writer);
  }

  return wasmUnknownSectionWriteTo((NQWasmUnknownSection*)thiz, tmpBuf, writer);
}

bool NQWasmSection_writeTo(NQWasmSection* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQByteBuffer buffer;
  NQByteBuffer_init(&buffer);

  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);

  bool res = wasmSectionWriteTo(thiz, &buffer, &writer);
  NQByteBuffer_finalize(&buffer);
  return res;
}

static inline void clearSectionList(NQListHead* sectionList)
{
  NQListHead* iter = sectionList->next;
  while (iter != sectionList) {
    NQWasmSection* section = NQ_CONTAINER_OF(iter, NQWasmSection, list);
    iter = iter->next;
    NQWasmSection_destroy(section);
  }
}

void NQWasmModule_finalize(NQWasmModule* thiz)
{
  clearSectionList(&thiz->sectionList);
}

void NQWasmModule_destroy(NQWasmModule* thiz)
{
  clearSectionList(&thiz->sectionList);
  NQFree(thiz);
}

NQWasmSection* NQWasmModule_findSection(const NQWasmModule* thiz, uint8_t sectionId)
{
  NQWasmSection* iter = NQWasmModule_firstSection(thiz);
  while (iter != NULL) {
    if (iter->sectionId == sectionId)
      break;
    iter = NQWasmModule_nextSection(thiz, iter);
  }
  return iter;
}

NQWasmCustomSection* NQWasmModule_findCustomSection(const NQWasmModule* thiz, const char* name)
{
  NQWasmSection* iter = NQWasmModule_firstSection(thiz);
  while (iter != NULL) {
    if (iter->sectionId == NQ_WASM_SECTION_CUSTOM_ID && strcmp(((NQWasmCustomSection*)iter)->name, name) == 0)
      return (NQWasmCustomSection*)iter;
    iter = NQWasmModule_nextSection(thiz, iter);
  }
  return NULL;
}

static bool wasmModuleWriteTo(const NQWasmModule* module, NQWriteWrapper* writer)
{
  if (!NQWriteWrapper_writeUint32LE(writer, module->header.magic)) {
    NQ_LOGE("Can not write magic");
    return false;
  }

  if (!NQWriteWrapper_writeUint32LE(writer, module->header.version)) {
    NQ_LOGE("Can not write version");
    return false;
  }

  NQByteBuffer buffer;
  NQByteBuffer_init(&buffer);;

  NQWasmSection* section = NQWasmModule_firstSection(module);
  while (section != NULL) {
    if (!wasmSectionWriteTo(section, &buffer, writer)) {
      NQ_LOGE("Can not write %u section", section->sectionId);
      break;
    }
    section = NQWasmModule_nextSection(module, section);
  }

  NQByteBuffer_finalize(&buffer);
  return section == NULL;
}

bool NQWasmModule_writeTo(const NQWasmModule* thiz, NQWasmWriteCallback write, void* userdata)
{
  NQWriteWrapper writer;
  NQWriteWrapper_init(&writer, write, userdata);
  return wasmModuleWriteTo(thiz, &writer);
}

void NQWasmModule_init(NQWasmModule* thiz)
{
  thiz->header.magic = NQ_WASM_MAGIC;
  thiz->header.version = NQ_WASM_VERSION;
  NQListHead_init(&thiz->sectionList);
}

NQWasmModule* NQWasmModule_fromMemory(const void* data, size_t size)
{
  NQDataReader reader;
  NQDataReader_init(&reader, data, size);

  uint32_t magic;
  uint32_t version;

  if (!NQDataReader_readUint32LE(&reader, &magic)) {
    NQ_LOGE("Not enough data to get magic");
    return NULL;
  }

  if (magic != NQ_WASM_MAGIC) {
    NQ_LOGE("Magic is not correct 0x%04X", magic);
    return NULL;
  }

  if (!NQDataReader_readUint32LE(&reader, &version)) {
    NQ_LOGE("Not enough data to get version");
    return NULL;
  }

  if (version != NQ_WASM_VERSION) {
    NQ_LOGE("Version is not correct 0x%04X", version);
    return NULL;
  }

  NQListHead sectionList;
  NQListHead_init(&sectionList);

  while (!NQDataReader_isEmpty(&reader)) {
    uint8_t sectionId;
    if (!NQDataReader_readUint8(&reader, &sectionId)) {
      NQ_LOGE("Not enough data to get section id");
      break;
    }

    uint32_t sectionSize;
    if (!readLeb128Uint32(&reader, &sectionSize)) {
      NQ_LOGE("Section size format is wrong");
      break;
    }

    uint8_t* sectionData = NQDataReader_currentData(&reader);
    if (!NQDataReader_skipAll(&reader, sectionSize)) {
      NQ_LOGE("Not enough data to get section content");
      break;
    }

    NQWasmSection* section = NQWasmSection_fromMemory(sectionId, sectionData, sectionSize);
    if (section == NULL) {
      break;
    }

    NQListHead_addBack(&sectionList, &section->list);
  }

  NQWasmModule* thiz;
  if (NQDataReader_isEmpty(&reader)) {
    thiz = (NQWasmModule*)NQMalloc(sizeof(*thiz));
    if (thiz != NULL) {
      thiz->header.magic = magic;
      thiz->header.version = version;
      NQListHead_init(&thiz->sectionList);
      NQListHead_swap(&thiz->sectionList, &sectionList);
    }
  }
  else {
    thiz = NULL;
  }

  clearSectionList(&sectionList);
  return thiz;
}
