//
// Created by cds on 2020/10/16.
//

#include "tcp_client.h"

#include <arpa/inet.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <iostream>

namespace mindspore {
namespace ps {
namespace comm {

TcpClient::TcpClient() : event_base_(nullptr), event_timeout_(nullptr), buffer_event_(nullptr) {
  message_handler_.SetCallback([this](const void *buf, size_t num) {
    if (buf == nullptr) {
      // Format error, disconnect
      if (disconnected_callback_) disconnected_callback_(*this, 200);
      Stop();
    }
    if (message_callback_) message_callback_(*this, buf, num);
  });
}

TcpClient::~TcpClient() { Stop(); }

void TcpClient::SetTarget(const std::string &target) { target_ = target; }

std::string TcpClient::GetTarget() const { return target_; }

void TcpClient::SetCallback(on_connected conn, on_disconnected disconn, on_read read, on_timeout timeout) {
  connected_callback_ = conn;
  disconnected_callback_ = disconn;
  read_callback_ = read;
  timeout_callback_ = timeout;
}

void TcpClient::InitTcpClient() {
  if (buffer_event_) return;

  int retcode = 0;

  event_base_ = event_base_new();

  // Find IP address
  uint16_t port = static_cast<uint16_t>(DEFAULT_PORT);
  std::string::size_type p = target_.find(':');
  if (p != std::string::npos) port = static_cast<uint16_t>(std::atoi(target_.c_str() + p + 1));

  sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  if (evutil_inet_pton(AF_INET, target_.c_str(), &sin.sin_addr.s_addr) != 0) {
  }
  sin.sin_port = htons(port);

  // Prepare bufferevent
  buffer_event_ = bufferevent_socket_new(event_base_, -1, BEV_OPT_CLOSE_ON_FREE);

  // No write callback for now
  bufferevent_setcb(buffer_event_, ReadCallback, nullptr, EventCallback, this);
  bufferevent_enable(buffer_event_, EV_READ | EV_WRITE);

  // evbuffer_add(bufferevent_get_output(bev), message, block_size);

  retcode = bufferevent_socket_connect(buffer_event_, reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin));
  if (retcode < 0) {
  }
}

void TcpClient::StartWithDelay(int seconds) {
  if (buffer_event_) return;

  event_base_ = event_base_new();

  timeval timeout_value;
  timeout_value.tv_sec = seconds;
  timeout_value.tv_usec = 0;

  event_timeout_ = evtimer_new(event_base_, TimeoutCallback, this);
  evtimer_add(event_timeout_, &timeout_value);
}

void TcpClient::Stop() {
  if (!event_base_) return;

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

/*
static void set_tcp_no_delay(evutil_socket_t fd)
{
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,   &one, sizeof one);
}
*/

void TcpClient::TimeoutCallback(evutil_socket_t fd, short what, void *arg) {
  // Time to start connecting
  auto *c = reinterpret_cast<TcpClient *>(arg);
  try {
    c->InitTcpClient();
  } catch (const std::exception &e) {
  }
}

void TcpClient::ReadCallback(struct bufferevent *bev, void *ctx) {
  auto *c = reinterpret_cast<TcpClient *>(ctx);

  /* This callback is invoked when there is data to read on bev. */
  struct evbuffer *input = bufferevent_get_input(bev);
  // struct evbuffer *output = bufferevent_get_output(bev);

  char readbuf[1024];
  size_t read = 0;

  while ((read = static_cast<size_t>(evbuffer_remove(input, &readbuf, sizeof(readbuf)))) > 0) {
    c->OnReadHandler(readbuf, read);
  }
}

void TcpClient::OnReadHandler(const void *buf, size_t num) {
  if (read_callback_) read_callback_(*this, buf, num);
  message_handler_.ReceiveMessage(buf, num);
}

void TcpClient::EventCallback(struct bufferevent *bev, short events, void *ptr) {
  auto *c = reinterpret_cast<TcpClient *>(ptr);
  if (events & BEV_EVENT_CONNECTED) {
    // Connected
    if (c->connected_callback_) c->connected_callback_(*c);
    // evutil_socket_t fd = bufferevent_getfd(bev);
    // set_tcp_no_delay(fd);
  } else if (events & BEV_EVENT_ERROR) {
    // printf("NOT Connected\n");
    if (c->disconnected_callback_) c->disconnected_callback_(*c, errno);
  } else if (events & BEV_EVENT_EOF) {
    if (c->disconnected_callback_) c->disconnected_callback_(*c, 0);
  }
}

void TcpClient::Start() {
  if (event_base_) event_base_dispatch(event_base_);
}

void TcpClient::SetMessageCallback(on_message cb) { message_callback_ = cb; }

void TcpClient::SendMessage(const void *buf, size_t num) {
  if (buffer_event_) {
    evbuffer_add(bufferevent_get_output(buffer_event_), buf, num);
  }
}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
