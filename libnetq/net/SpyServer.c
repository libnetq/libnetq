/*
 * MIT License
 *
 * Copyright (c) 2020-2026  Yurii Yakubin (yurii.yakubin@gmail.com)
 *
 * Permission is granted to use, copy, modify, and distribute this software
 * under the MIT License. See LICENSE file for details.
 */

#include "config.h"
#include "libnetq/net/SpyServer.h"

#include <libnetq/net/SocketHandle.h>
#include <libnetq/PlatformPoll.h>
#include <libnetq/Malloc.h>
#include <libnetq/ErrorCode.h>
#include <libnetq/Time.h>
#include <libnetq/String.h>
#include <libnetq/net/NetBufferQueue.h>
#include <libnetq/Assert.h>

#include <libnetq/net/SpyModuleList.h>

typedef struct NQSpyNode NQSpyNode;
typedef struct NQSpyConnection NQSpyConnection;

typedef enum NQSpyNodeState {
  SOCK_CLOSE_STATE,
  SOCK_INIT_STATE,
  SOCK_OPEN_STATE,
} NQSpyNodeState;

typedef enum NQSpyNodeType {
  SPY_CLIENT_NODE_TYPE,
  SPY_REMOTE_NODE_TYPE,
} NQSpyNodeType;

struct NQSpyNode {
  NQSpyServer* server;
  NQSpyConnection* conn;
  NQSpyNode* partner;

  NQSpyNodeType type;
  NQSpyNodeState state;

  NQSpyClient client;
  char address[32];
  NQSocketHandle sock;
  NQNetBufferQueue bufferQueue;
};

struct NQSpyConnection {
  NQSpyConnection* next;
  NQSpyConnection* prev;

  NQSpyServer* server;
  NQSpyNode clientNode;
  NQSpyNode remoteNode;
};

typedef struct NQSpyConnectionList {
  NQSpyConnection* first;
  size_t size;
} NQSpyConnectionList;

#define MAX_CONN 32
#define MAX_POLL (2 * MAX_CONN + 1)

struct NQSpyServer {
  NQEndPoint clientEndpoint;
  NQEndPoint remoteEndpoint;

  NQSocketHandle sock;
  bool stopLoop;

  NQSpyModuleList modules;

  NQSpyConnection conns[MAX_CONN];

  NQSpyConnectionList connActiveList;
  NQSpyConnectionList connFreeList;
};

static void NQSpyServer_closeConnection(NQSpyServer* server, NQSpyConnection* conn);

static void NQSpyNode_onBufferReceive(NQSpyNode* node, NQNetBufferEntry* bufEntry)
{
  if (node->type == SPY_REMOTE_NODE_TYPE)
    NQSpyModuleList_doRemoteReceive(&node->server->modules, &node->client, bufEntry->bytes, bufEntry->size);
  else if (node->type == SPY_CLIENT_NODE_TYPE)
    NQSpyModuleList_doClientReceive(&node->server->modules, &node->client, bufEntry->bytes, bufEntry->size);
}

static void NQSpyNode_onSocketRecv(NQSpyNode* node, NQSocketHandle sock)
{
  NQ_ASSERT(node->sock == sock);

  if (node->state != SOCK_OPEN_STATE)
    return;

  int sz;

  NQNetBufferEntry* bufEntry = NQNetBufferQueue_alloc(&node->partner->bufferQueue);
  if (bufEntry == NULL) {
    NQSpyServer_closeConnection(node->server, node->conn);
    return;
  }

  sz = NQSocketRecv(sock, bufEntry->bytes, sizeof(bufEntry->bytes), 0);
  if (sz <= 0) {
    NQSpyServer_closeConnection(node->server, node->conn);
    return;
  }

  // bufEntry->time = NQGetTime();
  bufEntry->size = (uint16_t)sz;

  NQSpyNode_onBufferReceive(node, bufEntry);
}

static void NQSpyNode_onBufferSend(NQSpyNode* node, const NQNetBufferEntry* bufEntry)
{
  if (node->type == SPY_REMOTE_NODE_TYPE)
    NQSpyModuleList_doRemoteSend(&node->server->modules, &node->client, bufEntry->bytes, bufEntry->size);
  else if (node->type == SPY_CLIENT_NODE_TYPE)
    NQSpyModuleList_doClientSend(&node->server->modules, &node->client, bufEntry->bytes, bufEntry->size);
}

