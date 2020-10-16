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
  typedef std::function<void(TcpServer &, TcpConnection &conn, const void *buffer, size_t num)> on_message;

 public:
  // Callbacks
  typedef std::function<void(TcpServer *, TcpConnection *)> on_connected;
  typedef std::function<void(TcpServer *, TcpConnection *)> on_disconnected;
  typedef std::function<TcpConnection *(TcpServer *)> on_accepted;
  typedef std::function<void(TcpServer *, TcpConnection *, const void *buffer, size_t num)> on_received;

  TcpServer();
  virtual ~TcpServer();

  //        void set_callbacks(on_connected client_conn, on_disconnected client_disconn,
  //                           on_accepted client_accept, on_received client_recv);
  void setup(const unsigned short &port);
  void update();
  void sendToAllClients(const char *data, size_t len);
  void addConnection(evutil_socket_t fd, TcpConnection *connection);
  void removeConnection(evutil_socket_t fd);
  void set_msg_callback(on_message cb);
  void send_msg(TcpConnection &conn, const void *data, size_t num);
  void send_msg(const void *data, size_t num);

 protected:
  static void listenerCallback(struct evconnlistener *listener, evutil_socket_t socket, struct sockaddr *saddr,
                               int socklen, void *server);

  static void signalCallback(evutil_socket_t sig, short events, void *server);
  static void writeCallback(struct bufferevent *, void *server);
  static void readCallback(struct bufferevent *, void *connection);
  static void eventCallback(struct bufferevent *, short, void *server);

  struct sockaddr_in sin;
  struct event_base *base;
  struct event *signal_event;
  struct evconnlistener *listener;

  std::map<evutil_socket_t, TcpConnection *> connections;
  on_connected client_conn;
  on_disconnected client_disconn;
  on_accepted client_accept;
  on_received client_recv;
  std::recursive_mutex mConnectionsMutex;
  on_message mMessageCb;
  virtual TcpConnection *on_create_conn();
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
#endif  // RPC_TCP_SERVER_H
