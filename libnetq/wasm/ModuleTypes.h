/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_WASM_MODULETYPES_H
#define _LIBNETQ_WASM_MODULETYPES_H

#include <libnetq/Basic.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	NQ_WASM_MAG0 0x00
#define	NQ_WASM_MAG1 'a'
#define	NQ_WASM_MAG2 's'
#define	NQ_WASM_MAG3 'm'
#define	NQ_WASM_MAG_STR "\0asm"
#define NQ_WASM_MAG_LEN 4

#define NQ_WASM_MAGIC 0x6d736100
#define NQ_WASM_VERSION 1

enum {
  NQ_WASM_SECTION_CUSTOM_ID = 0,
  NQ_WASM_SECTION_TYPE_ID = 1,
  NQ_WASM_SECTION_IMPORT_ID = 2,
  NQ_WASM_SECTION_FUNCTION_ID = 3,
  NQ_WASM_SECTION_TABLE_ID = 4,
  NQ_WASM_SECTION_MEMORY_ID = 5,
  NQ_WASM_SECTION_GLOBAL_ID = 6,
  NQ_WASM_SECTION_EXPORT_ID = 7,
  NQ_WASM_SECTION_START_ID = 8,
  NQ_WASM_SECTION_ELEMENT_ID = 9,
  NQ_WASM_SECTION_CODE_ID = 10,
  NQ_WASM_SECTION_DATA_ID = 11,
  NQ_WASM_SECTION_DATA_COUNT_ID = 12,
};

NQ_EXPORT const char* NQGetWasmSectionNameById(int sectionId);

enum {
  NQ_WASM_IMPORT_FUNC_ID = 0,
  NQ_WASM_IMPORT_TABLE_ID = 1,
  NQ_WASM_IMPORT_MEM_ID = 2,
  NQ_WASM_IMPORT_GLOBAL_ID = 3,
};

enum {
  NQ_WASM_EXPORT_FUNC_ID = 0,
  NQ_WASM_EXPORT_TABLE_ID = 1,
  NQ_WASM_EXPORT_MEM_ID = 2,
  NQ_WASM_EXPORT_GLOBAL_ID = 3,
};

struct NQWasmHeader {
  uint32_t magic;
  uint32_t version;
};

#define NQ_WASM_MEMTYPE_MAXVAL (1 << 0)
#define NQ_WASM_MEMTYPE_SHARED (1 << 1)
#define NQ_WASM_MEMTYPE_WASM64 (1 << 2)

static inline bool NQWasmIsMemType(uint8_t memtype)
{
  return (memtype & ~(NQ_WASM_MEMTYPE_MAXVAL | NQ_WASM_MEMTYPE_SHARED | NQ_WASM_MEMTYPE_WASM64)) == 0;
}

#define NQ_WASM_MEMORY32_MAX_PAGES (1ULL << 16)
#define NQ_WASM_MEMORY64_MAX_PAGES (1ULL << 48)

#define NQ_WASM_TABLETYPE_MAXVAL (1 << 0)
#define NQ_WASM_TABLETYPE_TABLE64 (1 << 2)

// Element segment flags (0..7)
#define NQ_WASM_ELEM_FLAG_PASSIVE (1 << 0) // Declarative when combined with NQ_WASM_ELEM_FLAG_EXPLICIT_TABLE
#define NQ_WASM_ELEM_FLAG_EXPLICIT_TABLE (1 << 1)
#define NQ_WASM_ELEM_FLAG_EXPRS (1 << 2)
#define NQ_WASM_ELEM_FLAGS_MAX 7

// Data segment flags (0..2)
#define NQ_WASM_DATA_FLAG_PASSIVE (1 << 0)
#define NQ_WASM_DATA_FLAG_EXPLICIT_MEMORY (1 << 1)
#define NQ_WASM_DATA_FLAGS_MAX 2

static inline bool NQWasmIsTableType(uint8_t tabletype)
{
  return (tabletype & ~(NQ_WASM_TABLETYPE_MAXVAL | NQ_WASM_TABLETYPE_TABLE64)) == 0;
}

enum {
  NQ_WASM_TYPE_FUNC = 0x60,
  NQ_WASM_TYPE_REFNULL = 0x63,
  NQ_WASM_TYPE_REF = 0x64,
  NQ_WASM_TYPE_V128 = 0x7b,
  NQ_WASM_TYPE_EXTERNREF = 0x6f,
  NQ_WASM_TYPE_FUNCREF = 0x70,
  NQ_WASM_TYPE_F64 = 0x7c,
  NQ_WASM_TYPE_F32 = 0x7d,
  NQ_WASM_TYPE_I64 = 0x7e,
  NQ_WASM_TYPE_I32 = 0x7f,
};

NQ_EXPORT const char* NQWasmValTypeToString(uint8_t type);
static inline bool NQWasmIsValType(uint8_t valtype)
{
  switch (valtype) {
  case NQ_WASM_TYPE_I32:
  case NQ_WASM_TYPE_I64:
  case NQ_WASM_TYPE_F32:
  case NQ_WASM_TYPE_F64:
  case NQ_WASM_TYPE_V128:
  case NQ_WASM_TYPE_REFNULL:
  case NQ_WASM_TYPE_REF:
    return true;
  }
  return false;
}

static inline bool NQWasmIsElemType(uint8_t elemtype)
{
  switch (elemtype) {
  case NQ_WASM_TYPE_FUNCREF:
    return true;
  }
  return false;
}

struct NQWasmMemoryInfo {
  uint32_t minValue;
  uint32_t maxValue;
  bool hasMaxValue;
  bool isShared;
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBNETQ_WASM_MODULETYPES_H */
