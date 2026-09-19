/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/dbc/DBCDocToJSONWriter.h"

#include <libnetq/string/String.h>
#include <libnetq/fs/Path.h>
#include <libnetq/dbc/DBCID.h>
#include <libnetq/dbc/DBCJ1939.h>

bool NQDBCAttributeWriteTo(const NQDBCAttribute* thiz, NQJSONWriter* writer)
{
  const char* name = NQDBCAttribute_name(thiz);
  switch (NQDBCAttribute_type(thiz)) {
  case kNQDBCAttrValueInt:
    if (!NQJSONWriter_writeKeyInt64(writer, name, NQDBCAttribute_asIntValue(thiz)))
      return false;
    break;

  case kNQDBCAttrValueHex:
    if (!NQJSONWriter_writeKeyInt64(writer, name, NQDBCAttribute_asIntValue(thiz)))
      return false;
    break;

  case kNQDBCAttrValueFloat:
    if (!NQJSONWriter_writeKeyDouble(writer, name, NQDBCAttribute_asFloatValue(thiz)))
      return false;
    break;

  case kNQDBCAttrValueString:
    if (!NQJSONWriter_writeKeyString(writer, name, NQDBCAttribute_asStringValue(thiz)))
      return false;
    break;

  case kNQDBCAttrValueEnum:
    if (!NQJSONWriter_writeKeyString(writer, name, NQDBCAttribute_asStringValue(thiz)))
      return false;
    break;
  }

  return true;
}

bool NQDBCSignalWriteTo(const NQDBCSignal* thiz, NQJSONWriter* writer)
{
  if (!NQJSONWriter_writeObjectBegin(writer))
    return false;

  // Name
  {
    const char* name = NQDBCSignal_name(thiz);
    if (!NQJSONWriter_writeKeyString(writer, "name", name))
      return false;
  }

  // Comment
  {
    const char* comment = NQDBCSignal_comment(thiz);
    if (comment != NULL) {
      if (!NQJSONWriter_writeKeyString(writer, "comment", comment))
        return false;
    }
  }

  // Attributes
  {
    size_t attributeCount = NQDBCSignal_attributeCount(thiz);
    if(attributeCount != 0) {
      if (!NQJSONWriter_writeKeyObjectBegin(writer, "attributes"))
        return false;
      for (size_t i = 0; i < attributeCount; i++) {
        const NQDBCAttribute* attr = NQDBCSignal_attributeAt(thiz, i);
        if (!NQDBCAttributeWriteTo(attr, writer))
          return false;
      }
      if (!NQJSONWriter_writeObjectEnd(writer))
        return false;
    }
  }

  // ValueType
  {
    NQDBCSigValueType valueType = NQDBCSignal_valueType(thiz);
    const char* str = NQDBCSigValueType_toCString(valueType);
    if (!NQJSONWriter_writeKeyString(writer, "valueType", str))
      return false;
  }

  // StartBit
  {
    uint32_t startBit = NQDBCSignal_startBit(thiz);
    if (!NQJSONWriter_writeKeyUint32(writer, "startBit", startBit))
      return false;
  }

  // SizeInBits
  {
    uint32_t sizeInBits = NQDBCSignal_sizeInBits(thiz);
    if (!NQJSONWriter_writeKeyUint32(writer, "sizeInBits", sizeInBits))
      return false;
  }

  // ByteOrder
  {
    NQDBCByteOrder byteOrder = NQDBCSignal_byteOrder(thiz);
    const char* str = NQDBCByteOrder_toCString(byteOrder);
    if (!NQJSONWriter_writeKeyString(writer, "byteOrder", str))
      return false;
  }

  // IsUnsigned
  {
    bool isUnsigned = NQDBCSignal_isUnsigned(thiz);
    if (!NQJSONWriter_writeKeyBool(writer, "isUnsigned", isUnsigned))
      return false;
  }

  // Factor
  {
    double factor = NQDBCSignal_factor(thiz);
    if (!NQJSONWriter_writeKeyDouble(writer, "factor", factor))
      return false;
  }

  // Offset
  {
    double offset = NQDBCSignal_offset(thiz);
    if (!NQJSONWriter_writeKeyDouble(writer, "offset", offset))
      return false;
  }

  // Minimum
  {
    double minimum = NQDBCSignal_minimum(thiz);
    if (!NQJSONWriter_writeKeyDouble(writer, "minimum", minimum))
      return false;
  }

  // Maximum
  {
    double maximum = NQDBCSignal_maximum(thiz);
    if (!NQJSONWriter_writeKeyDouble(writer, "maximum", maximum))
      return false;
  }

  // Unit
  {
    const char* unit = NQDBCSignal_unit(thiz);
    if (!NQJSONWriter_writeKeyString(writer, "unit", unit))
      return false;
  }

  // Receiver
  {
    size_t receiverCount = NQDBCSignal_receiverCount(thiz);
    if (receiverCount != 0) {
      if (!NQJSONWriter_writeKeyArrayBegin(writer, "receivers"))
        return false;
      for (size_t index = 0; index < receiverCount; index++) {
        NQDBCNetNode* receiver = NQDBCSignal_receiverAt(thiz, index);
        const char* name = NQDBCNetNode_name(receiver);
        if (!NQJSONWriter_writeString(writer, name))
          return false;
      }
      if (!NQJSONWriter_writeArrayEnd(writer))
        return false;
    }
  }

  // GenSigStartValue
  {
    int64_t startValue;
    if (NQDBCSignal_getAttrInt(thiz, "GenSigStartValue", &startValue)) {
      if (!NQJSONWriter_writeKeyInt64(writer, "startValue", startValue))
        return false;
    }
  }

  if (!NQJSONWriter_writeObjectEnd(writer))
    return false;

  return true;
}

