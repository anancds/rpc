/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MINDSPORE_CCSRC_PS_COMM_TCP_CLIENT_H_
#define MINDSPORE_CCSRC_PS_COMM_TCP_CLIENT_H_

#include "tcp_message_handler.h"

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <functional>
#include <string>

#include "message.h"

namespace mindspore {
namespace ps {
namespace comm {

class TcpClient {
 public:
  using OnConnected = std::function<void(const TcpClient &)>;
  using OnDisconnected = std::function<void(const TcpClient &, int)>;
  using OnRead = std::function<void(const TcpClient &, const void *, size_t)>;
  using OnTimeout = std::function<void(const TcpClient &)>;

  explicit TcpClient(std::string address, std::uint16_t port);
  virtual ~TcpClient();

  std::string GetServerAddress() const;
  void SetCallback(const OnConnected &conn, const OnDisconnected &disconn, const OnRead &read,
                   const OnTimeout &timeout);
  void InitTcpClient();
  void StartWithDelay(int seconds);
  void Stop();
  void Start();
  void StartWithNoBlock();

 protected:
  static void SetTcpNoDelay(const evutil_socket_t &fd);
  static void TimeoutCallback(evutil_socket_t fd, std::int16_t what, void *arg);
  static void ReadCallback(struct bufferevent *bev, void *ctx);
  static void EventCallback(struct bufferevent *bev, std::int16_t events, void *ptr);
  virtual void OnReadHandler(const void *buf, size_t num);

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

class TcpMessageClient : public TcpClient {
 public:
  using OnMessage = std::function<void(const TcpMessageClient &, const void *, size_t)>;

  explicit TcpMessageClient(std::string address, std::uint16_t port);
  ~TcpMessageClient() override = default;

  void OnReadHandler(const void *buf, size_t num) override;
  void ReceiveMessage(const OnMessage &cb);
  void SendMessage(const void *buf, size_t num) const;

 private:
  OnMessage message_callback_;
  TcpMessageHandler message_handler_;
};

class TcpKVClient : public TcpClient {
 public:
  using OnKVMessage = std::function<void(const TcpKVClient &, const CommMessage &)>;

  explicit TcpKVClient(std::string address, std::uint16_t port);
  ~TcpKVClient() override = default;

  void OnReadHandler(const void *buf, size_t num) override;
  void ReceiveKVMessage(const OnKVMessage &cb);
  void SendKVMessage(const CommMessage &message) const;

 private:
  OnKVMessage kv_message_callback_;
  TcpMessageHandler message_handler_;
};

}  // namespace comm
}  // namespace ps
}  // namespace mindspore
#endif  // MINDSPORE_CCSRC_PS_COMM_TCP_CLIENT_H_