static void NQSpyNode_onSocketSend(NQSpyNode* node, NQSocketHandle sock)
{
  const NQNetBufferEntry* entry;
  NQ_ASSERT(node->sock == sock);

  if (node->state == SOCK_INIT_STATE) {
    int value;
    NQSocketGetOptInt(node->sock, NQ_SOL_SOCKET, NQ_SO_ERROR, &value);
    if (value == 0) {
      node->state = SOCK_OPEN_STATE;
      NQSpyModuleList_doRemoteConnect(&node->server->modules, &node->client);
    }
    else
      NQSpyServer_closeConnection(node->server, node->conn);
  }
  else if (node->state == SOCK_OPEN_STATE) {
    entry = NQNetBufferQueue_shift(&node->bufferQueue);
    NQ_ASSERT(entry);
    if (NQSocketSend(node->sock, entry->bytes, entry->size, 0) != entry->size)
      NQSpyServer_closeConnection(node->server, node->conn);
    else {
      NQSpyNode_onBufferSend(node, entry);
    }
  }
}

static void NQSpyNode_onSocketClose(NQSpyNode* node, NQSocketHandle sock)
{
  NQ_ASSERT(node->sock == sock);

  if (node->state == SOCK_OPEN_STATE)
    NQSpyServer_closeConnection(node->server, node->conn);
}

static void NQSpyNode_onSocketError(NQSpyNode* node, NQSocketHandle sock)
{
  NQ_ASSERT(node->sock == sock);

  if (node->state == SOCK_OPEN_STATE)
    NQSpyServer_closeConnection(node->server, node->conn);
}

static bool NQSpyConnectionList_isEmpty(const NQSpyConnectionList* list)
{
  return list->first == NULL;
}

static void NQSpyConnectionList_remove(NQSpyConnectionList* list, NQSpyConnection* conn)
{
  if (conn->next)
    conn->next->prev = conn->prev;

  if (conn->prev)
    conn->prev->next = conn->next;
  else
    list->first = conn->next;
  list->size--;
}

static NQSpyConnection* NQSpyConnectionList_removeFirst(NQSpyConnectionList* list)
{
  NQSpyConnection* result = list->first;
  NQSpyConnectionList_remove(list, list->first);
  return result;
}

static void NQSpyConnectionList_unshift(NQSpyConnectionList* list, NQSpyConnection* conn)
{
  conn->next = list->first;
  conn->prev = NULL;
  if (conn->next)
    conn->next->prev = conn;
  list->first = conn;
  list->size++;
}

static void NQSpyServer_init(NQSpyServer* server, const NQSpyServerOptions* options)
{
  size_t i;
  memset(server, 0, sizeof(*server));

  NQSpyModuleList_init(&server->modules);

  NQEndPoint_parse(&server->clientEndpoint, options->listenAddress);
  NQEndPoint_parse(&server->remoteEndpoint, options->targetAddress);

  for (i = 0; i < MAX_CONN; i++) {
    server->conns[i].clientNode.state = SOCK_CLOSE_STATE;
    server->conns[i].remoteNode.state = SOCK_CLOSE_STATE;
    NQSpyConnectionList_unshift(&server->connFreeList, &server->conns[i]);
  }
}

NQSpyServer* NQSpyServer_create(const NQSpyServerOptions* options)
{
  NQSpyServer* server;
  if (options == NULL)
    return NULL;

  server = (NQSpyServer*)NQMalloc(sizeof(NQSpyServer));
  if (server == NULL)
    return NULL;

  NQSpyServer_init(server, options);

  return server;
}

void NQSpyServer_destroy(NQSpyServer* server)
{
  NQSpyModuleList_finalize(&server->modules);

  NQFree((void*)server);
}

bool NQSpyServer_addModule(NQSpyServer* server, const NQSpyModule* module)
{
  return NQSpyModuleList_add(&server->modules, module);
}

