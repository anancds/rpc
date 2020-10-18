//
// Created by cds on 2020/10/16.
//

#ifndef RPC_TCP_CLIENT_H
#define RPC_TCP_CLIENT_H
#include <event2/event.h>
#include <functional>
#include "event2/bufferevent.h"
#include "tcp_message_handler.h"

namespace mindspore {
namespace ps {
namespace comm {

class TcpClient {
 public:
  const int IO_TIMEOUT = 5;       // Seconds
  const int DEFAULT_PORT = 9000;  // Default port number for target

  using on_message = std::function<void(TcpClient &, const void *, size_t)>;
  using on_connected = std::function<void(TcpClient &)>;
  using on_disconnected = std::function<void(TcpClient &, int)>;
  using on_read = std::function<void(TcpClient &, const void *, size_t)>;
  using on_timeout = std::function<void(TcpClient &)>;

  TcpClient();
  virtual ~TcpClient();

  void SetTarget(const std::string &target);
  std::string GetTarget() const;
  void SetCallback(on_connected conn, on_disconnected disconn, on_read read, on_timeout timeout);
  void InitTcpClient();
  void StartWithDelay(int seconds);
  void Stop();
  void SetMessageCallback(on_message cb);
  void SendMessage(const void *buf, size_t num);
  void Start();

 protected:
  static void TimeoutCallback(evutil_socket_t fd, short what, void *arg);
  static void readcb(struct bufferevent *bev, void *ctx);
  static void eventcb(struct bufferevent *bev, short events, void *ptr);
  virtual void on_read_handler(const void *buf, size_t num);

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
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
#endif  // RPC_TCP_CLIENT_H
