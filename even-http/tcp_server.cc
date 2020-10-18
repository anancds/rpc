//
// Created by cds on 2020/10/15.
//
#include "tcp_server.h"
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <csignal>
#include <sys/socket.h>
namespace mindspore {
namespace ps {
namespace comm {

void TcpConnection::setup(evutil_socket_t fd, struct bufferevent *bev, TcpServer *srv) {
  mBufferEvent = bev;
  mFd = fd;
  mServer = srv;

  auto *ms = dynamic_cast<TcpServer *>(srv);
  if (ms) {
    tcp_message_handler_.SetCallback([this, ms](const void *buf, size_t num) {
      if (ms->mMessageCb) ms->mMessageCb(*ms, *this, buf, num);
    });
  }
}

void TcpConnection::on_read_handler(const void *buffer, size_t num) {
  tcp_message_handler_.ReceiveMessage(buffer, num);
}

void TcpConnection::send_msg(const void *buffer, size_t num) {
  if (bufferevent_write(mBufferEvent, buffer, num) == -1) {
  }
}

TcpServer::TcpServer() : base(nullptr), signal_event(nullptr), listener(nullptr) {}

TcpServer::~TcpServer() {
  if (signal_event != nullptr) {
    event_free(signal_event);
    signal_event = nullptr;
  }

  if (listener != nullptr) {
    evconnlistener_free(listener);
    listener = nullptr;
  }

  if (base != nullptr) {
    event_base_free(base);
    base = nullptr;
  }
}

void TcpServer::SetServerCallback(OnConnected client_conn, OnDisconnected client_disconn, OnAccepted client_accept) {
  this->client_conn = client_conn;
  this->client_disconn = client_disconn;
  this->client_accept = client_accept;
}

void TcpServer::InitServer(const unsigned short &port) {
  base = event_base_new();
  MS_EXCEPTION_IF_NULL(base);

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);

  listener = evconnlistener_new_bind(base, ListenerCallback, reinterpret_cast<void *>(this),
                                     LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
                                     reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin));

  MS_EXCEPTION_IF_NULL(listener);

  signal_event = evsignal_new(base, SIGINT, SignalCallback, reinterpret_cast<void *>(this));
  MS_EXCEPTION_IF_NULL(signal_event);
  if (event_add(signal_event, nullptr) < 0) {
    MS_LOG(EXCEPTION) << "Cannot create signal event.";
  }
}

void TcpServer::Start() {
  std::unique_lock<std::recursive_mutex> l(mConnectionsMutex);
  MS_EXCEPTION_IF_NULL(base);
  int ret = event_base_dispatch(base);
}
void TcpServer::SendToAllClients(const char *data, size_t len) {
  std::unique_lock<std::recursive_mutex> l(mConnectionsMutex);

  auto it = connections.begin();
  while (it != connections.end()) {
    it->second->send_msg(data, len);
    ++it;
  }
}

void TcpServer::AddConnection(evutil_socket_t fd, TcpConnection *connection) {
  std::unique_lock<std::recursive_mutex> l(mConnectionsMutex);
  connections.insert(std::pair<evutil_socket_t, class TcpConnection *>(fd, connection));
}

void TcpServer::RemoveConnection(evutil_socket_t fd) {
  std::unique_lock<std::recursive_mutex> l(mConnectionsMutex);
  connections.erase(fd);
}

void TcpServer::ListenerCallback(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *saddr,
                                 int socklen, void *data) {
  auto *server = reinterpret_cast<class TcpServer *>(data);
  auto *base = reinterpret_cast<struct event_base *>(server->base);
  struct bufferevent *bev;

  bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  if (!bev) {
    event_base_loopbreak(base);
    printf("Error constructing bufferevent!\n");
    return;
  }

  TcpConnection *conn = server->on_create_conn();
  if (!conn) {
    printf("Error creation of connection object.");
    return;
  }

  conn->setup(fd, bev, server);

  server->AddConnection(fd, conn);

  bufferevent_setcb(bev, TcpServer::ReadCallback, TcpServer::WriteCallback, TcpServer::EventCallback,
                    reinterpret_cast<void *>(conn));
  bufferevent_enable(bev, EV_WRITE);
  bufferevent_enable(bev, EV_READ);
}

TcpConnection *TcpServer::on_create_conn() {
  TcpConnection *conn;
  if (client_accept)
    conn = client_accept(this);
  else
    conn = new TcpConnection();

  return conn;
}

void TcpServer::SignalCallback(evutil_socket_t sig, short events, void *data) {
  auto *server = reinterpret_cast<class TcpServer *>(data);
  struct event_base *base = server->base;
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
    conn->on_read_handler(readbuf, static_cast<size_t>(read));
  }
}

void TcpServer::EventCallback(struct bufferevent *bev, short events, void *data) {
  auto *conn = reinterpret_cast<TcpConnection *>(data);
  TcpServer *srv = conn->mServer;

  if (events & BEV_EVENT_EOF) {
    // Notify about disconnection
    if (srv->client_disconn) srv->client_disconn(conn->mServer, conn);

    // Free connection structures
    conn->mServer->RemoveConnection(conn->mFd);
    bufferevent_free(bev);

  } else if (events & BEV_EVENT_ERROR) {
    // Free connection structures
    conn->mServer->RemoveConnection(conn->mFd);
    bufferevent_free(bev);

    // Notify about disconnection
    if (srv->client_disconn) srv->client_disconn(conn->mServer, conn);

  } else {
    printf("unhandled.\n");
  }
}
void TcpServer::SetMessageCallback(OnServerReceiveMessage cb) { mMessageCb = cb; }
void TcpServer::SendMessage(TcpConnection &conn, const void *data, size_t num) {
  auto &mc = dynamic_cast<TcpConnection &>(conn);
  mc.send_msg(data, num);
}

void TcpServer::SendMessage(const void *data, size_t num) {
  std::unique_lock<std::recursive_mutex> l(mConnectionsMutex);

  auto it = connections.begin();
  while (it != connections.end()) {
    SendMessage(*it->second, data, num);
    ++it;
  }
}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
