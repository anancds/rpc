//
// Created by cds on 2020/10/15.
//

#ifndef RPC_TCP_SERVER_H
#define RPC_TCP_SERVER_H

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include "tcp_message_handler.h"
#include "log_adapter.h"
namespace mindspore {
namespace ps {
namespace comm {

class TcpServer;
class TcpConnection {
  friend class TcpServer;

 public:
  explicit TcpConnection() = default;
  virtual ~TcpConnection() = default;

  virtual void setup(evutil_socket_t fd, struct bufferevent *bev, TcpServer *srv);
  void send_msg(const void *buffer, size_t num);
  virtual void on_read_handler(const void *buffer, size_t numBytes);

 protected:
  TcpMessageHandler tcp_message_handler_;

  struct bufferevent *mBufferEvent;
  evutil_socket_t mFd;
  TcpServer *mServer;
};
class TcpServer {
  friend class TcpConnection;

 public:
  using OnServerReceiveMessage = std::function<void(TcpServer &, TcpConnection &conn, const void *buffer, size_t num)>;
  using OnConnected = std::function<void(TcpServer *, TcpConnection *)>;
  using OnDisconnected = std::function<void(TcpServer *, TcpConnection *)>;
  using OnAccepted = std::function<TcpConnection *(TcpServer *)>;

  explicit TcpServer();
  virtual ~TcpServer();

  void SetServerCallback(OnConnected client_conn, OnDisconnected client_disconn, OnAccepted client_accept);
  void InitServer(const unsigned short &port);
  void Start();
  void SendToAllClients(const char *data, size_t len);
  void AddConnection(evutil_socket_t fd, TcpConnection *connection);
  void RemoveConnection(evutil_socket_t fd);
  void SetMessageCallback(OnServerReceiveMessage cb);
  void SendMessage(TcpConnection &conn, const void *data, size_t num);
  void SendMessage(const void *data, size_t num);

 protected:
  static void ListenerCallback(struct evconnlistener *listener, evutil_socket_t socket, struct sockaddr *saddr,
                               int socklen, void *server);

  static void SignalCallback(evutil_socket_t sig, short events, void *server);
  static void WriteCallback(struct bufferevent *, void *server);
  static void ReadCallback(struct bufferevent *, void *connection);
  static void EventCallback(struct bufferevent *, short, void *server);

  struct sockaddr_in sin;
  struct event_base *base;
  struct event *signal_event;
  struct evconnlistener *listener;

  std::map<evutil_socket_t, TcpConnection *> connections;
  OnConnected client_conn;
  OnDisconnected client_disconn;
  OnAccepted client_accept;
  std::recursive_mutex mConnectionsMutex;
  OnServerReceiveMessage mMessageCb;
  virtual TcpConnection *on_create_conn();
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
#endif  // RPC_TCP_SERVER_H
