//
// Created by cds on 2020/10/16.
//

#include "tcp_client.h"
#include "comm_util.h"

#include <arpa/inet.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <utility>

namespace mindspore {
namespace ps {
namespace comm {

TcpClient::TcpClient(std::string address, std::uint16_t port)
    : event_base_(nullptr),
      event_timeout_(nullptr),
      buffer_event_(nullptr),
      server_address_(std::move(address)),
      server_port_(port) {
  message_handler_.SetCallback([this](const void *buf, size_t num) {
    if (buf == nullptr) {
      if (disconnected_callback_) disconnected_callback_(*this, 200);
      Stop();
    }
    if (message_callback_) message_callback_(*this, buf, num);
  });
}

TcpClient::~TcpClient() { Stop(); }

std::string TcpClient::GetServerAddress() const { return server_address_; }

void TcpClient::SetCallback(OnConnected conn, OnDisconnected disconn, OnRead read, OnTimeout timeout) {
  connected_callback_ = std::move(conn);
  disconnected_callback_ = std::move(disconn);
  read_callback_ = std::move(read);
  timeout_callback_ = std::move(timeout);
}

void TcpClient::InitTcpClient() {
  if (buffer_event_) return;
  CommUtil::CheckIpAndPort(server_address_, server_port_);

  event_base_ = event_base_new();
  MS_EXCEPTION_IF_NULL(event_base_);

  sockaddr_in sin{};
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = inet_addr(server_address_.c_str());
  sin.sin_port = htons(server_port_);

  buffer_event_ = bufferevent_socket_new(event_base_, -1, BEV_OPT_CLOSE_ON_FREE);
  MS_EXCEPTION_IF_NULL(buffer_event_);

  bufferevent_setcb(buffer_event_, ReadCallback, nullptr, EventCallback, this);
  bufferevent_enable(buffer_event_, EV_READ | EV_WRITE);

  int result_code = bufferevent_socket_connect(buffer_event_, reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin));
  if (result_code < 0) {
    MS_LOG(EXCEPTION) << "Connect server ip:" << server_address_ << " and port: " << server_port_ << " is failed!";
  }
}

void TcpClient::StartWithDelay(int seconds) {
  if (buffer_event_) return;

  event_base_ = event_base_new();

  timeval timeout_value{};
  timeout_value.tv_sec = seconds;
  timeout_value.tv_usec = 0;

  event_timeout_ = evtimer_new(event_base_, TimeoutCallback, this);
  evtimer_add(event_timeout_, &timeout_value);
}

void TcpClient::Stop() {
  if (!event_base_) {
    return;
  }

  if (buffer_event_) {
    bufferevent_free(buffer_event_);
    buffer_event_ = nullptr;
  }

  if (event_timeout_) {
    event_free(event_timeout_);
    event_timeout_ = nullptr;
  }

  if (event_base_) {
    event_base_free(event_base_);
    event_base_ = nullptr;
  }
}

void TcpClient::SetTcpNoDelay(evutil_socket_t fd) {
  int one = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
}

void TcpClient::TimeoutCallback(evutil_socket_t fd, short what, void *arg) {
  MS_EXCEPTION_IF_NULL(arg);
  auto *tcp_client = reinterpret_cast<TcpClient *>(arg);
  tcp_client->InitTcpClient();
}

void TcpClient::ReadCallback(struct bufferevent *bev, void *ctx) {
  MS_EXCEPTION_IF_NULL(bev);
  MS_EXCEPTION_IF_NULL(ctx);
  auto *tcp_client = reinterpret_cast<TcpClient *>(ctx);
  struct evbuffer *input = bufferevent_get_input(bev);
  MS_EXCEPTION_IF_NULL(input);

  char read_buffer[1024];
  size_t read = 0;

  while ((read = static_cast<size_t>(evbuffer_remove(input, &read_buffer, sizeof(read_buffer)))) > 0) {
    tcp_client->OnReadHandler(read_buffer, read);
  }
}

void TcpClient::OnReadHandler(const void *buf, size_t num) {
  MS_EXCEPTION_IF_NULL(buf);
  if (read_callback_) read_callback_(*this, buf, num);
  message_handler_.ReceiveMessage(buf, num);
}

void TcpClient::EventCallback(struct bufferevent *bev, short events, void *ptr) {
  MS_EXCEPTION_IF_NULL(bev);
  MS_EXCEPTION_IF_NULL(ptr);
  auto *tcp_client = reinterpret_cast<TcpClient *>(ptr);
  if (events & BEV_EVENT_CONNECTED) {
    // Connected
    if (tcp_client->connected_callback_) tcp_client->connected_callback_(*tcp_client);
    evutil_socket_t fd = bufferevent_getfd(bev);
    SetTcpNoDelay(fd);
    MS_LOG(INFO) << "Client connected!";
  } else if (events & BEV_EVENT_ERROR) {
    MS_LOG(ERROR) << "Client connected error!";
    if (tcp_client->disconnected_callback_) tcp_client->disconnected_callback_(*tcp_client, errno);
  } else if (events & BEV_EVENT_EOF) {
    MS_LOG(ERROR) << "Client connected end of file";
    if (tcp_client->disconnected_callback_) tcp_client->disconnected_callback_(*tcp_client, 0);
  }
}

void TcpClient::Start() {
  MS_EXCEPTION_IF_NULL(event_base_);
  event_base_dispatch(event_base_);
}

void TcpClient::ReceiveMessage(OnMessage cb) { message_callback_ = std::move(cb); }

void TcpClient::SendMessage(const void *buf, size_t num) const{
  MS_EXCEPTION_IF_NULL(buffer_event_);
  evbuffer_add(bufferevent_get_output(buffer_event_), buf, num);
}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
