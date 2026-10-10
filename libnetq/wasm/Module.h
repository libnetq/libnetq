/*
 * MIT License
 *
 * Copyright (c) 2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#ifndef _LIBNETQ_WASM_MODULE_H
#define _LIBNETQ_WASM_MODULE_H

#include <libnetq/List.h>
#include <libnetq/wasm/ModuleTypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*NQWasmWriteCallback) (void* userdata, const void* data, size_t size);

typedef struct NQWasmSection NQWasmSection;
struct NQWasmSection {
  NQListHead list;
  uint8_t sectionId;
};

NQ_EXPORT NQWasmSection* NQWasmSection_fromMemory(uint8_t sectionId, const void* data, size_t size);
NQ_EXPORT void NQWasmSection_destroy(NQWasmSection*);
NQ_EXPORT bool NQWasmSection_writeTo(NQWasmSection*, NQWasmWriteCallback write, void* userdata);

typedef struct NQWasmCustomSection NQWasmCustomSection;
struct NQWasmCustomSection {
  NQWasmSection base;

  const char* name;     // UTF-8, zero-terminated
  uint32_t nameLength;
  uint32_t size;
  const uint8_t* data;
};

NQ_EXPORT NQWasmCustomSection* NQWasmCustomSection_create(const char* name, const void* data, size_t size);
NQ_EXPORT NQWasmCustomSection* NQWasmCustomSection_fromMemory(const void* data, size_t size);
NQ_EXPORT void NQWasmCustomSection_destroy(NQWasmCustomSection*);
NQ_EXPORT bool NQWasmCustomSection_writeTo(const NQWasmCustomSection*, NQWasmWriteCallback write, void* userdata);

typedef struct NQWasmFuncType NQWasmFuncType;
struct NQWasmFuncType {
  NQListHead list;

  uint32_t paramCount;
  uint32_t resultCount;
  const uint8_t* params;
  const uint8_t* results;
};

typedef struct NQWasmTypeSection NQWasmTypeSection;
struct NQWasmTypeSection {
  NQWasmSection base;

  NQListHead itemList;
  uint32_t itemCount;
};

NQ_EXPORT NQWasmTypeSection* NQWasmTypeSection_create(void);
NQ_EXPORT NQWasmTypeSection* NQWasmTypeSection_fromMemory(const void* data, size_t size);
NQ_EXPORT void NQWasmTypeSection_init(NQWasmTypeSection*);
NQ_EXPORT void NQWasmTypeSection_finalize(NQWasmTypeSection*);
NQ_EXPORT void NQWasmTypeSection_destroy(NQWasmTypeSection*);
NQ_EXPORT bool NQWasmTypeSection_writeTo(const NQWasmTypeSection*, NQWasmWriteCallback write, void* userdata);
NQ_EXPORT bool NQWasmTypeSection_addFuncType(NQWasmTypeSection*, const uint8_t* params, uint32_t paramCount, const uint8_t* results, uint32_t resultCount);
NQ_EXPORT NQWasmFuncType* NQWasmTypeSection_getFuncType(const NQWasmTypeSection*, uint32_t typeidx);

#define NQWasmTypeSection_firstItem(thiz) ((thiz)->itemList.next != &(thiz)->itemList) \
  ? NQ_CONTAINER_OF((thiz)->itemList.next, NQWasmFuncType, list) : NULL
#define NQWasmTypeSection_nextItem(thiz, item) ((item)->list.next != &(thiz)->itemList) \
  ? NQ_CONTAINER_OF((item)->list.next, NQWasmFuncType, list) : NULL

typedef struct NQWasmFunctionSection NQWasmFunctionSection;
struct NQWasmFunctionSection {
  NQWasmSection base;

  uint32_t* typeIndices;
  uint32_t itemCount;
  uint32_t capacity;
};

NQ_EXPORT NQWasmFunctionSection* NQWasmFunctionSection_create(void);
NQ_EXPORT NQWasmFunctionSection* NQWasmFunctionSection_fromMemory(const void* data, size_t size);
NQ_EXPORT void NQWasmFunctionSection_init(NQWasmFunctionSection*);
NQ_EXPORT void NQWasmFunctionSection_finalize(NQWasmFunctionSection*);
NQ_EXPORT void NQWasmFunctionSection_destroy(NQWasmFunctionSection*);
NQ_EXPORT bool NQWasmFunctionSection_writeTo(const NQWasmFunctionSection*, NQWasmWriteCallback write, void* userdata);
NQ_EXPORT bool NQWasmFunctionSection_addFunction(NQWasmFunctionSection*, uint32_t typeidx);

static inline uint32_t NQWasmFunctionSection_getTypeIndex(const NQWasmFunctionSection* thiz, uint32_t index)
{
  return thiz->typeIndices[index];
}

typedef struct NQWasmTable NQWasmTable;
struct NQWasmTable {
  uint8_t reftype;
  uint8_t tabletype;
  uint64_t minValue;
  uint64_t maxValue;
};

typedef struct NQWasmTableSection NQWasmTableSection;
struct NQWasmTableSection {
  NQWasmSection base;

  NQWasmTable* tables;
  uint32_t itemCount;
  uint32_t capacity;
};

NQ_EXPORT NQWasmTableSection* NQWasmTableSection_create(void);
NQ_EXPORT NQWasmTableSection* NQWasmTableSection_fromMemory(const void* data, size_t size);
NQ_EXPORT void NQWasmTableSection_init(NQWasmTableSection*);
NQ_EXPORT void NQWasmTableSection_finalize(NQWasmTableSection*);
NQ_EXPORT void NQWasmTableSection_destroy(NQWasmTableSection*);
NQ_EXPORT bool NQWasmTableSection_writeTo(const NQWasmTableSection*, NQWasmWriteCallback write, void* userdata);
NQ_EXPORT bool NQWasmTableSection_addTable(NQWasmTableSection*, uint8_t reftype, uint8_t tabletype, uint64_t minValue, uint64_t maxValue);

static inline NQWasmTable* NQWasmTableSection_getTable(const NQWasmTableSection* thiz, uint32_t index)
{
  return &thiz->tables[index];
}

typedef struct NQWasmMemory NQWasmMemory;
struct NQWasmMemory {
  uint8_t memtype;
  uint64_t minValue;
  uint64_t maxValue;
};

typedef struct NQWasmMemorySection NQWasmMemorySection;
struct NQWasmMemorySection {
  NQWasmSection base;

  NQWasmMemory* memories;
  uint32_t itemCount;
  uint32_t capacity;
};

NQ_EXPORT NQWasmMemorySection* NQWasmMemorySection_create(void);
NQ_EXPORT NQWasmMemorySection* NQWasmMemorySection_fromMemory(const void* data, size_t size);
NQ_EXPORT void NQWasmMemorySection_init(NQWasmMemorySection*);
NQ_EXPORT void NQWasmMemorySection_finalize(NQWasmMemorySection*);
NQ_EXPORT void NQWasmMemorySection_destroy(NQWasmMemorySection*);
NQ_EXPORT bool NQWasmMemorySection_writeTo(const NQWasmMemorySection*, NQWasmWriteCallback write, void* userdata);
NQ_EXPORT bool NQWasmMemorySection_addMemory(NQWasmMemorySection*, uint8_t memtype, uint64_t minValue, uint64_t maxValue);

static inline NQWasmMemory* NQWasmMemorySection_getMemory(const NQWasmMemorySection* thiz, uint32_t index)
{
  return &thiz->memories[index];
}

typedef struct NQWasmGlobal NQWasmGlobal;
struct NQWasmGlobal {
  NQListHead list;

  uint8_t valtype;
  uint8_t mut;
  uint32_t exprSize;
  const uint8_t* expr; // Constant expression including the terminating end opcode
};

typedef struct NQWasmGlobalSection NQWasmGlobalSection;
struct NQWasmGlobalSection {
  NQWasmSection base;

  NQListHead itemList;
  uint32_t itemCount;
};

NQ_EXPORT NQWasmGlobalSection* NQWasmGlobalSection_create(void);
NQ_EXPORT NQWasmGlobalSection* NQWasmGlobalSection_fromMemory(const void* data, size_t size);
NQ_EXPORT void NQWasmGlobalSection_init(NQWasmGlobalSection*);
NQ_EXPORT void NQWasmGlobalSection_finalize(NQWasmGlobalSection*);
NQ_EXPORT void NQWasmGlobalSection_destroy(NQWasmGlobalSection*);
NQ_EXPORT bool NQWasmGlobalSection_writeTo(const NQWasmGlobalSection*, NQWasmWriteCallback write, void* userdata);
NQ_EXPORT bool NQWasmGlobalSection_addGlobal(NQWasmGlobalSection*, uint8_t valtype, uint8_t mut, const void* expr, size_t exprSize);
NQ_EXPORT NQWasmGlobal* NQWasmGlobalSection_getGlobal(const NQWasmGlobalSection*, uint32_t index);

#define NQWasmGlobalSection_firstItem(thiz) ((thiz)->itemList.next != &(thiz)->itemList) \
  ? NQ_CONTAINER_OF((thiz)->itemList.next, NQWasmGlobal, list) : NULL
#define NQWasmGlobalSection_nextItem(thiz, item) ((item)->list.next != &(thiz)->itemList) \
  ? NQ_CONTAINER_OF((item)->list.next, NQWasmGlobal, list) : NULL

typedef struct NQWasmStartSection NQWasmStartSection;
struct NQWasmStartSection {
  NQWasmSection base;

  uint32_t funcidx;
};

NQ_EXPORT NQWasmStartSection* NQWasmStartSection_create(uint32_t funcidx);
NQ_EXPORT NQWasmStartSection* NQWasmStartSection_fromMemory(const void* data, size_t size);
NQ_EXPORT void NQWasmStartSection_init(NQWasmStartSection*, uint32_t funcidx);
NQ_EXPORT void NQWasmStartSection_finalize(NQWasmStartSection*);
NQ_EXPORT void NQWasmStartSection_destroy(NQWasmStartSection*);
NQ_EXPORT bool NQWasmStartSection_writeTo(const NQWasmStartSection*, NQWasmWriteCallback write, void* userdata);

typedef struct NQWasmElement NQWasmElement;
struct NQWasmElement {
  NQListHead list;

  uint8_t flags;              // NQ_WASM_ELEM_FLAG_*
  uint8_t reftype;            // Always funcref unless NQ_WASM_ELEM_FLAG_EXPRS is set
  uint32_t tableidx;          // Non-zero only for active segments with NQ_WASM_ELEM_FLAG_EXPLICIT_TABLE
  uint32_t offsetSize;
  const uint8_t* offset;      // Offset constant expression of active segments, NULL otherwise
  uint32_t itemCount;
  uint32_t exprsSize;
  const uint32_t* funcIndices; // itemCount function indices unless NQ_WASM_ELEM_FLAG_EXPRS is set
  const uint8_t* exprs;       // itemCount constant expressions back-to-back if NQ_WASM_ELEM_FLAG_EXPRS is set
};

static inline bool NQWasmElement_isActive(const NQWasmElement* thiz)
{
  return !(thiz->flags & NQ_WASM_ELEM_FLAG_PASSIVE);
}

static inline bool NQWasmElement_isPassive(const NQWasmElement* thiz)
{
  return (thiz->flags & (NQ_WASM_ELEM_FLAG_PASSIVE | NQ_WASM_ELEM_FLAG_EXPLICIT_TABLE)) == NQ_WASM_ELEM_FLAG_PASSIVE;
}

static inline bool NQWasmElement_isDeclarative(const NQWasmElement* thiz)
{
  return (thiz->flags & (NQ_WASM_ELEM_FLAG_PASSIVE | NQ_WASM_ELEM_FLAG_EXPLICIT_TABLE)) == (NQ_WASM_ELEM_FLAG_PASSIVE | NQ_WASM_ELEM_FLAG_EXPLICIT_TABLE);
}

typedef struct NQWasmElementSection NQWasmElementSection;
struct NQWasmElementSection {
  NQWasmSection base;

  NQListHead itemList;
  uint32_t itemCount;
};

NQ_EXPORT NQWasmElementSection* NQWasmElementSection_create(void);
NQ_EXPORT NQWasmElementSection* NQWasmElementSection_fromMemory(const void* data, size_t size);
NQ_EXPORT void NQWasmElementSection_init(NQWasmElementSection*);
NQ_EXPORT void NQWasmElementSection_finalize(NQWasmElementSection*);
NQ_EXPORT void NQWasmElementSection_destroy(NQWasmElementSection*);
NQ_EXPORT bool NQWasmElementSection_writeTo(const NQWasmElementSection*, NQWasmWriteCallback write, void* userdata);
// Copies the element, its list field is ignored
NQ_EXPORT bool NQWasmElementSection_addElement(NQWasmElementSection*, const NQWasmElement* element);
NQ_EXPORT NQWasmElement* NQWasmElementSection_getElement(const NQWasmElementSection*, uint32_t index);

#define NQWasmElementSection_firstItem(thiz) ((thiz)->itemList.next != &(thiz)->itemList) \
  ? NQ_CONTAINER_OF((thiz)->itemList.next, NQWasmElement, list) : NULL
#define NQWasmElementSection_nextItem(thiz, item) ((item)->list.next != &(thiz)->itemList) \
  ? NQ_CONTAINER_OF((item)->list.next, NQWasmElement, list) : NULL

typedef struct NQWasmLocal NQWasmLocal;
struct NQWasmLocal {
  uint32_t count;
  uint8_t valtype;
};

typedef struct NQWasmCode NQWasmCode;
struct NQWasmCode {
  void* data;                // Owns locals and expr
  const NQWasmLocal* locals; // Groups of locals of the same type
  uint32_t localGroupCount;
  uint32_t exprSize;
  const uint8_t* expr;       // Function body instructions including the terminating end opcode
};

typedef struct NQWasmCodeSection NQWasmCodeSection;
struct NQWasmCodeSection {
  NQWasmSection base;

  NQWasmCode* codes;
  uint32_t itemCount;
  uint32_t capacity;
};

NQ_EXPORT NQWasmCodeSection* NQWasmCodeSection_create(void);
NQ_EXPORT NQWasmCodeSection* NQWasmCodeSection_fromMemory(const void* data, size_t size);
NQ_EXPORT void NQWasmCodeSection_init(NQWasmCodeSection*);
NQ_EXPORT void NQWasmCodeSection_finalize(NQWasmCodeSection*);
NQ_EXPORT void NQWasmCodeSection_destroy(NQWasmCodeSection*);
NQ_EXPORT bool NQWasmCodeSection_writeTo(const NQWasmCodeSection*, NQWasmWriteCallback write, void* userdata);
NQ_EXPORT bool NQWasmCodeSection_addCode(NQWasmCodeSection*, const NQWasmLocal* locals, uint32_t localGroupCount, const void* expr, size_t exprSize);

static inline NQWasmCode* NQWasmCodeSection_getCode(const NQWasmCodeSection* thiz, uint32_t index)
{
  return &thiz->codes[index];
}

typedef struct NQWasmData NQWasmData;
struct NQWasmData {
  NQListHead list;

  uint8_t flags;          // NQ_WASM_DATA_FLAG_*
  uint32_t memidx;        // Non-zero only with NQ_WASM_DATA_FLAG_EXPLICIT_MEMORY
  uint32_t offsetSize;
  const uint8_t* offset;  // Offset constant expression of active segments, NULL otherwise
  uint32_t size;
  const uint8_t* bytes;
};

static inline bool NQWasmData_isActive(const NQWasmData* thiz)
{
  return !(thiz->flags & NQ_WASM_DATA_FLAG_PASSIVE);
}

typedef struct NQWasmDataSection NQWasmDataSection;
struct NQWasmDataSection {
  NQWasmSection base;

  NQListHead itemList;
  uint32_t itemCount;
};

NQ_EXPORT NQWasmDataSection* NQWasmDataSection_create(void);
NQ_EXPORT NQWasmDataSection* NQWasmDataSection_fromMemory(const void* data, size_t size);
NQ_EXPORT void NQWasmDataSection_init(NQWasmDataSection*);
NQ_EXPORT void NQWasmDataSection_finalize(NQWasmDataSection*);
NQ_EXPORT void NQWasmDataSection_destroy(NQWasmDataSection*);
NQ_EXPORT bool NQWasmDataSection_writeTo(const NQWasmDataSection*, NQWasmWriteCallback write, void* userdata);
// Copies the data segment, its list field is ignored
NQ_EXPORT bool NQWasmDataSection_addData(NQWasmDataSection*, const NQWasmData* data);
NQ_EXPORT NQWasmData* NQWasmDataSection_getData(const NQWasmDataSection*, uint32_t index);

#define NQWasmDataSection_firstItem(thiz) ((thiz)->itemList.next != &(thiz)->itemList) \
  ? NQ_CONTAINER_OF((thiz)->itemList.next, NQWasmData, list) : NULL
#define NQWasmDataSection_nextItem(thiz, item) ((item)->list.next != &(thiz)->itemList) \
  ? NQ_CONTAINER_OF((item)->list.next, NQWasmData, list) : NULL

typedef struct NQWasmDataCountSection NQWasmDataCountSection;
struct NQWasmDataCountSection {
  NQWasmSection base;

  uint32_t count;
};

NQ_EXPORT NQWasmDataCountSection* NQWasmDataCountSection_create(uint32_t count);
NQ_EXPORT NQWasmDataCountSection* NQWasmDataCountSection_fromMemory(const void* data, size_t size);
NQ_EXPORT void NQWasmDataCountSection_init(NQWasmDataCountSection*, uint32_t count);
NQ_EXPORT void NQWasmDataCountSection_finalize(NQWasmDataCountSection*);
NQ_EXPORT void NQWasmDataCountSection_destroy(NQWasmDataCountSection*);
NQ_EXPORT bool NQWasmDataCountSection_writeTo(const NQWasmDataCountSection*, NQWasmWriteCallback write, void* userdata);

typedef struct NQWasmImportItem NQWasmImportItem;
struct NQWasmImportItem {
  NQListHead list;
  uint8_t importId;

  const char* module;
  const char* name;

  union {
    struct {
      uint32_t typeidx;
    } function;

    struct {
      uint8_t elemtype;
      uint32_t minValue;
      uint32_t maxValue;
    } table;

    struct {
      uint8_t memtype;
      uint64_t minValue;
      uint64_t maxValue;
    } memory;

    struct {
      uint8_t valtype;
      uint8_t mut;
    } global;
  };
};

typedef struct NQWasmImportSection NQWasmImportSection;
struct NQWasmImportSection {
  NQWasmSection base;

  NQListHead itemList;
  uint32_t itemCount;
};

NQ_EXPORT NQWasmImportSection* NQWasmImportSection_create(void);
NQ_EXPORT NQWasmImportSection* NQWasmImportSection_fromMemory(const void* data, size_t size);
NQ_EXPORT void NQWasmImportSection_init(NQWasmImportSection*);
NQ_EXPORT void NQWasmImportSection_finalize(NQWasmImportSection*);
NQ_EXPORT void NQWasmImportSection_destroy(NQWasmImportSection*);
NQ_EXPORT bool NQWasmImportSection_writeTo(const NQWasmImportSection*, NQWasmWriteCallback write, void* userdata);
NQ_EXPORT bool NQWasmImportSection_addMemory(NQWasmImportSection*, const char* module, const char* name, uint8_t memtype, uint64_t minValue, uint64_t maxValue);

#define NQWasmImportSection_firstItem(thiz) ((thiz)->itemList.next != &(thiz)->itemList) \
  ? NQ_CONTAINER_OF((thiz)->itemList.next, NQWasmImportItem, list) : NULL
#define NQWasmImportSection_nextItem(thiz, item) ((item)->list.next != &(thiz)->itemList) \
  ? NQ_CONTAINER_OF((item)->list.next, NQWasmImportItem, list) : NULL

typedef struct NQWasmExportItem NQWasmExportItem;
struct NQWasmExportItem {
  NQListHead list;
  uint8_t exportId;

  const char* name;
  uint32_t index;
};

typedef struct NQWasmExportSection NQWasmExportSection;
struct NQWasmExportSection {
  NQWasmSection base;

  NQListHead itemList;
  uint32_t itemCount;
};

NQ_EXPORT NQWasmExportSection* NQWasmExportSection_create(void);
NQ_EXPORT NQWasmExportSection* NQWasmExportSection_fromMemory(const void* data, size_t size);
NQ_EXPORT void NQWasmExportSection_init(NQWasmExportSection*);
NQ_EXPORT void NQWasmExportSection_finalize(NQWasmExportSection*);
NQ_EXPORT void NQWasmExportSection_destroy(NQWasmExportSection*);
NQ_EXPORT bool NQWasmExportSection_writeTo(const NQWasmExportSection*, NQWasmWriteCallback write, void* userdata);
NQ_EXPORT bool NQWasmExportSection_addItem(NQWasmExportSection*, const char* name, uint8_t exportId, uint32_t index);
NQ_EXPORT NQWasmExportItem* NQWasmExportSection_findItem(const NQWasmExportSection*, const char* name);

#define NQWasmExportSection_firstItem(thiz) ((thiz)->itemList.next != &(thiz)->itemList) \
  ? NQ_CONTAINER_OF((thiz)->itemList.next, NQWasmExportItem, list) : NULL
#define NQWasmExportSection_nextItem(thiz, item) ((item)->list.next != &(thiz)->itemList) \
  ? NQ_CONTAINER_OF((item)->list.next, NQWasmExportItem, list) : NULL

typedef struct NQWasmModule NQWasmModule; // ? WasmBinaryModule
struct NQWasmModule {
  struct NQWasmHeader header;
  NQListHead sectionList;
};

NQ_EXPORT void NQWasmModule_init(NQWasmModule*);
NQ_EXPORT NQWasmModule* NQWasmModule_fromMemory(const void* data, size_t size);
NQ_EXPORT void NQWasmModule_finalize(NQWasmModule*);
NQ_EXPORT void NQWasmModule_destroy(NQWasmModule*);
NQ_EXPORT bool NQWasmModule_writeTo(const NQWasmModule*, NQWasmWriteCallback write, void* userdata);

#define NQWasmModule_firstSection(thiz) (thiz)->sectionList.next != &(thiz)->sectionList \
  ? NQ_CONTAINER_OF((thiz)->sectionList.next, NQWasmSection, list) : NULL
#define NQWasmModule_nextSection(thiz, section) ((section)->list.next != &(thiz)->sectionList) \
  ? NQ_CONTAINER_OF((section)->list.next, NQWasmSection, list) : NULL

NQ_EXPORT NQWasmSection* NQWasmModule_findSection(const NQWasmModule*, uint8_t sectionId);

// Custom sections may repeat, returns the first one with the name
NQ_EXPORT NQWasmCustomSection* NQWasmModule_findCustomSection(const NQWasmModule*, const char* name);

static inline NQWasmTypeSection* NQWasmModule_findTypeSection(const NQWasmModule* thiz)
{
  return (NQWasmTypeSection*)NQWasmModule_findSection(thiz, NQ_WASM_SECTION_TYPE_ID);
}

static inline NQWasmFunctionSection* NQWasmModule_findFunctionSection(const NQWasmModule* thiz)
{
  return (NQWasmFunctionSection*)NQWasmModule_findSection(thiz, NQ_WASM_SECTION_FUNCTION_ID);
}

static inline NQWasmTableSection* NQWasmModule_findTableSection(const NQWasmModule* thiz)
{
  return (NQWasmTableSection*)NQWasmModule_findSection(thiz, NQ_WASM_SECTION_TABLE_ID);
}

static inline NQWasmMemorySection* NQWasmModule_findMemorySection(const NQWasmModule* thiz)
{
  return (NQWasmMemorySection*)NQWasmModule_findSection(thiz, NQ_WASM_SECTION_MEMORY_ID);
}

static inline NQWasmGlobalSection* NQWasmModule_findGlobalSection(const NQWasmModule* thiz)
{
  return (NQWasmGlobalSection*)NQWasmModule_findSection(thiz, NQ_WASM_SECTION_GLOBAL_ID);
}

static inline NQWasmStartSection* NQWasmModule_findStartSection(const NQWasmModule* thiz)
{
  return (NQWasmStartSection*)NQWasmModule_findSection(thiz, NQ_WASM_SECTION_START_ID);
}

static inline NQWasmElementSection* NQWasmModule_findElementSection(const NQWasmModule* thiz)
{
  return (NQWasmElementSection*)NQWasmModule_findSection(thiz, NQ_WASM_SECTION_ELEMENT_ID);
}

static inline NQWasmCodeSection* NQWasmModule_findCodeSection(const NQWasmModule* thiz)
{
  return (NQWasmCodeSection*)NQWasmModule_findSection(thiz, NQ_WASM_SECTION_CODE_ID);
}

static inline NQWasmDataSection* NQWasmModule_findDataSection(const NQWasmModule* thiz)
{
  return (NQWasmDataSection*)NQWasmModule_findSection(thiz, NQ_WASM_SECTION_DATA_ID);
}

static inline NQWasmDataCountSection* NQWasmModule_findDataCountSection(const NQWasmModule* thiz)
{
  return (NQWasmDataCountSection*)NQWasmModule_findSection(thiz, NQ_WASM_SECTION_DATA_COUNT_ID);
}

static inline NQWasmImportSection* NQWasmModule_findImportSection(const NQWasmModule* thiz)
{
  return (NQWasmImportSection*)NQWasmModule_findSection(thiz, NQ_WASM_SECTION_IMPORT_ID);
}

static inline NQWasmExportSection* NQWasmModule_findExportSection(const NQWasmModule* thiz)
{
  return (NQWasmExportSection*)NQWasmModule_findSection(thiz, NQ_WASM_SECTION_EXPORT_ID);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBNETQ_WASM_MODULE_H */
