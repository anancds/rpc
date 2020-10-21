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
  using OnMessage = std::function<void(const TcpClient &, const void *, size_t)>;
  using OnConnected = std::function<void(const TcpClient &)>;
  using OnDisconnected = std::function<void(const TcpClient &, int)>;
  using OnRead = std::function<void(const TcpClient &, const void *, size_t)>;
  using OnTimeout = std::function<void(const TcpClient &)>;

  explicit TcpClient(std::string address, std::uint16_t port);
  virtual ~TcpClient();

  [[nodiscard]] std::string GetServerAddress() const;
  void SetCallback(OnConnected conn, OnDisconnected disconn, OnRead read, OnTimeout timeout);
  void InitTcpClient();
  void StartWithDelay(int seconds);
  void Stop();
  void ReceiveMessage(OnMessage cb);
  void SendMessage(const void *buf, size_t num) const;
  void Start();

 protected:
  static void SetTcpNoDelay(evutil_socket_t fd);
  static void TimeoutCallback(evutil_socket_t fd, short what, void *arg);
  static void ReadCallback(struct bufferevent *bev, void *ctx);
  static void EventCallback(struct bufferevent *bev, short events, void *ptr);
  virtual void OnReadHandler(const void *buf, size_t num);

 private:
  TcpMessageHandler message_handler_;
  OnMessage message_callback_;
  OnConnected connected_callback_;
  OnDisconnected disconnected_callback_;
  OnRead read_callback_;
  OnTimeout timeout_callback_;

  event_base *event_base_;
  event *event_timeout_;
  bufferevent *buffer_event_;

  std::string server_address_;
  std::uint16_t server_port_;
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
#endif  // RPC_TCP_CLIENT_H
