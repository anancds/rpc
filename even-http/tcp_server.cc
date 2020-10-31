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

#include "tcp_server.h"

#include <arpa/inet.h>
#include <event2/buffer.h>
#include <event2/buffer_compat.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <sys/socket.h>
#include <csignal>
#include <utility>

#include "comm_util.h"
#include "securec.h"

namespace mindspore {
namespace ps {
namespace comm {

void TcpConnection::InitConnection(const evutil_socket_t &fd, const struct bufferevent *bev, const TcpServer *server) {
  MS_EXCEPTION_IF_NULL(bev);
  MS_EXCEPTION_IF_NULL(server);
  buffer_event_ = const_cast<struct bufferevent *>(bev);
  fd_ = fd;
  server_ = const_cast<TcpServer *>(server);
}

void TcpConnection::OnReadHandler(const void *buffer, size_t num) {
  OnServerReceive on_server_receive = server_->GetServerReceive();
  if (on_server_receive) on_server_receive(*server_, *this, buffer, num);
}

void TcpConnection::SendMessage(const void *buffer, size_t num) const {
  if (bufferevent_write(buffer_event_, buffer, num) == -1) {
    MS_LOG(ERROR) << "Write message to buffer event failed!";
  }
}

TcpServer *TcpConnection::GetServer() const { return server_; }

evutil_socket_t TcpConnection::GetFd() const { return fd_; }

void TcpMessageConnection::InitConnection(const evutil_socket_t &fd, const struct bufferevent *bev,
                                          const TcpServer *server) {
  TcpConnection::InitConnection(fd, bev, server);

  auto tcp_server = dynamic_cast<TcpMessageServer *>(const_cast<TcpServer *>(server));
  MS_EXCEPTION_IF_NULL(tcp_server);
  tcp_message_handler_.SetCallback([this, tcp_server](const void *buf, size_t num) {
    if (tcp_server->message_callback_) tcp_server->message_callback_(*tcp_server, *this, buf, num);
  });
}

void TcpMessageConnection::OnReadHandler(const void *buffer, size_t num) {
  tcp_message_handler_.ReceiveMessage(buffer, num);
}

void TcpMessageConnection::SendMessage(const void *buffer, size_t num) const {
  MessageHeader message_header;
  message_header.message_magic_ = htonl(MAGIC);
  message_header.message_length_ = htonl(static_cast<uint32_t>(num));
  if (bufferevent_write(buffer_event_, &message_header, sizeof(message_header)) == -1) {
    MS_LOG(ERROR) << "Write message to buffer event failed!";
  }
  if (bufferevent_write(buffer_event_, buffer, num) == -1) {
    MS_LOG(ERROR) << "Write message to buffer event failed!";
  }
}

void TcpKVConnection::InitConnection(const evutil_socket_t &fd, const struct bufferevent *bev,
                                     const TcpServer *server) {
  TcpConnection::InitConnection(fd, bev, server);

  auto tcp_server = dynamic_cast<TcpKVServer *>(const_cast<TcpServer *>(server));
  MS_EXCEPTION_IF_NULL(tcp_server);
  tcp_message_handler_.SetKVCallback([this, tcp_server](const PBMessage &message) {
    if (tcp_server->message_kv_callback_) tcp_server->message_kv_callback_(*tcp_server, *this, message);
  });
}

void TcpKVConnection::OnReadHandler(const void *buffer, size_t num) {
  tcp_message_handler_.ReceiveKVMessage(buffer, num);
}

void TcpKVConnection::SendKVMessage(const PBMessage &message) const {
  MS_EXCEPTION_IF_NULL(buffer_event_);
  size_t buf_size = message.ByteSizeLong();
  std::unique_ptr<char[]> serialized(new char[buf_size]);
  message.SerializeToArray(&serialized[0], static_cast<int>(buf_size));
  MessageHeader message_header;
  message_header.message_magic_ = htonl(MAGIC);
  message_header.message_length_ = htonl(static_cast<uint32_t>(buf_size));
  if (evbuffer_add(bufferevent_get_output(buffer_event_), &message_header, sizeof(message_header)) == -1) {
    MS_LOG(EXCEPTION) << "Event buffer add header failed!";
  }
  if (evbuffer_add(bufferevent_get_output(buffer_event_), serialized.get(), buf_size) == -1) {
    MS_LOG(EXCEPTION) << "Event buffer add protobuf data failed!";
  }
}

TcpServer::TcpServer(std::string address, std::uint16_t port)
    : base_(nullptr),
      signal_event_(nullptr),
      listener_(nullptr),
      server_address_(std::move(address)),
      server_port_(port) {}

TcpServer::~TcpServer() { Stop(); }

void TcpServer::SetServerCallback(const OnConnected &client_conn, const OnDisconnected &client_disconn,
                                  const OnAccepted &client_accept) {
  this->client_connection_ = client_conn;
  this->client_disconnection_ = client_disconn;
  this->client_accept_ = client_accept;
}

void TcpServer::InitServer() {
  base_ = event_base_new();
  MS_EXCEPTION_IF_NULL(base_);
  CommUtil::CheckIp(server_address_);

  struct sockaddr_in sin {};
  memset(&sin, 0, sizeof(sin));
  //  if (memset_s(&sin, sizeof(sin), 0, sizeof(sin)) != EOK) {
  //    MS_LOG(EXCEPTION) << "Initialize sockaddr_in failed!";
  //  }
  sin.sin_family = AF_INET;
  sin.sin_port = htons(server_port_);
  sin.sin_addr.s_addr = inet_addr(server_address_.c_str());

  listener_ = evconnlistener_new_bind(base_, ListenerCallback, reinterpret_cast<void *>(this),
                                      LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
                                      reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin));

