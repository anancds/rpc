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
  static void ReadCallback(struct bufferevent *bev, void *ctx);
  static void EventCallback(struct bufferevent *bev, short events, void *ptr);
  virtual void OnReadHandler(const void *buf, size_t num);

  TcpMessageHandler message_handler_;
  on_message message_callback_;
  on_connected connected_callback_;
  on_disconnected disconnected_callback_;
  on_read read_callback_;
  on_timeout timeout_callback_;

  event_base *event_base_;
  event *event_timeout_;
  bufferevent *buffer_event_;

  std::string target_;
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
#endif  // RPC_TCP_CLIENT_H
