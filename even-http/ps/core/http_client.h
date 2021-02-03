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

#ifndef MINDSPORE_CCSRC_PS_CORE_HTTP_CLIENT_H_
#define MINDSPORE_CCSRC_PS_CORE_HTTP_CLIENT_H_

#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/listener.h>
#include <event2/util.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <atomic>
#include <thread>

#include "ps/core/http_message_handler.h"

namespace mindspore {
namespace ps {
namespace core {

enum class HttpMethod {
  HM_GET = 1 << 0,
  HM_POST = 1 << 1,
  HM_HEAD = 1 << 2,
  HM_PUT = 1 << 3,
  HM_DELETE = 1 << 4,
  HM_OPTIONS = 1 << 5,
  HM_TRACE = 1 << 6,
  HM_CONNECT = 1 << 7,
  HM_PATCH = 1 << 8
};

class HttpClient {
 public:
  HttpClient() : event_base_(nullptr), event_http_(nullptr), is_init_(false) {}

  virtual ~HttpClient() = default;

  bool Init();
  int MakeRequest(const std::string &url, HttpMethod method, const void *data, size_t len,
                  const std::map<std::string, std::string> &headers, std::shared_ptr<std::vector<char>> output);

 private:
  static void RemoteReadCallback(struct evhttp_request *remote_rsp, void *arg);
  static int ReadHeaderDoneCallback(struct evhttp_request *remote_rsp, void *arg);
  static void ReadChunkCallback(struct evhttp_request *remote_rsp, void *arg);
  static void RemoteRequestErrorCallback(enum evhttp_request_error error, void *arg);
  static void RemoteConnectionCloseCallback(struct evhttp_connection *connection, void *arg);

  void AddHeaders(const std::map<std::string, std::string> &headers, struct evhttp_request *request,
                  std::shared_ptr<HttpMessageHandler> handler);

  std::string server_address_;
  std::uint16_t server_port_;
  struct event_base *event_base_;
  struct evdns_base *dns_base_;
  struct evhttp *event_http_;
  bool is_init_;
};

}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_PS_CORE_HTTP_CLIENT_H_