bool NQDBCSigGroupWriteTo(const NQDBCSigGroup* thiz, NQJSONWriter* writer)
{
  if (!NQJSONWriter_writeObjectBegin(writer))
    return false;

  // Name
  {
    const char* name = NQDBCSigGroup_name(thiz);
    if (!NQJSONWriter_writeKeyString(writer, "name", name))
      return false;
  }

  // Signals
  {
    if (!NQJSONWriter_writeKeyArrayBegin(writer, "signals"))
      return false;

    size_t signalCount = NQDBCSigGroup_signalCount(thiz);
    for (size_t i = 0; i < signalCount; i++) {
      const NQDBCSignal* iter = NQDBCSigGroup_signalAt(thiz, i);
      if (!NQDBCSignalWriteTo(iter, writer))
        return false;
    }

    if (!NQJSONWriter_writeArrayEnd(writer))
      return false;
  }

  if (!NQJSONWriter_writeObjectEnd(writer))
    return false;

  return true;
}

bool NQDBCDocumentWriteTo(const NQDBCDocument* thiz, const char* filename, NQJSONWriter* writer)
{
  size_t dbNameLen = 0;
  const char* dbName = NULL;

  if (NQDBCDocument_getAttrString(thiz, "DBName", &dbName)) {
    dbNameLen = strlen(dbName);
  }
  if (dbNameLen == 0 && filename != NULL) {
    dbName = filename;
    const char* ext = NQGetExtname(filename);
    dbNameLen = ext ? ext - filename : strlen(filename);
  }
  if (dbNameLen == 0) {
    dbName = "DBC";
    dbNameLen = 3;
  }

  bool isJ1939 = false;
  const char* protocolType = NULL;
  if (NQDBCDocument_getAttrString(thiz, "ProtocolType", &protocolType)) {
    isJ1939 = strcmp(protocolType, "J1939") == 0;
  }

  const char* busType = NULL;
  NQDBCDocument_getAttrString(thiz, "BusType", &busType);

  if (!NQJSONWriter_writeObjectBegin(writer))
    return false;

  // Name
  {
    if (dbName != NULL) {
      if (!NQJSONWriter_writeKeyString2(writer, "name", dbName, dbNameLen))
        return false;
    }
  }

  // Comment
  {
    const char* comment = NQDBCDocument_comment(thiz);
    if (comment != NULL) {
      if (!NQJSONWriter_writeKeyString(writer, "comment", comment))
        return false;
    }
  }

  // Attributes
  {
    size_t attributeCount = NQDBCDocument_attributeCount(thiz);
    if (attributeCount != 0) {
      if (!NQJSONWriter_writeKeyObjectBegin(writer, "attributes"))
        return false;
      for (size_t i = 0; i < attributeCount; i++) {
        const NQDBCAttribute* attr = NQDBCDocument_attributeAt(thiz, i);
        if (!NQDBCAttributeWriteTo(attr, writer))
          return false;
      }
      if (!NQJSONWriter_writeObjectEnd(writer))
        return false;
    }
  }

  // Version
  {
    const char* version = NQDBCDocument_version(thiz);
    if (version != NULL) {
      if (!NQJSONWriter_writeKeyString(writer, "version", version))
        return false;
    }
  }

  // NewSymbols
  {
    if (!NQJSONWriter_writeKeyArrayBegin(writer, "newSymbols"))
      return false;

    size_t symbolCount = NQDBCDocument_symbolCount(thiz);
    for (size_t i = 0; i < symbolCount; i++) {
      const char* symbol = NQDBCDocument_symbolAt(thiz, i);
      if (!NQJSONWriter_writeString(writer, symbol))
        return false;
    }

    if (!NQJSONWriter_writeArrayEnd(writer))
      return false;
  }

  // BitTiming
  {
    const NQDBCBitTiming* bitTiming = NQDBCDocument_bitTiming(thiz);
    if (bitTiming != NULL) {
      if (!NQJSONWriter_writeKeyObjectBegin(writer, "bitTiming"))
        return false;

      if (!NQJSONWriter_writeKeyUint32(writer, "baudrate", bitTiming->baudrate))
        return false;

      if (!NQJSONWriter_writeKeyUint32(writer, "btr1", bitTiming->btr1))
        return false;

      if (!NQJSONWriter_writeKeyUint32(writer, "btr2", bitTiming->btr2))
        return false;

      if (!NQJSONWriter_writeObjectEnd(writer))
        return false;
    }
  }

  // Messages
  {
    if (!NQJSONWriter_writeKeyArrayBegin(writer, "messages"))
      return false;
    size_t messageCount = NQDBCDocument_messageCount(thiz);
    for (size_t i = 0; i < messageCount; i++) {
      const NQDBCMessage* iter = NQDBCDocument_messageAt(thiz, i);
      if (!NQDBCMessageWriteTo(iter, isJ1939, writer))
        return false;
    }
    if (!NQJSONWriter_writeArrayEnd(writer))
      return false;
  }

  // ProtocolType
  {
    if (protocolType != NULL) {
      if (!NQJSONWriter_writeKeyString(writer, "protocol", protocolType))
        return false;
    }
  }

  // BusType
  {
    if (busType != NULL) {
      if (!NQJSONWriter_writeKeyString(writer, "bus", busType))
        return false;
    }
  }

  return NQJSONWriter_writeObjectEnd(writer);
}

