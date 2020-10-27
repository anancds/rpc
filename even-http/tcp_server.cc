//
// Created by cds on 2020/10/15.
//
#include "tcp_server.h"
#include <arpa/inet.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <sys/socket.h>
#include <csignal>
#include <utility>
#include "comm_util.h"
namespace mindspore {
namespace ps {
namespace comm {

void TcpConnection::InitConnection(evutil_socket_t fd, struct bufferevent *bev, TcpServer *server) {
  MS_EXCEPTION_IF_NULL(bev);
  MS_EXCEPTION_IF_NULL(server);
  buffer_event_ = bev;
  fd_ = fd;
  server_ = server;

  auto *tcp_server = dynamic_cast<TcpServer *>(server);
  MS_EXCEPTION_IF_NULL(tcp_server);
  tcp_message_handler_.SetCallback([this, tcp_server](const void *buf, size_t num) {
    if (tcp_server->message_callback_) tcp_server->message_callback_(*tcp_server, *this, buf, num);
  });
}

void TcpConnection::OnReadHandler(const void *buffer, size_t num) { tcp_message_handler_.ReceiveMessage(buffer, num); }

void TcpConnection::SendMessage(const void *buffer, size_t num) {
  Message::MessageHeader message_header;
  message_header.mMagic = htonl(Message::MAGIC);
  message_header.mLength = htonl(static_cast<uint32_t>(num));
  if (bufferevent_write(buffer_event_, &message_header, sizeof(message_header)) == -1) {
    MS_LOG(ERROR) << "Write message to buffer event failed!";
  }
  if (bufferevent_write(buffer_event_, buffer, num) == -1) {
    MS_LOG(ERROR) << "Write message to buffer event failed!";
  }
}

TcpServer::TcpServer(std::string address, std::uint16_t port)
    : base_(nullptr),
      signal_event_(nullptr),
      listener_(nullptr),
      server_address_(std::move(address)),
      server_port_(port) {}

TcpServer::~TcpServer() { Stop(); }

void TcpServer::SetServerCallback(OnConnected client_conn, OnDisconnected client_disconn, OnAccepted client_accept) {
  this->client_connection_ = std::move(client_conn);
  this->client_disconnection_ = std::move(client_disconn);
  this->client_accept_ = std::move(client_accept);
}

void TcpServer::InitServer() {
  base_ = event_base_new();
  MS_EXCEPTION_IF_NULL(base_);
  CommUtil::CheckIpAndPort(server_address_, server_port_);

  struct sockaddr_in sin {};
  memset(&sin, 0, sizeof(sin));
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
  auto it = connections_.begin();
  while (it != connections_.end()) {
    it->second->SendMessage(data, len);
    ++it;
  }
}

void TcpServer::AddConnection(evutil_socket_t fd, TcpConnection *connection) {
  MS_EXCEPTION_IF_NULL(connection);
  std::unique_lock<std::recursive_mutex> lock(connection_mutex_);
  connections_.insert(std::pair<evutil_socket_t, class TcpConnection *>(fd, connection));
}

void TcpServer::RemoveConnection(evutil_socket_t fd) {
  std::unique_lock<std::recursive_mutex> lock(connection_mutex_);
  connections_.erase(fd);
}

void TcpServer::ListenerCallback(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *saddr,
                                 int socklen, void *data) {
  auto *server = reinterpret_cast<class TcpServer *>(data);
  auto *base = reinterpret_cast<struct event_base *>(server->base_);
  MS_EXCEPTION_IF_NULL(server);
  MS_EXCEPTION_IF_NULL(base);

  struct bufferevent *bev;
  bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  if (!bev) {
    MS_LOG(ERROR) << "Error constructing buffer event!";
    event_base_loopbreak(base);
    return;
  }

  TcpConnection *conn = server->onCreateConnection();
  MS_EXCEPTION_IF_NULL(conn);

  conn->InitConnection(fd, bev, server);
  server->AddConnection(fd, conn);
  bufferevent_setcb(bev, TcpServer::ReadCallback, TcpServer::WriteCallback, TcpServer::EventCallback,
                    reinterpret_cast<void *>(conn));
  bufferevent_enable(bev, EV_WRITE);
  bufferevent_enable(bev, EV_READ);
}

TcpConnection *TcpServer::onCreateConnection() {
  TcpConnection *conn;
  if (client_accept_)
    conn = client_accept_(this);
  else
    conn = new TcpConnection();

  return conn;
}

void TcpServer::SignalCallback(evutil_socket_t sig, short events, void *data) {
  auto *server = reinterpret_cast<class TcpServer *>(data);
  MS_EXCEPTION_IF_NULL(server);
  struct event_base *base = server->base_;
  struct timeval delay = {0, 0};
  MS_LOG(ERROR) << "Caught an interrupt signal; exiting cleanly in 0 seconds.";
  event_base_loopexit(base, &delay);
}

void TcpServer::WriteCallback(struct bufferevent *bev, void *data) {
  MS_EXCEPTION_IF_NULL(bev);
  MS_EXCEPTION_IF_NULL(data);
  struct evbuffer *output = bufferevent_get_output(bev);
  MS_EXCEPTION_IF_NULL(output);
  if (evbuffer_get_length(output) == 0) {
  }
}

void TcpServer::ReadCallback(struct bufferevent *bev, void *connection) {
  MS_EXCEPTION_IF_NULL(bev);
  MS_EXCEPTION_IF_NULL(connection);
  auto *conn = static_cast<class TcpConnection *>(connection);
  struct evbuffer *buf = bufferevent_get_input(bev);
  char read_buffer[1024];
  int read = 0;

  while ((read = evbuffer_remove(buf, &read_buffer, sizeof(read_buffer))) > 0) {
    conn->OnReadHandler(read_buffer, static_cast<size_t>(read));
  }
}

void TcpServer::EventCallback(struct bufferevent *bev, short events, void *data) {
  MS_EXCEPTION_IF_NULL(bev);
  MS_EXCEPTION_IF_NULL(data);
  auto *conn = reinterpret_cast<TcpConnection *>(data);
  TcpServer *srv = conn->server_;

  if (events & BEV_EVENT_EOF) {
    // Notify about disconnection
    if (srv->client_disconnection_) srv->client_disconnection_(conn->server_, conn);
    // Free connection structures
    conn->server_->RemoveConnection(conn->fd_);
    bufferevent_free(bev);
  } else if (events & BEV_EVENT_ERROR) {
    // Free connection structures
    conn->server_->RemoveConnection(conn->fd_);
    bufferevent_free(bev);

    // Notify about disconnection
    if (srv->client_disconnection_) srv->client_disconnection_(conn->server_, conn);
  } else {
    MS_LOG(ERROR) << "unhandled event!";
  }
}

void TcpServer::ReceiveMessage(OnServerReceiveMessage cb) { message_callback_ = cb; }

void TcpServer::ReceiveKVMessage(const OnServerReceiveKVMessage &cb) { kv_message_callback_ = cb; }

void TcpServer::SendMessage(const TcpConnection &conn, const void *data, size_t num) {
  MS_EXCEPTION_IF_NULL(data);
  auto &mc = const_cast<TcpConnection &>(conn);
  mc.SendMessage(data, num);
}

void TcpServer::SendMessage(const void *data, size_t num) {
  MS_EXCEPTION_IF_NULL(data);
  std::unique_lock<std::recursive_mutex> lock(connection_mutex_);

  auto it = connections_.begin();
  while (it != connections_.end()) {
    SendMessage(*it->second, data, num);
    ++it;
  }
}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
