//
// Created by cds on 2020/9/27.
//

#include <arpa/inet.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <cstring>
#include <iostream>
void echo_read_cb(struct bufferevent *bev, void *ctx) {
  struct evbuffer *input = bufferevent_get_input(bev);    // 输入缓存区
  struct evbuffer *output = bufferevent_get_output(bev);  // 输出缓存区
  // 将输入缓冲区的数据移动到输出缓冲区
  evbuffer_add_buffer(output, input);
}
void echo_event_cb(struct bufferevent *bev, short events, void *ctx) {
  if (events & BEV_EVENT_ERROR) {
    int err = EVUTIL_SOCKET_ERROR();
    std::cerr << "Got an error from bufferevent: " << evutil_socket_error_to_string(err) << std::endl;
  }
  if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
    bufferevent_free(bev);
  }
}
void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen,
                    void *arg) {
  // 设置 socket 为非阻塞
  evutil_make_socket_nonblocking(fd);
  auto *base = evconnlistener_get_base(listener);
  auto *b = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  bufferevent_setcb(b, echo_read_cb, nullptr, echo_event_cb, nullptr);
  bufferevent_enable(b, EV_READ | EV_WRITE);
}
int main() {
  short port = 8000;
  struct sockaddr_in sin {};
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(port);
  auto *base = event_base_new();
  auto *listener = evconnlistener_new_bind(base, accept_conn_cb, nullptr, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
                                           reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin));
  if (listener == nullptr) {
    std::cerr << "Couldn't create listener" << std::endl;
    return 1;
  }
  event_base_dispatch(base);
  return 0;
}