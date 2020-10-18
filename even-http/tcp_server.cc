//
// Created by cds on 2020/10/15.
//
#include "tcp_server.h"
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <sys/socket.h>
#include <csignal>
namespace mindspore {
namespace ps {
namespace comm {

void TcpConnection::InitConnection(evutil_socket_t fd, struct bufferevent *bev, TcpServer *srv) {
  buffer_event_ = bev;
  fd_ = fd;
  server_ = srv;

  auto *ms = dynamic_cast<TcpServer *>(srv);
  if (ms) {
    tcp_message_handler_.SetCallback([this, ms](const void *buf, size_t num) {
      if (ms->message_callback_) ms->message_callback_(*ms, *this, buf, num);
    });
  }
}

void TcpConnection::OnReadHandler(const void *buffer, size_t num) {
  tcp_message_handler_.ReceiveMessage(buffer, num);
}

void TcpConnection::SendMessage(const void *buffer, size_t num) {
  if (bufferevent_write(buffer_event_, buffer, num) == -1) {
  }
}

TcpServer::TcpServer() : base_(nullptr), signal_event_(nullptr), listener_(nullptr) {}

TcpServer::~TcpServer() {
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

void TcpServer::SetServerCallback(OnConnected client_conn, OnDisconnected client_disconn, OnAccepted client_accept) {
  this->client_connection_ = client_conn;
  this->client_disconnection_ = client_disconn;
  this->client_accept_ = client_accept;
}

void TcpServer::InitServer(const unsigned short &port) {
  base_ = event_base_new();
  MS_EXCEPTION_IF_NULL(base_);
  struct sockaddr_in sin {};
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);

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
}
void TcpServer::SendToAllClients(const char *data, size_t len) {
  std::unique_lock<std::recursive_mutex> l(connection_mutex_);

  auto it = connections_.begin();
  while (it != connections_.end()) {
    it->second->SendMessage(data, len);
    ++it;
  }
}

void TcpServer::AddConnection(evutil_socket_t fd, TcpConnection *connection) {
  std::unique_lock<std::recursive_mutex> l(connection_mutex_);
  connections_.insert(std::pair<evutil_socket_t, class TcpConnection *>(fd, connection));
}

void TcpServer::RemoveConnection(evutil_socket_t fd) {
  std::unique_lock<std::recursive_mutex> l(connection_mutex_);
  connections_.erase(fd);
}

void TcpServer::ListenerCallback(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *saddr,
                                 int socklen, void *data) {
  auto *server = reinterpret_cast<class TcpServer *>(data);
  auto *base = reinterpret_cast<struct event_base *>(server->base_);
  struct bufferevent *bev;

  bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  if (!bev) {
    event_base_loopbreak(base);
    printf("Error constructing bufferevent!\n");
    return;
  }

  TcpConnection *conn = server->onCreateConnection();
  if (!conn) {
    printf("Error creation of connection object.");
    return;
  }

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
  struct event_base *base = server->base_;
  struct timeval delay = {0, 0};
  MS_LOG(ERROR) << "Caught an interrupt signal; exiting cleanly in 0 seconds.";
  event_base_loopexit(base, &delay);
  // Exited
}

void TcpServer::WriteCallback(struct bufferevent *bev, void *data) {
  struct evbuffer *output = bufferevent_get_output(bev);
  if (evbuffer_get_length(output) == 0) {
  }
}

void TcpServer::ReadCallback(struct bufferevent *bev, void *connection) {
  auto *conn = static_cast<class TcpConnection *>(connection);
  struct evbuffer *buf = bufferevent_get_input(bev);
  char readbuf[1024];
  int read = 0;

  while ((read = evbuffer_remove(buf, &readbuf, sizeof(readbuf))) > 0) {
    conn->OnReadHandler(readbuf, static_cast<size_t>(read));
  }
}

void TcpServer::EventCallback(struct bufferevent *bev, short events, void *data) {
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
    printf("unhandled.\n");
  }
}
void TcpServer::SetMessageCallback(OnServerReceiveMessage cb) { message_callback_ = cb; }
void TcpServer::SendMessage(TcpConnection &conn, const void *data, size_t num) {
  auto &mc = dynamic_cast<TcpConnection &>(conn);
  mc.SendMessage(data, num);
}

void TcpServer::SendMessage(const void *data, size_t num) {
  std::unique_lock<std::recursive_mutex> l(connection_mutex_);

  auto it = connections_.begin();
  while (it != connections_.end()) {
    SendMessage(*it->second, data, num);
    ++it;
  }
}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