static void NQSpyServer_closeConnection(NQSpyServer* server, NQSpyConnection* conn)
{
  NQ_ASSERT(server == conn->server);

  if (conn->clientNode.state == SOCK_INIT_STATE || conn->clientNode.state == SOCK_OPEN_STATE) {
    NQSocketClose(conn->clientNode.sock);
    NQSpyModuleList_doClientClose(&server->modules, &conn->clientNode.client);
  }
  conn->clientNode.state = SOCK_CLOSE_STATE;

  if (conn->remoteNode.state == SOCK_INIT_STATE || conn->remoteNode.state == SOCK_OPEN_STATE) {
    NQSocketClose(conn->remoteNode.sock);
    NQSpyModuleList_doRemoteClose(&server->modules, &conn->remoteNode.client);
  }
  conn->remoteNode.state = SOCK_CLOSE_STATE;
}

static void NQSpyServer_onSocketAccept(NQSpyServer* server, NQSocketHandle sock)
{
  NQ_ASSERT(server->sock == sock);

  int ret;
  NQSocketHandle clientSocket, remoteSocket;
  NQEndPoint clientEndpoint, remoteEndpoint;
  NQSpyConnection* conn;

  if (NQSocketAccept(server->sock, &clientEndpoint, &clientSocket) < 0)
    return;

  if (NQSpyConnectionList_isEmpty(&server->connFreeList)) {
    NQSocketClose(clientSocket);
    return;
  }

  if (NQSocketSetNonBlocking(clientSocket, true) != 0) {
    NQSocketClose(clientSocket);
    return;
  }

  if (NQSocketOpen(NQ_AF_INET, NQ_SOCK_STREAM, 0, &remoteSocket) != 0) {
    NQSocketClose(clientSocket);
    return;
  }

  if (NQSocketSetNonBlocking(remoteSocket, true) != 0) {
    NQSocketClose(clientSocket);
    NQSocketClose(remoteSocket);
    return;
  }

  remoteEndpoint = server->remoteEndpoint;
  ret = NQSocketConnect(remoteSocket, &remoteEndpoint);
  if (ret != 0 && ret != -NQ_EINPROGRESS && ret != -NQ_EWOULDBLOCK) {
    NQSocketClose(clientSocket);
    NQSocketClose(remoteSocket);
    return;
  }

  conn = NQSpyConnectionList_removeFirst(&server->connFreeList);

  conn->server = server;

  conn->clientNode.type = SPY_CLIENT_NODE_TYPE;
  conn->clientNode.sock = clientSocket;
  conn->clientNode.server = server;
  conn->clientNode.conn = conn;
  conn->clientNode.partner = &conn->remoteNode;
  conn->clientNode.state = SOCK_OPEN_STATE;
  NQEndPoint_sprintf(&clientEndpoint, conn->clientNode.address, sizeof(conn->clientNode.address));
  NQNetBufferQueue_init(&conn->clientNode.bufferQueue);
  conn->clientNode.client.address = conn->clientNode.address;

  conn->remoteNode.type = SPY_REMOTE_NODE_TYPE;
  conn->remoteNode.sock = remoteSocket;
  conn->remoteNode.server = server;
  conn->remoteNode.conn = conn;
  conn->remoteNode.partner = &conn->clientNode;
  conn->remoteNode.state = ret == 0 ? SOCK_OPEN_STATE : SOCK_INIT_STATE;
  NQEndPoint_sprintf(&remoteEndpoint, conn->remoteNode.address, sizeof(conn->remoteNode.address));
  NQNetBufferQueue_init(&conn->remoteNode.bufferQueue);
  conn->remoteNode.client.address = conn->remoteNode.address;

  NQSpyConnectionList_unshift(&server->connActiveList, conn);

  if (conn->clientNode.state == SOCK_OPEN_STATE)
    NQSpyModuleList_doClientConnect(&server->modules, &conn->clientNode.client);
  if (conn->remoteNode.state == SOCK_OPEN_STATE)
    NQSpyModuleList_doRemoteConnect(&server->modules, &conn->remoteNode.client);
}

static void NQSpyServer_onSocketError(NQSpyServer* server, NQSocketHandle sock)
{
  NQ_ASSERT(server->sock == sock);

  server->stopLoop = true;
}

typedef void (NQSpyPollCallback) (void*, NQSocketHandle);
struct NQSpyPoll {
  void* data;
  NQSpyPollCallback* onRecv;
  NQSpyPollCallback* onSend;
  NQSpyPollCallback* onClose;
  NQSpyPollCallback* onError;
};

