//
// Created by cds on 2020/10/16.
//

#ifndef RPC_TCP_CLIENT_H
#define RPC_TCP_CLIENT_H
#include <event2/event.h>
#include "event2/bufferevent.h"
#include <functional>
#include "tcp_message_handler.h"

namespace mindspore {
namespace ps {
namespace comm {

class TcpClient {
  using on_message = std::function<void(TcpClient &, const void *, size_t)>;

 public:
  const int IO_TIMEOUT = 5;       // Seconds
  const int DEFAULT_PORT = 9000;  // Default port number for target

  typedef std::function<void(TcpClient &)> on_connected;
  typedef std::function<void(TcpClient &, int)> on_disconnected;
  typedef std::function<void(TcpClient &, const void *, size_t)> on_read;
  typedef std::function<void(TcpClient &)> on_timeout;

  TcpClient();
  virtual ~TcpClient();

  void set_target(const std::string &target);
  std::string get_target() const;

  void set_callbacks(on_connected conn, on_disconnected disconn, on_read read, on_timeout timeout);
  void InitTcpClient();
  void start_with_delay(int seconds);

  void stop();
  void set_message_callback(on_message cb);
  void send_msg(const void *buf, size_t num);

  void update();

 protected:
  static void timeoutcb(evutil_socket_t fd, short what, void *arg);
  static void readcb(struct bufferevent *bev, void *ctx);
  static void eventcb(struct bufferevent *bev, short events, void *ptr);
  TcpMessageHandler tcpMessageHandler;
  on_message mMessageCb;

  on_connected mConnectedCb;
  on_disconnected mDisconnectedCb;
  on_read mReadCb;
  on_timeout mTimeoutCb;

  event_base *mBase;
  event *mTimeoutEvent;
  bufferevent *mBufferEvent;

  std::string mTarget;

  virtual void on_read_handler(const void *buf, size_t num);
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
#endif  // RPC_TCP_CLIENT_H
