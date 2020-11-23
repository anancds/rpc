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

#ifndef MINDSPORE_CCSRC_PS_CORE_TCP_CLIENT_H_
#define MINDSPORE_CCSRC_PS_CORE_TCP_CLIENT_H_

#include "ps/core/tcp_message_handler.h"

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/thread.h>
#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <thread>

#include "ps/core/cluster_config.h"
#include "../../../build/even-http/ps/core/comm.pb.h"

namespace mindspore {
namespace ps {
namespace core {

class TcpClient {
 public:
  using OnConnected = std::function<void(const TcpClient &)>;
  using OnDisconnected = std::function<void(const TcpClient &, int)>;
  using OnRead = std::function<void(const TcpClient &, const void *, size_t)>;
  using OnTimeout = std::function<void(const TcpClient &)>;
  using OnMessage = std::function<void(const TcpClient &, const CommMessage &)>;
  using OnTimer = std::function<void(const TcpClient &)>;

  explicit TcpClient(const std::string &address, std::uint16_t port);
  virtual ~TcpClient();

  std::string GetServerAddress() const;
  void SetCallback(const OnConnected &conn, const OnDisconnected &disconn, const OnRead &read,
                   const OnTimeout &timeout);
  void Init();
  void StartWithDelay(int seconds);
  void Stop();
  static void StopEventBase();
  void Start();
  void StartWithNoBlock();
  void SetMessageCallback(const OnMessage &cb);
  void SendMessage(const CommMessage &message) const;
  void SendMessageWithTimer();
  void set_timer_callback(const OnTimer &timer);
  const event_base& EventBase();
  void SetNodeId(const uint32_t &node_id);
  const uint32_t &NodeId() const;

 protected:
  static void SetTcpNoDelay(const evutil_socket_t &fd);
  static void TimeoutCallback(evutil_socket_t fd, std::int16_t what, void *arg);
  static void ReadCallback(struct bufferevent *bev, void *ctx);
  static void EventCallback(struct bufferevent *bev, std::int16_t events, void *ptr);
  virtual void OnReadHandler(const void *buf, size_t num);
  static void SendHeartBeatCallback(evutil_socket_t fd, int16_t event, void *arg);

 private:
  OnMessage message_callback_;
  TcpMessageHandler message_handler_;

  OnConnected connected_callback_;
  OnDisconnected disconnected_callback_;
  OnRead read_callback_;
  OnTimeout timeout_callback_;
  OnTimer on_timer_callback_;

  static event_base *event_base_;
  std::mutex connection_mutex_;
  event *event_timeout_;
  bufferevent *buffer_event_;

  std::string server_address_;
  std::uint16_t server_port_;
  uint32_t node_id_;
};

}  // namespace core
}  // namespace ps
}  // namespace mindspore
#endif  // MINDSPORE_CCSRC_PS_CORE_TCP_CLIENT_H_
