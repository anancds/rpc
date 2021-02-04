#include "event2/http.h"
#include "event2/http_struct.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/dns.h"
#include "event2/thread.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/queue.h>
#include <event.h>

#include <ps/core/http_client.h>

namespace mindspore {
namespace ps {
namespace core {
bool HttpClient::Init() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!is_init_) {
    is_init_ = true;
    event_base_ = event_base_new();
    MS_EXCEPTION_IF_NULL(event_base_);
    dns_base_ = evdns_base_new(event_base_, 1);
    MS_EXCEPTION_IF_NULL(dns_base_);
  }
  return true;
}

int HttpClient::MakeRequest(const std::string &url, HttpMethod method, const void *data, size_t len,
                            const std::map<std::string, std::string> &headers,
                            std::shared_ptr<std::vector<char>> output) {
  auto handler = std::make_shared<HttpMessageHandler>();
  handler->set_http_base(event_base_);
  output->clear();
  handler->set_body(output);
  struct evhttp_request *request_ = evhttp_request_new(RemoteReadCallback, reinterpret_cast<void *>(handler.get()));
  evhttp_request_set_header_cb(request_, ReadHeaderDoneCallback);
  evhttp_request_set_chunked_cb(request_, ReadChunkCallback);
  evhttp_request_set_error_cb(request_, RemoteRequestErrorCallback);
  handler->InitRequest(url);

  MS_LOG(DEBUG) << "The url is:" << url << ", The host is:" << handler->GetHostByUri()
                << ", The port is:" << handler->GetUriPort()
                << ", The request_url is:" << handler->GetRequestPath()->data();

  struct evhttp_connection *connection =
    evhttp_connection_base_new(event_base_, dns_base_, handler->GetHostByUri(), handler->GetUriPort());
  if (!connection) {
    MS_LOG(ERROR) << "Create http connection failed!";
    return HTTP_BADREQUEST;
  }

  evhttp_connection_set_closecb(connection, RemoteConnectionCloseCallback, event_base_);

  AddHeaders(headers, request_, handler);

  struct evbuffer *buffer = evhttp_request_get_output_buffer(request_);
  if (evbuffer_add(buffer, data, len) != 0) {
    MS_LOG(ERROR) << "Add buffer failed!";
    return HTTP_INTERNAL;
  }

  if (evhttp_make_request(connection, request_, evhttp_cmd_type(method), handler->GetRequestPath()->data()) != 0) {
    MS_LOG(ERROR) << "Make request failed!";
    return HTTP_INTERNAL;
  }

  if (!Start()) {
    MS_LOG(ERROR) << "Start http client failed!";
    return HTTP_INTERNAL;
  }

  if (handler->request()) {
    MS_LOG(DEBUG) << "The http response code is:" << evhttp_request_get_response_code(handler->request())
                  << ", The request code line is:" << evhttp_request_get_response_code_line(handler->request());
    return evhttp_request_get_response_code(handler->request());
  }
  return HTTP_BADREQUEST;
}

void HttpClient::RemoteReadCallback(struct evhttp_request *request, void *arg) {
  MS_EXCEPTION_IF_NULL(request);
  MS_EXCEPTION_IF_NULL(arg);
  auto handler = static_cast<HttpMessageHandler *>(arg);
  if (event_base_loopexit(handler->http_base(), nullptr) != 0) {
    MS_LOG(EXCEPTION) << "event base loop exit failed!";
  }
}

int HttpClient::ReadHeaderDoneCallback(struct evhttp_request *request, void *arg) {
  MS_EXCEPTION_IF_NULL(request);
  MS_EXCEPTION_IF_NULL(arg);
  auto handler = static_cast<HttpMessageHandler *>(arg);
  handler->set_request(request);
  MS_LOG(DEBUG) << "The http response code is:" << evhttp_request_get_response_code(request)
                << ", The request code line is:" << evhttp_request_get_response_code_line(request);
  struct evkeyvalq *headers = evhttp_request_get_input_headers(request);
  struct evkeyval *header;
  TAILQ_FOREACH(header, headers, next) {
    MS_LOG(DEBUG) << "The key:" << header->key << ",The value:" << header->value;
    std::string len = "Content-Length";
    if (!strcmp(header->key, len.c_str())) {
      handler->set_content_len(strtouq(header->value, nullptr, 10));
      handler->InitBodySize();
    }
  }
  return 0;
}

void HttpClient::ReadChunkCallback(struct evhttp_request *request, void *arg) {
  MS_EXCEPTION_IF_NULL(request);
  MS_EXCEPTION_IF_NULL(arg);
  auto handler = static_cast<HttpMessageHandler *>(arg);
  char buf[4096];
  struct evbuffer *evbuf = evhttp_request_get_input_buffer(request);
  MS_EXCEPTION_IF_NULL(evbuf);
  int n = 0;
  while ((n = evbuffer_remove(evbuf, &buf, sizeof(buf))) > 0) {
    handler->ReceiveMessage(buf, n);
  }
}

void HttpClient::RemoteRequestErrorCallback(enum evhttp_request_error error, void *arg) {
  MS_EXCEPTION_IF_NULL(arg);
  auto handler = static_cast<HttpMessageHandler *>(arg);
  MS_LOG(ERROR) << "The request failed, the error is:" << error;
  if (event_base_loopexit(handler->http_base(), nullptr) != 0) {
    MS_LOG(EXCEPTION) << "event base loop exit failed!";
  }
}

void HttpClient::RemoteConnectionCloseCallback(struct evhttp_connection *connection, void *arg) {
  MS_EXCEPTION_IF_NULL(connection);
  MS_EXCEPTION_IF_NULL(arg);
  MS_LOG(ERROR) << "Remote connection closed!";
  if (event_base_loopexit((struct event_base *)arg, nullptr) != 0) {
    MS_LOG(EXCEPTION) << "event base loop exit failed!";
  }
}

void HttpClient::AddHeaders(const std::map<std::string, std::string> &headers, struct evhttp_request *request,
                            std::shared_ptr<HttpMessageHandler> handler) {
  if (evhttp_add_header(evhttp_request_get_output_headers(request), "Host", handler->GetHostByUri()) != 0) {
    MS_LOG(EXCEPTION) << "Add header failed!";
  }
  for (auto &header : headers) {
    if (evhttp_add_header(evhttp_request_get_output_headers(request), header.first.data(), header.second.data()) != 0) {
      MS_LOG(EXCEPTION) << "Add header failed!";
    }
  }
}

bool HttpClient::Start() {
  MS_LOG(INFO) << "Start http Client!";
  MS_EXCEPTION_IF_NULL(event_base_);
  int ret = event_base_dispatch(event_base_);
  if (ret == 0) {
    MS_LOG(INFO) << "Event base dispatch success!";
    return true;
  } else if (ret == 1) {
    MS_LOG(ERROR) << "Event base dispatch failed with no events pending or active!";
    return false;
  } else if (ret == -1) {
    MS_LOG(ERROR) << "Event base dispatch failed with error occurred!";
    return false;
  } else {
    MS_LOG(EXCEPTION) << "Event base dispatch with unexpect error code!";
  }
  return true;
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore
