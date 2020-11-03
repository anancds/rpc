/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MINDSPORE_CCSRC_PS_COMM_TCP_SERVER_H_
#define MINDSPORE_CCSRC_PS_COMM_TCP_SERVER_H_

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
#include "tcp_message_handler.h"

namespace mindspore {
namespace ps {
namespace comm {

class TcpServer;
class TcpConnection {
 public:
  TcpConnection() : buffer_event_(nullptr), fd_(0), server_(nullptr) {}
  virtual ~TcpConnection() = default;

  virtual void InitConnection(const evutil_socket_t &fd, const struct bufferevent *bev, const TcpServer *server);
  virtual void SendMessage(const void *buffer, size_t num) const;
  virtual void OnReadHandler(const void *buffer, size_t numBytes);
  TcpServer *GetServer() const;
  evutil_socket_t GetFd() const;

 protected:
  struct bufferevent *buffer_event_;
  evutil_socket_t fd_;
  TcpServer *server_;
};

class TcpKVConnection : public TcpConnection {
 public:
  TcpKVConnection() = default;
  ~TcpKVConnection() override = default;

  void InitConnection(const evutil_socket_t &fd, const struct bufferevent *bev, const TcpServer *server) override;
  void SendKVMessage(const CommMessage &message) const;
  void OnReadHandler(const void *buffer, size_t numBytes) override;

 protected:
  TcpMessageHandler tcp_message_handler_;
};

class TcpMessageConnection : public TcpConnection {
 public:
  TcpMessageConnection() = default;
  ~TcpMessageConnection() override = default;

  void InitConnection(const evutil_socket_t &fd, const struct bufferevent *bev, const TcpServer *server) override;
  void SendMessage(const void *buffer, size_t num) const override;
  void OnReadHandler(const void *buffer, size_t numBytes) override;

 protected:
  TcpMessageHandler tcp_message_handler_;
};

using OnServerReceive =
  std::function<void(const TcpServer &tcp_server, const TcpConnection &conn, const void *buffer, size_t num)>;

class TcpServer {
 public:
  using OnConnected = std::function<void(const TcpServer *, const TcpConnection *)>;
  using OnDisconnected = std::function<void(const TcpServer *, const TcpConnection *)>;
  using OnAccepted = std::function<const TcpConnection *(const TcpServer *)>;

  explicit TcpServer(std::string address, std::uint16_t port);
  virtual ~TcpServer();

  void SetServerCallback(const OnConnected &client_conn, const OnDisconnected &client_disconn,
                         const OnAccepted &client_accept);
  void InitServer();
  void Start();
  void StartWithNoBlock();
  void Stop();
  void SendToAllClients(const char *data, size_t len);
  void AddConnection(const evutil_socket_t &fd, const TcpConnection *connection);
  void RemoveConnection(const evutil_socket_t &fd);
  OnServerReceive GetServerReceive() const;

 protected:
  static void ListenerCallback(struct evconnlistener *listener, evutil_socket_t socket, struct sockaddr *saddr,
                               int socklen, void *server);
  static void SignalCallback(evutil_socket_t sig, std::int16_t events, void *server);
  static void ReadCallback(struct bufferevent *, void *connection);
  static void EventCallback(struct bufferevent *, std::int16_t events, void *server);
  virtual TcpConnection *onCreateConnection();

  struct event_base *base_;
  struct event *signal_event_;
  struct evconnlistener *listener_;
  std::string server_address_;
  std::uint16_t server_port_;

  std::map<evutil_socket_t, std::unique_ptr<TcpConnection>> connections_;
  OnConnected client_connection_;
  OnDisconnected client_disconnection_;
  OnAccepted client_accept_;
  std::recursive_mutex connection_mutex_;
  OnServerReceive message_callback_;
};

class TcpMessageServer : public TcpServer {
 public:
  using OnServerReceiveMessage =
    std::function<void(TcpMessageServer &tcp_server, const TcpMessageConnection &conn, const void *buffer, size_t num)>;

  explicit TcpMessageServer(std::string address, std::uint16_t port) : TcpServer(address, port) {}
  ~TcpMessageServer() override = default;

  void ReceiveMessage(const OnServerReceiveMessage &cb);
  static void SendMessage(const TcpConnection &conn, const void *data, size_t num);
  void SendMessage(const void *data, size_t num);
  TcpConnection *onCreateConnection() override;
  OnServerReceiveMessage message_callback_;
};

class TcpKVServer : public TcpServer {
 public:
  using OnServerReceiveKVMessage =
    std::function<void(TcpKVServer &tcp_server, const TcpKVConnection &conn, const CommMessage &)>;

  explicit TcpKVServer(std::string address, std::uint16_t port) : TcpServer(address, port) {}
  ~TcpKVServer() override = default;

  void ReceiveKVMessage(const OnServerReceiveKVMessage &cb);
  static void SendKVMessage(const TcpConnection &conn, const CommMessage &message);
  void SendKVMessage(const CommMessage &message);
  TcpConnection *onCreateConnection() override;
  OnServerReceiveKVMessage message_kv_callback_;
};

}  // namespace comm
}  // namespace ps
}  // namespace mindspore
#endif  // MINDSPORE_CCSRC_PS_COMM_TCP_SERVER_H_