bool NQDBCMessageWriteTo(const NQDBCMessage* thiz, bool isJ1939, NQJSONWriter* writer)
{
  if (!NQJSONWriter_writeObjectBegin(writer))
    return false;

  // Name
  {
    const char* name = NQDBCMessage_name(thiz);
    if (!NQJSONWriter_writeKeyString(writer, "name", name))
      return false;
  }

  // Comment
  {
    const char* comment = NQDBCMessage_comment(thiz);
    if (comment != NULL) {
      if (!NQJSONWriter_writeKeyString(writer, "comment", comment))
        return false;
    }
  }

  // Attributes
  {
    size_t attributeCount = NQDBCMessage_attributeCount(thiz);
    if(attributeCount != 0) {
      if (!NQJSONWriter_writeKeyObjectBegin(writer, "attributes"))
        return false;
      for (size_t i = 0; i < attributeCount; i++) {
        const NQDBCAttribute* attr = NQDBCMessage_attributeAt(thiz, i);
        if (!NQDBCAttributeWriteTo(attr, writer))
          return false;
      }
      if (!NQJSONWriter_writeObjectEnd(writer))
        return false;
    }
  }

  uint32_t id = NQDBCMessage_id(thiz);
  if (!NQJSONWriter_writeKeyUint32(writer, "id", NQDBCGetId(id)))
    return false;

  if (!NQJSONWriter_writeKeyUint32(writer, "idBits", NQDBCGetIdBits(id)))
    return false;

  if (isJ1939) {
    if (!NQJSONWriter_writeKeyObjectBegin(writer, "pdu"))
      return false;

    NQDBCJ1939PDU pdu;
    NQDBCJ1939PDU_init(&pdu, id);

    if (!NQJSONWriter_writeKeyUint8(writer, "version", NQDBCJ1939PDU_version(&pdu)))
      return false;

    if (!NQJSONWriter_writeKeyUint8(writer, "priority", NQDBCJ1939PDU_priority(&pdu)))
      return false;

    if (!NQJSONWriter_writeKeyUint32(writer, "pgn", NQDBCJ1939PDU_pgn(&pdu)))
      return false;

    if (!NQJSONWriter_writeKeyUint8(writer, "sa", NQDBCJ1939PDU_sa(&pdu)))
      return false;

    if (!NQJSONWriter_writeKeyUint8(writer, "da", NQDBCJ1939PDU_da(&pdu)))
      return false;

    if (!NQJSONWriter_writeObjectEnd(writer))
      return false;
  }

  uint32_t sizeInBytes = NQDBCMessage_sizeInBytes(thiz);
  if (!NQJSONWriter_writeKeyUint32(writer, "size", sizeInBytes))
    return false;

  // Groups
  {
    if (!NQJSONWriter_writeKeyArrayBegin(writer, "groups"))
      return false;

    size_t groupCount = NQDBCMessage_groupCount(thiz);
    for (size_t i = 0; i < groupCount; i++) {
      NQDBCSigGroup* iter = NQDBCMessage_groupAt(thiz, i);
      if (!NQDBCSigGroupWriteTo(iter, writer))
        return false;
    }

    if (!NQJSONWriter_writeArrayEnd(writer))
      return false;
  }

  // Signals
  {
    if (!NQJSONWriter_writeKeyArrayBegin(writer, "signals"))
      return false;

    size_t signalCount = NQDBCMessage_signalCount(thiz);
    for (size_t i = 0; i < signalCount; i++) {
      const NQDBCSignal* iter = NQDBCMessage_signalAt(thiz, i);
      if (NQDBCSignal_sigGroup(iter) == NULL) {
        if (!NQDBCSignalWriteTo(iter, writer))
          return false;
      }
    }

    if (!NQJSONWriter_writeArrayEnd(writer))
      return false;
  }

  size_t senderCount = NQDBCMessage_senderCount(thiz);
  if (senderCount != 0) {
    if (!NQJSONWriter_writeKeyArrayBegin(writer, "transmitters"))
      return false;

    for (size_t i = 0; i < senderCount; i++) {
      const NQDBCNetNode* iter = NQDBCMessage_senderAt(thiz, i);
      if (!NQJSONWriter_writeString(writer, NQDBCNetNode_name(iter)))
        return false;
    }

    if (!NQJSONWriter_writeArrayEnd(writer))
      return false;
  }

  if (NQDBCMessage_isPseudo(thiz)) {
    if (!NQJSONWriter_writeKeyBool(writer, "isPseudo", true))
      return false;
  }

  {
    int64_t cycleTime;
    if (NQDBCMessage_getAttrInt(thiz, "GenMsgCycleTime", &cycleTime)) {
      if (!NQJSONWriter_writeKeyInt64(writer, "cycleTime", cycleTime))
        return false;
    }
  }

  if (!NQJSONWriter_writeObjectEnd(writer))
    return false;

  return true;
}