  MS_EXCEPTION_IF_NULL(listener_);

  signal_event_ = evsignal_new(base_, SIGINT, SignalCallback, reinterpret_cast<void *>(this));
  MS_EXCEPTION_IF_NULL(signal_event_);
  if (event_add(signal_event_, nullptr) < 0) {
    MS_LOG(EXCEPTION) << "Cannot create signal event.";
  }
}

void TcpServer::Start() {
  std::unique_lock<std::recursive_mutex> l(connection_mutex_);
  MS_LOG(INFO) << "Start tcp server!";
  MS_EXCEPTION_IF_NULL(base_);
  int ret = event_base_dispatch(base_);
  if (ret == 0) {
    MS_LOG(INFO) << "Event base dispatch success!";
  } else if (ret == 1) {
    MS_LOG(ERROR) << "Event base dispatch failed with no events pending or active!";
  } else if (ret == -1) {
    MS_LOG(ERROR) << "Event base dispatch failed with error occurred!";
  } else {
    MS_LOG(EXCEPTION) << "Event base dispatch with unexpect error code!";
  }
}

void TcpServer::Stop() {
  MS_LOG(INFO) << "Stop tcp server!";
  if (signal_event_ != nullptr) {
    event_free(signal_event_);
    signal_event_ = nullptr;
  }

  if (listener_ != nullptr) {
    evconnlistener_free(listener_);
    listener_ = nullptr;
  }

  if (base_ != nullptr) {
    event_base_free(base_);
    base_ = nullptr;
  }
}

void TcpServer::SendToAllClients(const char *data, size_t len) {
  MS_EXCEPTION_IF_NULL(data);
  std::unique_lock<std::recursive_mutex> lock(connection_mutex_);
  for (auto it = connections_.begin(); it != connections_.end(); ++it) {
    it->second->SendMessage(data, len);
  }
}

void TcpServer::AddConnection(const evutil_socket_t &fd, const TcpConnection *connection) {
  MS_EXCEPTION_IF_NULL(connection);
  std::unique_lock<std::recursive_mutex> lock(connection_mutex_);
  connections_.insert(std::make_pair(fd, connection));
}

void TcpServer::RemoveConnection(const evutil_socket_t &fd) {
  std::unique_lock<std::recursive_mutex> lock(connection_mutex_);
  connections_.erase(fd);
}

void TcpServer::ListenerCallback(struct evconnlistener *, evutil_socket_t fd, struct sockaddr *, int, void *data) {
  auto server = reinterpret_cast<class TcpServer *>(data);
  auto base = reinterpret_cast<struct event_base *>(server->base_);
  MS_EXCEPTION_IF_NULL(server);
  MS_EXCEPTION_IF_NULL(base);

  struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  if (!bev) {
    MS_LOG(ERROR) << "Error constructing buffer event!";
    event_base_loopbreak(base);
    return;
  }

  TcpConnection *conn = server->onCreateConnection();
  MS_EXCEPTION_IF_NULL(conn);

  conn->InitConnection(fd, bev, server);
  server->AddConnection(fd, conn);
  bufferevent_setcb(bev, TcpServer::ReadCallback, nullptr, TcpServer::EventCallback, reinterpret_cast<void *>(conn));
  if (bufferevent_enable(bev, EV_READ | EV_WRITE) == -1) {
    MS_LOG(EXCEPTION) << "Buffer event enable read and write failed!";
  }
}

TcpConnection *TcpServer::onCreateConnection() {
  TcpConnection *conn = nullptr;
  if (client_accept_)
    conn = const_cast<TcpConnection *>(client_accept_(this));
  else
    conn = new TcpConnection();

  return conn;
}

OnServerReceive TcpServer::GetServerReceive() const { return message_callback_; }

void TcpServer::SignalCallback(evutil_socket_t, std::int16_t, void *data) {
  auto server = reinterpret_cast<class TcpServer *>(data);
  MS_EXCEPTION_IF_NULL(server);
  struct event_base *base = server->base_;
  struct timeval delay = {0, 0};
  MS_LOG(ERROR) << "Caught an interrupt signal; exiting cleanly in 0 seconds.";
  if (event_base_loopexit(base, &delay) == -1) {
    MS_LOG(EXCEPTION) << "Event base loop exit failed.";
  }
}

void TcpServer::ReadCallback(struct bufferevent *bev, void *connection) {
  MS_EXCEPTION_IF_NULL(bev);
  MS_EXCEPTION_IF_NULL(connection);

  auto conn = static_cast<class TcpConnection *>(connection);
  struct evbuffer *buf = bufferevent_get_input(bev);
  char read_buffer[4096];
  while (EVBUFFER_LENGTH(buf) > 0) {
    int read = evbuffer_remove(buf, &read_buffer, sizeof(read_buffer));
    if (read == -1) {
      MS_LOG(EXCEPTION) << "Can not drain data from the event buffer!";
    }
    conn->OnReadHandler(read_buffer, static_cast<size_t>(read));
  }
}

void TcpServer::EventCallback(struct bufferevent *bev, std::int16_t events, void *data) {
  MS_EXCEPTION_IF_NULL(bev);
  MS_EXCEPTION_IF_NULL(data);
  auto conn = reinterpret_cast<TcpConnection *>(data);
  TcpServer *srv = conn->GetServer();

  if (events & BEV_EVENT_EOF) {
    // Notify about disconnection
    if (srv->client_disconnection_) srv->client_disconnection_(srv, conn);
    // Free connection structures
    srv->RemoveConnection(conn->GetFd());
    bufferevent_free(bev);
  } else if (events & BEV_EVENT_ERROR) {
    // Free connection structures
    srv->RemoveConnection(conn->GetFd());
    bufferevent_free(bev);

    // Notify about disconnection
    if (srv->client_disconnection_) srv->client_disconnection_(srv, conn);
  } else {
    MS_LOG(ERROR) << "Unhandled event!";
  }
}

void TcpMessageServer::SendMessage(const TcpConnection &conn, const void *data, size_t num) {
  MS_EXCEPTION_IF_NULL(data);
  auto mc = dynamic_cast<const TcpMessageConnection &>(conn);
  mc.SendMessage(data, num);
}

void TcpMessageServer::SendMessage(const void *data, size_t num) {
  MS_EXCEPTION_IF_NULL(data);
  std::unique_lock<std::recursive_mutex> lock(connection_mutex_);

  for (auto it = connections_.begin(); it != connections_.end(); ++it) {
    SendMessage(*it->second, data, num);
  }
}
void TcpMessageServer::ReceiveMessage(const OnServerReceiveMessage &cb) { message_callback_ = cb; }

TcpConnection *TcpMessageServer::onCreateConnection() {
  const TcpConnection *conn;
  if (client_accept_)
    conn = client_accept_(this);
  else
    conn = new TcpMessageConnection();

  return const_cast<TcpConnection *>(conn);
}

void TcpKVServer::SendKVMessage(const TcpConnection &conn, const PBMessage &message) {
  auto mc = dynamic_cast<const TcpKVConnection &>(conn);
  mc.SendKVMessage(message);
}

void TcpKVServer::SendKVMessage(const PBMessage &message) {
  std::unique_lock<std::recursive_mutex> lock(connection_mutex_);

  for (auto it = connections_.begin(); it != connections_.end(); ++it) {
    SendKVMessage(*it->second, message);
  }
}

void TcpKVServer::ReceiveKVMessage(const OnServerReceiveKVMessage &cb) { message_kv_callback_ = cb; }

TcpConnection *TcpKVServer::onCreateConnection() {
  const TcpConnection *conn;
  if (client_accept_)
    conn = client_accept_(this);
  else
    conn = new TcpKVConnection();

  return const_cast<TcpConnection *>(conn);
}

}  // namespace comm
}  // namespace ps
}  // namespace mindspore
