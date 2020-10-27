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
#include "log_adapter.h"
#include "message.h"
#include "tcp_message_handler.h"

namespace mindspore {
namespace ps {
namespace comm {

class TcpServer;
class TcpConnection {
  friend class TcpServer;

 public:
  explicit TcpConnection() : buffer_event_(nullptr), fd_(0), server_(nullptr) {}
  virtual ~TcpConnection() = default;

  virtual void InitConnection(evutil_socket_t fd, struct bufferevent *bev, TcpServer *server);
  void SendMessage(const void *buffer, size_t num);
  virtual void OnReadHandler(const void *buffer, size_t numBytes);

 protected:
  TcpMessageHandler tcp_message_handler_;
  struct bufferevent *buffer_event_;
  evutil_socket_t fd_;
  TcpServer *server_;
};

using OnServerReceiveMessage =
  std::function<void( TcpServer &tcp_server, const TcpConnection &conn, const void *buffer, size_t num)>;

using OnServerReceiveKVMessage =
  std::function<void(const TcpServer &tcp_server, const TcpConnection &conn, const Message &message)>;
class TcpServer {
  friend class TcpConnection;

 public:
  using OnConnected = std::function<void(TcpServer *, TcpConnection *)>;
  using OnDisconnected = std::function<void(TcpServer *, TcpConnection *)>;
  using OnAccepted = std::function<TcpConnection *(TcpServer *)>;

  explicit TcpServer(std::string address, std::uint16_t port);
  virtual ~TcpServer();

  void SetServerCallback(OnConnected client_conn, OnDisconnected client_disconn, OnAccepted client_accept);
  void InitServer();
  void Start();
  void Stop();
  void SendToAllClients(const char *data, size_t len);
  void AddConnection(evutil_socket_t fd, TcpConnection *connection);
  void RemoveConnection(evutil_socket_t fd);
  void ReceiveMessage(OnServerReceiveMessage cb);
  void ReceiveKVMessage(const OnServerReceiveKVMessage &cb);
   void SendMessage(const TcpConnection &conn, const void *data, size_t num);
  void SendMessage(const void *data, size_t num);

 protected:
  static void ListenerCallback(struct evconnlistener *listener, evutil_socket_t socket, struct sockaddr *saddr,
                               int socklen, void *server);
  static void SignalCallback(evutil_socket_t sig, short events, void *server);
  static void WriteCallback(struct bufferevent *, void *server);
  static void ReadCallback(struct bufferevent *, void *connection);
  static void EventCallback(struct bufferevent *, short, void *server);
  virtual TcpConnection *onCreateConnection();

 private:
  struct event_base *base_;
  struct event *signal_event_;
  struct evconnlistener *listener_;
  std::string server_address_;
  std::uint16_t server_port_;

  std::map<evutil_socket_t, TcpConnection *> connections_;
  OnConnected client_connection_;
  OnDisconnected client_disconnection_;
  OnAccepted client_accept_;
  std::recursive_mutex connection_mutex_;
  OnServerReceiveMessage message_callback_;
  OnServerReceiveKVMessage kv_message_callback_;
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
#endif  // RPC_TCP_SERVER_H