static bool SpyPoll_init(NQSpyNode* node, struct NQSpyPoll* sp, NQPlatformPollfd* pf)
{
  if (node->state != SOCK_OPEN_STATE && node->state != SOCK_INIT_STATE)
    return false;

  if (node->state == SOCK_INIT_STATE)
    pf->events = NQ_POLLOUT;
  else if (node->state == SOCK_OPEN_STATE) {
    pf->events = NQ_POLLIN;
    if (!NQNetBufferQueue_isEmpty(&node->bufferQueue))
      pf->events |= NQ_POLLOUT;
  }

  pf->fd = node->sock;
  pf->revents = 0;

  sp->data = node;
  sp->onRecv = (NQSpyPollCallback*)NQSpyNode_onSocketRecv;
  sp->onSend = (NQSpyPollCallback*)NQSpyNode_onSocketSend;
  sp->onClose = (NQSpyPollCallback*)NQSpyNode_onSocketClose;
  sp->onError = (NQSpyPollCallback*)NQSpyNode_onSocketError;

  return true;
}

static int NQSpyServer_runImpl(NQSpyServer* server)
{
  int ret;
  size_t i, n;

  NQPlatformPollfd spoll[MAX_POLL];
  struct NQSpyPoll spoll2[MAX_POLL];

  NQSpyConnection* conn;
  NQSpyConnection* next;

  ret = NQSocketOpen(server->clientEndpoint.family, NQ_SOCK_STREAM, 0, &server->sock);
  if (ret < 0)
    return ret;

  if (NQSocketSetNonBlocking(server->sock, true) != 0)
    return -1;

  if (NQSocketSetOptBool(server->sock, NQ_SOL_SOCKET, NQ_SO_REUSEADDR, true) != 0)
    return -1;

  ret = NQSocketBind(server->sock, &server->clientEndpoint);
  if (ret < 0)
    return ret;

  if (NQSocketListen(server->sock, MAX_CONN) < 0)
    return -1;

  while (!server->stopLoop) {
    n = 0;
    spoll[n].fd = server->sock;
    spoll[n].events = NQ_POLLIN;
    spoll[n].revents = 0;
    spoll2[n].data = server;
    spoll2[n].onRecv = (NQSpyPollCallback*)NQSpyServer_onSocketAccept;
    spoll2[n].onSend = NULL;
    spoll2[n].onClose = (NQSpyPollCallback*)NQSpyServer_onSocketError;
    spoll2[n].onError = (NQSpyPollCallback*)NQSpyServer_onSocketError;
    n++;

    conn = server->connActiveList.first;
    while (conn != NULL) {
      next = conn->next;

      if (conn->clientNode.state == SOCK_CLOSE_STATE && conn->remoteNode.state == SOCK_CLOSE_STATE) {
        NQSpyConnectionList_remove(&server->connActiveList, conn);
        NQSpyConnectionList_unshift(&server->connFreeList, conn);
      }
      else {
        if (SpyPoll_init(&conn->clientNode, &spoll2[n], &spoll[n]))
          n++;
        if (SpyPoll_init(&conn->remoteNode, &spoll2[n], &spoll[n]))
          n++;
      }

      conn = next;
    }

    ret = NQPlatformPoll(spoll, n, 500);
    if (ret > 0) {
      for (i = 0; i < n; i++) {
        NQPlatformPollfd* pf = &spoll[i];
        if (pf->revents == 0)
          continue;
        struct NQSpyPoll* sp = &spoll2[i];

        if (sp->onError && pf->revents & NQ_POLLERR)
          sp->onError(sp->data, pf->fd);

        if (sp->onClose && pf->revents & NQ_POLLHUP)
          sp->onClose(sp->data, pf->fd);

        if (sp->onSend && pf->revents & NQ_POLLOUT)
          sp->onSend(sp->data, pf->fd);

        if (sp->onRecv && pf->revents & NQ_POLLIN)
          sp->onRecv(sp->data, pf->fd);
      }

    }
    else if (ret == 0) {
    }
    else {
      server->stopLoop = true;
    }
  }

  NQSocketClose(server->sock);
  return -1;
}

int NQSpyServer_run(NQSpyServer* server)
{
  int ret;
  NQSpyModuleList_doInit(&server->modules);
  ret = NQSpyServer_runImpl(server);
  NQSpyModuleList_doRelease(&server->modules);
  return ret;
}
