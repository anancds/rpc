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

TcpClient::TcpClient() : mBase(nullptr), mTimeoutEvent(nullptr), mBufferEvent(nullptr) {
  tcpMessageHandler.SetCallback([this](const void *buf, size_t num) {
//    if (buf == nullptr) {
//      // Format error, disconnect
//      if (mDisconnectedCb) mDisconnectedCb(*this, 200);
//      stop();
//    }
    if (mMessageCb) mMessageCb(*this, buf, num);
  });
}

TcpClient::~TcpClient() { stop(); }

void TcpClient::set_target(const std::string &target) { mTarget = target; }

std::string TcpClient::get_target() const { return mTarget; }

void TcpClient::set_callbacks(on_connected conn, on_disconnected disconn, on_read read, on_timeout timeout) {
  mConnectedCb = conn;
  mDisconnectedCb = disconn;
  mReadCb = read;
  mTimeoutCb = timeout;
}

void TcpClient::InitTcpClient() {
  if (mBufferEvent) return;

  int retcode = 0;

  mBase = event_base_new();

  // Find IP address
  uint16_t port = static_cast<uint16_t>(DEFAULT_PORT);
  std::string::size_type p = mTarget.find(':');
  if (p != std::string::npos) port = static_cast<uint16_t>(std::atoi(mTarget.c_str() + p + 1));

  sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  if (evutil_inet_pton(AF_INET, mTarget.c_str(), &sin.sin_addr.s_addr) != 0) {
  }
  sin.sin_port = htons(port);

  // Prepare bufferevent
  mBufferEvent = bufferevent_socket_new(mBase, -1, BEV_OPT_CLOSE_ON_FREE);

  // No write callback for now
  bufferevent_setcb(mBufferEvent, readcb, nullptr, eventcb, this);
  bufferevent_enable(mBufferEvent, EV_READ | EV_WRITE);

  // evbuffer_add(bufferevent_get_output(bev), message, block_size);

  retcode = bufferevent_socket_connect(mBufferEvent, reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin));
  if (retcode < 0) {
  }
}

void TcpClient::start_with_delay(int seconds) {
  if (mBufferEvent) return;

  mBase = event_base_new();

  timeval timeout_value;
  timeout_value.tv_sec = seconds;
  timeout_value.tv_usec = 0;

  mTimeoutEvent = evtimer_new(mBase, timeoutcb, this);
  evtimer_add(mTimeoutEvent, &timeout_value);
}

void TcpClient::stop() {
  if (!mBase) return;

  if (mBufferEvent) {
    bufferevent_free(mBufferEvent);
    mBufferEvent = nullptr;
  }

  if (mTimeoutEvent) {
    event_free(mTimeoutEvent);
    mTimeoutEvent = nullptr;
  }

  if (mBase) {
    event_base_free(mBase);
    mBase = nullptr;
  }
}

/*
static void set_tcp_no_delay(evutil_socket_t fd)
{
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,   &one, sizeof one);
}
*/

void TcpClient::timeoutcb(evutil_socket_t fd, short what, void *arg) {
  // Time to start connecting
  TcpClient *c = reinterpret_cast<TcpClient *>(arg);
  try {
    c->InitTcpClient();
  } catch (const std::exception &e) {
  }
}

void TcpClient::readcb(struct bufferevent *bev, void *ctx) {
  TcpClient *c = reinterpret_cast<TcpClient *>(ctx);

  /* This callback is invoked when there is data to read on bev. */
  struct evbuffer *input = bufferevent_get_input(bev);
  // struct evbuffer *output = bufferevent_get_output(bev);

  char readbuf[1024];
  size_t read = 0;

  while ((read = static_cast<size_t>(evbuffer_remove(input, &readbuf, sizeof(readbuf)))) > 0) {
    c->on_read_handler(readbuf, read);
  }
}

void TcpClient::on_read_handler(const void *buf, size_t num) {
  if (mReadCb) mReadCb(*this, buf, num);
  tcpMessageHandler.ReceiveMessage(buf, num);
}

void TcpClient::eventcb(struct bufferevent *bev, short events, void *ptr) {
  TcpClient *c = reinterpret_cast<TcpClient *>(ptr);
  if (events & BEV_EVENT_CONNECTED) {
    // Connected
    if (c->mConnectedCb) c->mConnectedCb(*c);
    // evutil_socket_t fd = bufferevent_getfd(bev);
    // set_tcp_no_delay(fd);
  } else if (events & BEV_EVENT_ERROR) {
    // printf("NOT Connected\n");
    if (c->mDisconnectedCb) c->mDisconnectedCb(*c, errno);
  } else if (events & BEV_EVENT_EOF) {
    if (c->mDisconnectedCb) c->mDisconnectedCb(*c, 0);
  }
}

void TcpClient::update() {
  if (mBase) event_base_dispatch(mBase);
}

void TcpClient::set_message_callback(on_message cb) { mMessageCb = cb; }

void TcpClient::send_msg(const void *buf, size_t num) {
  if (mBufferEvent) {
    evbuffer_add(bufferevent_get_output(mBufferEvent), buf, num);
  }
}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
