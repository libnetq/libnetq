/*
 * MIT License
 *
 * Copyright (c) 2025-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/dbc/DBCDocWriter.h"

#include <libnetq/Assert.h>
#include <libnetq/String.h>
#include <libnetq/Log.h>

#include <libnetq/dbc/DBCParser.h>
#include <libnetq/dbc/DBCDocument.h>

struct NQDBCDocWriter {
  void* allocator;
  NQDBCAllocFn* alloc;
  NQDBCFreeFn* free;
  NQDBCParser* parser;
  NQDBCDocument* document;
  NQDBCMessage* lastMessage;
  NQDBCSignal* lastMultiplexerSignal;
  bool hasError;
};

static inline void* DBCAlloc(NQDBCDocWriter* context, size_t size)
{
  return context->alloc(context->allocator, size);
}

static inline void DBCFree(NQDBCDocWriter* context, void* ptr)
{
  context->free(context->allocator, ptr);
}

static void resetLastMessage(NQDBCDocWriter* thiz)
{
  if (thiz->lastMultiplexerSignal != NULL) {
    NQ_ASSERT(thiz->lastMessage != NULL);
    NQDBCMessage_setMultiplexerSignal(thiz->lastMessage, thiz->lastMultiplexerSignal);
    NQDBCSignal_release(thiz->lastMultiplexerSignal);
    thiz->lastMultiplexerSignal = NULL;
  }

  if (thiz->lastMessage != NULL) {
    NQDBCMessage_release(thiz->lastMessage);
    thiz->lastMessage = NULL;
  }
}

NQDBCNetNode* NQDBCDocument_findOrCreateNetNode(NQDBCDocument* thiz, const char* name)
{
  NQDBCNetNode* netNode = NQDBCDocument_findNetNode(thiz, name);
  if (netNode != NULL) {
    return netNode;
  }

  // TODO: Undeclared state
  netNode = NQDBCDocument_createNetNode(thiz, name);
  if (netNode == NULL) {
    NQ_LOGE("Can't create '%s' NetNode", name);
    return NULL;
  }

  if (!NQDBCDocument_addNetNode(thiz, netNode)) {
    NQ_LOGE("Can't add '%s' NetNode to document", name);
    NQDBCNetNode_release(netNode);
    return NULL;
  }

  NQ_LOGW("Created '%s' NetNode", name);

  NQDBCNetNode_release(netNode);
  return netNode;
}

static bool onParserSection(void* userdata, const NQDBCSection* section) {
  NQDBCDocWriter* thiz = (NQDBCDocWriter*)userdata;

  if (section->type != kNQDBCSectionSignal)
    resetLastMessage(thiz);

  switch (section->type) {
  case kNQDBCSectionVersion:
    if (!NQDBCDocument_setVersion(thiz->document, section->version.data))
      return false;
    return true;

  case kNQDBCSectionNewSymbols:
    if (!NQDBCDocument_setSymbols(thiz->document, section->newSymbols.data, section->newSymbols.count))
      return false;
    return true;

  case kNQDBCSectionBitTiming:
    NQDBCDocument_setBitTiming(thiz->document, &section->bitTiming.value);
    return true;

  case kNQDBCSectionNetNodes: {
    size_t i;
    for (i = 0; i < section->netNodes.count; i++) {
      NQDBCNetNode* netNode = NQDBCDocument_createNetNode(thiz->document, section->netNodes.names[i]);
      if (netNode == NULL)
        return false;
      if (!NQDBCDocument_addNetNode(thiz->document, netNode)) {
        NQDBCNetNode_release(netNode);
        return false;
      }
      NQDBCNetNode_release(netNode);
    }
    return true;
  }

  case kNQDBCSectionValTable: {
    NQDBCValTable* valTable = NQDBCDocument_createValTable(thiz->document, section->valTable.name, section->valTable.valDesc, section->valTable.count);
    if (valTable == NULL)
      return false;

    bool success = NQDBCDocument_addValTable(thiz->document, valTable);
    NQDBCValTable_release(valTable);
    return success;
  }

  case kNQDBCSectionMessage: {
    const char* messageName = section->message.name;
    NQDBCMessage* message = NQDBCDocument_createMessage(thiz->document, messageName, section->message.id, section->message.sizeInBytes);
    if (message == NULL)
      return false;

    if (NQStrcmp("Vector__XXX", section->message.transmitter) != 0) {
      const char* transmitterName = section->message.transmitter;
      NQDBCNetNode* transmitter = NQDBCDocument_findOrCreateNetNode(thiz->document, transmitterName);
      if (transmitter == NULL) {
        NQ_LOGE("Can't get transmitter '%s' for message '%s'", transmitterName, messageName);
        return false;
      }
      if (!NQDBCMessage_addTransmitter(message, transmitter)) {
        NQ_LOGE("Can't add transmitter '%s' to message '%s'", transmitterName, messageName);
        NQDBCMessage_release(message);
        return false;
      }
    }

    if (!NQDBCDocument_addMessage(thiz->document, message)) {
      NQDBCMessage_release(message);
      return false;
    }

    // DBCObject_release(message);
    NQ_ASSERT(thiz->lastMessage == NULL);
    thiz->lastMessage = message;

    return true;
  }

  case kNQDBCSectionMessageTransmitter: {
    NQDBCMessage* message = NQDBCDocument_findMessage(thiz->document, section->messageTransmitter.messageId);
    if (message == NULL)
      return false;

    size_t i;
    for (i = 0; i < section->messageTransmitter.count; i++) {
      NQDBCNetNode* transmitter = NQDBCDocument_findNetNode(thiz->document, section->messageTransmitter.transmitters[i]);
      if (transmitter == NULL)
        return false;
      if (!NQDBCMessage_addTransmitter(message, transmitter))
        return false;
    }

    return true;
  }

  case kNQDBCSectionSignal: {
    NQ_ASSERT(thiz->lastMessage != NULL);
    NQDBCSignal* signal = NQDBCDocument_createSignal(thiz->document, section->signal.name, section->signal.startBit, &section->signal.info);
    if (signal == NULL)
      return false;

    if (section->signal.count != 1 || NQStrcmp("Vector__XXX", section->signal.receivers[0]) != 0) {
      size_t i;
      for (i = 0; i < section->signal.count; i++) {
        const char* receiverName = section->signal.receivers[i];
        NQDBCNetNode* receiver = NQDBCDocument_findOrCreateNetNode(thiz->document, receiverName);
        if (receiver == NULL) {
          NQ_LOGE("Can't get receiver '%s' for signal '%s'", receiverName, section->signal.name);
          return false;
        }
        if (!NQDBCSignal_addReceiver(signal, receiver)) {
          NQ_LOGE("Can't add receiver '%s' for signal '%s'", receiverName, section->signal.name);
          NQDBCSignal_release(signal);
          return false;
        }
      }
    }

    if (section->signal.hasMultiplexorValue) {
      if (!NQDBCSignal_addMultiplexerRange(signal, section->signal.multiplexorValue, section->signal.multiplexorValue)) {
        NQDBCSignal_release(signal);
        return false;;
      }
    }

    if (!NQDBCMessage_addSignal(thiz->lastMessage, signal)) {
      NQDBCSignal_release(signal);
      return false;
    }

    if (section->signal.isMultiplexorSwitch && thiz->lastMultiplexerSignal == NULL)
      thiz->lastMultiplexerSignal = signal;
    else
      NQDBCSignal_release(signal);

    return true;
  }

  case kNQDBCSectionSignalGroup: {
    NQDBCMessage* message = NQDBCDocument_findMessage(thiz->document, section->signalGroup.messageId);
    if (message == NULL)
      return false;

    NQDBCSigGroup* sigGroup = NQDBCDocument_createSigGroup(
      thiz->document, section->signalGroup.name,
      section->signalGroup.repetitions
      );
    if (sigGroup == NULL)
      return false;

    if (!NQDBCMessage_addSigGroup(message, sigGroup)) {
      NQDBCSigGroup_release(sigGroup);
      return false;
    }

    size_t i;
    for (i = 0; i < section->signalGroup.count; i++) {
      NQDBCSignal* signal = NQDBCMessage_findSignal(message, section->signalGroup.signals[i]);
      if (!NQDBCSigGroup_addSignal(sigGroup, signal)) {
        NQDBCSigGroup_release(sigGroup);
        return false;
      }
    }

    NQDBCSigGroup_release(sigGroup);
    return true;
  }

  case kNQDBCSectionSignalValue: {
    NQDBCSignal* signal = NQDBCDocument_findSignal(thiz->document, section->signalValue.messageId, section->signalValue.signalName);
    if (signal == NULL) {
      NQ_LOGW("Can't find '%s' signal", section->signalValue.signalName);
      // TODO: Strict
      return true;
    }

    NQDBCValTable* valTable = NQDBCDocument_createValTable(thiz->document, "", section->signalValue.valDesc, section->signalValue.count);
    if (valTable == NULL)
      return false;

    NQDBCSignal_setValTable(signal, valTable);
    NQDBCValTable_release(valTable);
    return true;
  }
  
  case kNQDBCSectionSignalValueType: {
    NQDBCSignal* signal = NQDBCDocument_findSignal(thiz->document, section->signalValueType.messageId, section->signalValueType.signalName);
    if (signal == NULL)
      return false;

    NQDBCSignal_setValueType(signal, section->signalValueType.value);
    return true;
  }

  case kNQDBCSectionEnvVar: {
    NQDBCEnvVar* envVar = NQDBCDocument_createEnvVar(thiz->document, section->envVar.name, &section->envVar.info);
    if (envVar == NULL)
      return false;

    if (section->envVar.count != 1 || NQStrcmp(section->envVar.accessNode[0], "Vector__XXX") != 0) {
      size_t i;
      for (i = 0; i < section->envVar.count; i++) {
        NQDBCNetNode* netNode = NQDBCDocument_findNetNode(thiz->document, section->envVar.accessNode[i]);
        if (netNode == NULL) {
          NQDBCEnvVar_release(envVar);
          return false;
        }
        if (!NQDBCEnvVar_addAccessNode(envVar, netNode)) {
          NQDBCEnvVar_release(envVar);
          return false;
        }
      }
    }

    if (!NQDBCDocument_addEnvVar(thiz->document, envVar)) {
      NQDBCEnvVar_release(envVar);
      return false;
    }

    NQDBCEnvVar_release(envVar);
    return true;
  }
  
  case kNQDBCSectionSignalType: {
    NQDBCValTable* valTable = NQDBCDocument_findValTable(thiz->document, section->signalType.valTable);
    if (valTable == NULL)
      return false;

    NQDBCSigType* sigType = NQDBCDocument_createSigType(thiz->document, section->signalType.name, &section->signalType.info, section->signalType.defaultValue);
    if (sigType == NULL)
      return false;

    if (!NQDBCDocument_addSigType(thiz->document, sigType)) {
      NQDBCSigType_release(sigType);
      return false;
    }

    NQDBCSigType_setValTable(sigType, valTable);
    NQDBCSigType_release(sigType);
    return true;
  }
  
  case kNQDBCSectionSignalTypeRef: {
    NQDBCSigType* sigType = NQDBCDocument_findSigType(thiz->document, section->signalTypeRef.signalTypeName);
    if (sigType == NULL)
      return false;

    NQDBCSignal* signal = NQDBCDocument_findSignal(thiz->document, section->signalTypeRef.messageId, section->signalTypeRef.signalName);
    if (signal == NULL)
      return false;

    NQDBCSignal_setSignalType(signal, sigType);
    return true;
  }

  case kNQDBCSectionEnvVarData: {
    NQDBCEnvVar* envVar = NQDBCDocument_findEnvVar(thiz->document, section->envVarData.name);
    if (envVar == NULL)
      return false;

    if (!NQDBCEnvVar_setDataSize(envVar, section->envVarData.dataSize))
      return false;

    return true;
  }
   
  case kNQDBCSectionEnvVarValue: {
    NQDBCEnvVar* envVar = NQDBCDocument_findEnvVar(thiz->document, section->envVarValue.name);
    if (envVar == NULL)
      return false;

    NQDBCValTable* valTable = NQDBCDocument_createValTable(thiz->document, "", section->envVarValue.valDesc, section->envVarValue.count);
    if (valTable == NULL)
      return false;

    NQDBCEnvVar_setValTable(envVar, valTable);
    NQDBCValTable_release(valTable);
    return true;
  }

  case kNQDBCSectionSignalMultiplexed: {
    NQDBCSignal* multiplexedSignal = NQDBCDocument_findSignal(thiz->document, section->signalMultiplexed.messageId, section->signalMultiplexed.signalName);
    if (multiplexedSignal == NULL)
      return false;

    NQDBCSignal* multiplexorSwitch = NQDBCDocument_findSignal(thiz->document, section->signalMultiplexed.messageId, section->signalMultiplexed.switchName);
    if (multiplexorSwitch == NULL)
      return false;

    if (!NQDBCSignal_setMultiplexerSignal(multiplexedSignal, multiplexorSwitch))
      return false;

    size_t i;
    for (i = 0; i < section->signalMultiplexed.count; i++) {
      if (!NQDBCSignal_addMultiplexerRange(multiplexedSignal, section->signalMultiplexed.range[i].first, section->signalMultiplexed.range[i].second)) {
        return false;
      }
    }

    return true;
  }

  case kNQDBCSectionComment: {
    switch (section->comment.target.objectType) {
    case kNQDBCObjectDocument: {
      return NQDBCDocument_setComment(thiz->document, section->comment.value);
    }
    case kNQDBCObjectNetNode: {
      const char* netNodeName = section->comment.target.netNode.name;
      NQDBCNetNode* netNode = NQDBCDocument_findNetNode(thiz->document, netNodeName);
      if (netNode == NULL) {
        NQ_LOGW("Can't find '%s' NetNode", netNodeName);
        return true;
      }
      return NQDBCNetNode_setComment(netNode, section->comment.value);
    }
    case kNQDBCObjectMessage: {
      uint32_t messageId = section->comment.target.message.id;
      NQDBCMessage* message = NQDBCDocument_findMessage(thiz->document, messageId);
      if (message == NULL) {
        NQ_LOGW("Can't find %u Message", messageId);
        return true;
      }
      return NQDBCMessage_setComment(message, section->comment.value);
    }
    case kNQDBCObjectSignal: {
      uint32_t messageId = section->comment.target.signal.messageId;
      NQDBCMessage* message = NQDBCDocument_findMessage(thiz->document, messageId);
      if (message == NULL) {
        NQ_LOGW("Can't find %u Message", messageId);
        return true;
      }
      const char* signalName = section->comment.target.signal.name;
      NQDBCSignal* signal = NQDBCMessage_findSignal(message, signalName);
      if (signal == NULL) {
        NQ_LOGW("Can't find '%s' Signal of %u message", signalName, messageId);
        return true;
      }
      return NQDBCSignal_setComment(signal, section->comment.value);
    }
    case kNQDBCObjectEnvVar: {
      const char* envVarName = section->comment.target.envVar.name;
      NQDBCEnvVar* envVar = NQDBCDocument_findEnvVar(thiz->document, envVarName);
      if (envVar == NULL) {
        NQ_LOGW("Can't find '%s' EnvVar", envVarName);
        return true;
      }
      return NQDBCEnvVar_setComment(envVar, section->comment.value);
    }
    default:
      return false;
    }
  }

  case kNQDBCSectionAttr: {
    struct NQDBCAttrValueInfo attrValueInfo;
    switch (section->attr.target.objectType) {
    case kNQDBCObjectDocument: {
      switch (section->attr.value.type) {
      case kNQDBCVariantInt:
      case kNQDBCVariantUint:
        if (NQDBCDocument_setAttrInt(thiz->document, section->attr.name, section->attr.value.vInt))
          return true;

        attrValueInfo.type = kNQDBCAttrValueInt;
        attrValueInfo.vInt.first = 0;
        attrValueInfo.vInt.second = 0;

        if (!NQDBCDocument_defineDocumentAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          NQ_LOGE("Can't create '%s' INT attribute for Document", section->attr.name);
          return false;
        }

        NQ_LOGW("Created '%s' INT attribute for Document", section->attr.name);
        return NQDBCDocument_setAttrInt(thiz->document, section->attr.name, section->attr.value.vInt);

      case kNQDBCVariantFloat:
        if (NQDBCDocument_setAttrFloat(thiz->document, section->attr.name, section->attr.value.vFloat))
          return true;

        attrValueInfo.type = kNQDBCAttrValueFloat;
        attrValueInfo.vFloat.first = 0;
        attrValueInfo.vFloat.second = 0;

        if (!NQDBCDocument_defineDocumentAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          NQ_LOGE("Can't create '%s' FLOAT attribute for Document", section->attr.name);
          return false;
        }

        NQ_LOGW("Created '%s' FLOAT attribute for Document", section->attr.name);
        return NQDBCDocument_setAttrFloat(thiz->document, section->attr.name, section->attr.value.vFloat);

      case kNQDBCVariantString:
        if (!NQDBCDocument_setAttrString(thiz->document, section->attr.name, section->attr.value.vString)) {
          NQ_LOGW("Can't set '%s' value to attribute '%s'", section->attr.value.vString, section->attr.name);
        }
        break;

      default:
        NQ_ASSERT_NOT_REACHED();
        break;
      }
      break;
    }

    case kNQDBCObjectNetNode: {
      NQDBCNetNode* netNode = NQDBCDocument_findNetNode(thiz->document, section->attr.target.netNode.name);
      if (netNode == NULL) {
        NQ_LOGW("Can't find '%s' network node", section->attr.target.netNode.name);
        return true;
      }

      switch (section->attr.value.type) {
      case kNQDBCVariantInt:
      case kNQDBCVariantUint:
        if (NQDBCNetNode_setAttrInt(netNode, section->attr.name, section->attr.value.vInt))
          return true;

        attrValueInfo.type = kNQDBCAttrValueInt;
        attrValueInfo.vInt.first = 0;
        attrValueInfo.vInt.second = 0;

        if (!NQDBCDocument_defineNetNodeAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          NQ_LOGE("Can't create '%s' INT attribute for NetNode", section->attr.name);
          return false;
        }

        NQ_LOGW("Created '%s' INT attribute for NetNode", section->attr.name);
        return NQDBCNetNode_setAttrInt(netNode, section->attr.name, section->attr.value.vInt);

      case kNQDBCVariantFloat:
        if (NQDBCNetNode_setAttrFloat(netNode, section->attr.name, section->attr.value.vFloat))
          return true;

        attrValueInfo.type = kNQDBCAttrValueFloat;
        attrValueInfo.vFloat.first = 0;
        attrValueInfo.vFloat.second = 0;

        if (!NQDBCDocument_defineNetNodeAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          NQ_LOGE("Can't create '%s' FLOAT attribute for NetNode", section->attr.name);
          return false;
        }

        NQ_LOGW("Created '%s' FLOAT attribute for NetNode", section->attr.name);
        return NQDBCNetNode_setAttrFloat(netNode, section->attr.name, section->attr.value.vFloat);

      case kNQDBCVariantString:
        if (!NQDBCNetNode_setAttrString(netNode, section->attr.name, section->attr.value.vString)) {
          NQ_LOGW("Can't set '%s' value to attribute '%s'", section->attr.value.vString, section->attr.name);
        }
        break;

      default:
        NQ_ASSERT_NOT_REACHED();
        break;
      }
      break;
    }

    case kNQDBCObjectMessage: {
      NQDBCMessage* message = NQDBCDocument_findMessage(thiz->document, section->attr.target.message.id);
      if (message == NULL) {
        NQ_LOGW("Can't find %u message", section->attr.target.message.id);
        return true;
      }

      switch (section->attr.value.type) {
      case kNQDBCVariantInt:
      case kNQDBCVariantUint:
        if (NQDBCMessage_setAttrInt(message, section->attr.name, section->attr.value.vInt))
          return true;

        attrValueInfo.type = kNQDBCAttrValueInt;
        attrValueInfo.vInt.first = 0;
        attrValueInfo.vInt.second = 0;

        if (!NQDBCDocument_defineMessageAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          NQ_LOGE("Can't create '%s' INT attribute for Message", section->attr.name);
          return false;
        }

        NQ_LOGW("Created '%s' INT attribute for Message", section->attr.name);
        return NQDBCMessage_setAttrInt(message, section->attr.name, section->attr.value.vInt);

      case kNQDBCVariantFloat:
        if (NQDBCMessage_setAttrFloat(message, section->attr.name, section->attr.value.vFloat))
          return true;

        attrValueInfo.type = kNQDBCAttrValueFloat;
        attrValueInfo.vFloat.first = 0;
        attrValueInfo.vFloat.second = 0;

        if (!NQDBCDocument_defineMessageAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          NQ_LOGE("Can't create '%s' FLOAT attribute for Message", section->attr.name);
          return false;
        }

        NQ_LOGW("Created '%s' FLOAT attribute for Message", section->attr.name);
        return NQDBCMessage_setAttrFloat(message, section->attr.name, section->attr.value.vFloat);

      case kNQDBCVariantString:
        if (!NQDBCMessage_setAttrString(message, section->attr.name, section->attr.value.vString)) {
          NQ_LOGW("Can't set '%s' value to attribute '%s'", section->attr.value.vString, section->attr.name);
        }
        break;

      default:
        NQ_ASSERT_NOT_REACHED();
        break;
      }
      break;
    }

    case kNQDBCObjectSignal: {
      NQDBCMessage* message = NQDBCDocument_findMessage(thiz->document, section->attr.target.signal.messageId);
      if (message == NULL) {
        NQ_LOGW("Can't find %u message", section->attr.target.signal.messageId);
        return true;
      }

      NQDBCSignal* signal = NQDBCMessage_findSignal(message, section->attr.target.signal.name);
      if (signal == NULL) {
        NQ_LOGW("Can't find '%s' signal", section->attr.target.signal.name);
        return true;
      }

      switch (section->attr.value.type) {
      case kNQDBCVariantInt:
      case kNQDBCVariantUint:
        if (NQDBCSignal_setAttrInt(signal, section->attr.name, section->attr.value.vInt))
          return true;

        attrValueInfo.type = kNQDBCAttrValueInt;
        attrValueInfo.vInt.first = 0;
        attrValueInfo.vInt.second = 0;

        if (!NQDBCDocument_defineSignalAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          NQ_LOGE("Can't create '%s' INT attribute for Signal", section->attr.name);
          return false;
        }

        NQ_LOGW("Created '%s' INT attribute for Signal", section->attr.name);
        return NQDBCSignal_setAttrInt(signal, section->attr.name, section->attr.value.vInt);

      case kNQDBCVariantFloat:
        if (NQDBCSignal_setAttrFloat(signal, section->attr.name, section->attr.value.vFloat))
          return true;

        attrValueInfo.type = kNQDBCAttrValueFloat;
        attrValueInfo.vFloat.first = 0;
        attrValueInfo.vFloat.second = 0;

        if (!NQDBCDocument_defineSignalAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          NQ_LOGE("Can't create '%s' FLOAT attribute for Signal", section->attr.name);
          return false;
        }

        NQ_LOGW("Created '%s' FLOAT attribute for Signal", section->attr.name);
        return NQDBCSignal_setAttrFloat(signal, section->attr.name, section->attr.value.vFloat);

      case kNQDBCVariantString:
        if (!NQDBCSignal_setAttrString(signal, section->attr.name, section->attr.value.vString)) {
          NQ_LOGW("Can't set '%s' value to attribute '%s'", section->attr.value.vString, section->attr.name);
        }
        break;

      default:
        NQ_ASSERT_NOT_REACHED();
        break;
      }
      break;
    }

    case kNQDBCObjectEnvVar: {
      NQDBCEnvVar* envVar = NQDBCDocument_findEnvVar(thiz->document, section->attr.target.envVar.name);
      if (envVar == NULL) {
        NQ_LOGW("Can't find '%s' environment variable", section->attr.target.envVar.name);
        return true;
      }

      switch (section->attr.value.type) {
      case kNQDBCVariantInt:
      case kNQDBCVariantUint:
        if (NQDBCEnvVar_setAttrInt(envVar, section->attr.name, section->attr.value.vInt))
          return true;

        attrValueInfo.type = kNQDBCAttrValueInt;
        attrValueInfo.vInt.first = 0;
        attrValueInfo.vInt.second = 0;

        if (!NQDBCDocument_defineEnvVarAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          NQ_LOGE("Can't create '%s' INT attribute for EnvVar", section->attr.name);
          return false;
        }

        NQ_LOGW("Created '%s' INT attribute for EnvVar", section->attr.name);
        return NQDBCEnvVar_setAttrInt(envVar, section->attr.name, section->attr.value.vInt);

      case kNQDBCVariantFloat:
        if (NQDBCEnvVar_setAttrFloat(envVar, section->attr.name, section->attr.value.vFloat))
          return true;

        attrValueInfo.type = kNQDBCAttrValueFloat;
        attrValueInfo.vFloat.first = 0;
        attrValueInfo.vFloat.second = 0;

        if (!NQDBCDocument_defineEnvVarAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          NQ_LOGE("Can't create '%s' FLOAT attribute for EnvVar", section->attr.name);
          return false;
        }

        NQ_LOGW("Created '%s' FLOAT attribute for EnvVar", section->attr.name);
        return NQDBCEnvVar_setAttrFloat(envVar, section->attr.name, section->attr.value.vFloat);

      case kNQDBCVariantString:
        if (NQDBCEnvVar_setAttrString(envVar, section->attr.name, section->attr.value.vString))
          return true;

        attrValueInfo.type = kNQDBCAttrValueString;

        if (!NQDBCDocument_defineEnvVarAttr(thiz->document, section->attr.name, &attrValueInfo)) {
          NQ_LOGE("Can't create '%s' STRING attribute for EnvVar", section->attr.name);
          return false;
        }

        NQ_LOGW("Created '%s' STRING attribute for EnvVar", section->attr.name);
        return NQDBCEnvVar_setAttrString(envVar, section->attr.name, section->attr.value.vString);

      default:
        NQ_ASSERT_NOT_REACHED();
        break;
      }
      break;
    }

    }

    return true;
  }

  case kNQDBCSectionAttrRel: {
    return true;
  }

  case kNQDBCSectionAttrDefinition: {
    switch (section->attrDefinition.objectType) {
    case kNQDBCObjectDocument:
      return NQDBCDocument_defineDocumentAttr(thiz->document, section->attrDefinition.name, &section->attrDefinition.value);
    case kNQDBCObjectNetNode:
      return NQDBCDocument_defineNetNodeAttr(thiz->document, section->attrDefinition.name, &section->attrDefinition.value);
    case kNQDBCObjectMessage:
      return NQDBCDocument_defineMessageAttr(thiz->document, section->attrDefinition.name, &section->attrDefinition.value);
    case kNQDBCObjectSignal:
      return NQDBCDocument_defineSignalAttr(thiz->document, section->attrDefinition.name, &section->attrDefinition.value);
    case kNQDBCObjectEnvVar:
      return NQDBCDocument_defineEnvVarAttr(thiz->document, section->attrDefinition.name, &section->attrDefinition.value);
    default:
      NQ_ASSERT_NOT_REACHED();
      break;
    }

    return false;
  }

  case kNQDBCSectionAttrRelDefinition: {
    return true;
  }

  case kNQDBCSectionAttrDefault: {
    NQDBCAttrProto* attrProto = NQDBCDocument_findAttrProto(thiz->document, section->attrDefault.name);
    if (attrProto == NULL) {
      NQ_LOGW("Can't find propotype '%s'", section->attrDefault.name);
      return true;
    }

    switch (section->attrDefault.value.type) {
    case kNQDBCVariantInt:
    case kNQDBCVariantUint:
      if (!NQDBCAttrProto_setDefaultInt(attrProto, section->attrDefault.value.vInt)) {
        NQ_LOGW("Can't set %lli as default for attribute '%s'", (long long)section->attrDefault.value.vInt, section->attrDefault.name);
      }
      break;

    case kNQDBCVariantFloat:
      if (!NQDBCAttrProto_setDefaultFloat(attrProto, section->attrDefault.value.vFloat)) {
        NQ_LOGW("Can't set %f as default for attribute '%s'", (double)section->attrDefault.value.vFloat, section->attrDefault.name);
      }
      break;

    case kNQDBCVariantString:
      if (!NQDBCAttrProto_setDefaultString(attrProto, section->attrDefault.value.vString)) {
        NQ_LOGW("Can't set '%s' as default for attribute '%s'", section->attrDefault.value.vString, section->attrDefault.name);
      }
      break;

    default:
      NQ_ASSERT_NOT_REACHED();
      break;
    }

    return true;
  }

  case kNQDBCSectionAttrRelDefault: {
    return true;
  }

  }

  return false;
}

NQDBCDocWriter* NQDBCDocWriter_create(const char* name)
{
  return NQDBCDocWriter_create2(name, NULL, &NQDBCAllocDefault, &NQDBCFreeDefault);
}

NQDBCDocWriter* NQDBCDocWriter_create2(const char* name, void* allocator, NQDBCAllocFn* alloc, NQDBCFreeFn* free)
{
  if (alloc == NULL) {
    return NULL;
  }
  if (free == NULL) {
    free = &NQDBCFreeNope;
  }

  NQDBCDocWriter* thiz = (NQDBCDocWriter*)alloc(allocator, sizeof(struct NQDBCDocWriter));
  if (thiz == NULL)
    return NULL;

  thiz->document = NQDBCDocument_create2(name, allocator, alloc, free);
  if (thiz->document == NULL) {
    free(allocator, thiz);
    return NULL;
  }

  thiz->parser = NQDBCParser_create2(thiz, &onParserSection, allocator, alloc, free);
  if (thiz->parser == NULL) {
    NQDBCDocument_release(thiz->document);
    free(allocator, thiz);
    return NULL;
  }

  thiz->allocator = allocator;
  thiz->alloc = alloc;
  thiz->free = free;

  thiz->lastMessage = NULL;
  thiz->lastMultiplexerSignal = NULL;
  thiz->hasError = false;

  return thiz;
}

void NQDBCDocWriter_destroy(NQDBCDocWriter* thiz)
{
  void* allocator = thiz->allocator;
  NQDBCFreeFn* freeFn = thiz->free;

  if (thiz->parser != NULL)
    NQDBCParser_destroy(thiz->parser);

  if (thiz->document != NULL)
    NQDBCDocument_release(thiz->document);

  freeFn(allocator, thiz);
}

bool NQDBCDocWriter_write(NQDBCDocWriter* thiz, const void* data, size_t size)
{
  if (thiz->document == NULL)
    return false;

  if (!NQDBCParser_append(thiz->parser, (const char*)data, size))
    return false;

  return true;
}

NQDBCDocument* NQDBCDocWriter_toDocument(NQDBCDocWriter* thiz)
{
  if (thiz->hasError)
    return NULL;

  NQDBCDocument* result = thiz->document;
  if (result != NULL) {
    if (!NQDBCParser_finish(thiz->parser)) {
      thiz->hasError = true;
      return NULL;
    }
    resetLastMessage(thiz);
    NQDBCParser_destroy(thiz->parser);
    thiz->parser = NULL;
    thiz->document = NULL;
  }

  return result;
}
