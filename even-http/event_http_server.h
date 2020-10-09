//
// Created by cds on 2020/9/29.
//

#ifndef RPC_EVENT_HTTP_SERVER_H
#define RPC_EVENT_HTTP_SERVER_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>

#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/listener.h>
#include <event2/util.h>

#include "event_http_request.h"

namespace Network {

typedef enum eHttpMethod {
  HM_GET = 1 << 0,
  HM_POST = 1 << 1,
  HM_HEAD = 1 << 2,
  HM_PUT = 1 << 3,
  HM_DELETE = 1 << 4,
  HM_OPTIONS = 1 << 5,
  HM_TRACE = 1 << 6,
  HM_CONNECT = 1 << 7,
  HM_PATCH = 1 << 8
} HttpMethod;
typedef unsigned short HttpMethodsSet;

typedef void(handle_t)(EvHttpResp *);

class EvHttpServ {
 public:
  typedef std::function<handle_t> HandlerFunc;

 private:
  std::string servAddr_{"0.0.0.0"};
  std::uint16_t servPort_{1576};

  struct event_base *evBase_ = NULL;
  struct evhttp *evHttp_ = NULL;

 public:
  EvHttpServ();
  ~EvHttpServ();
  /// Server address only support IPV4 now, and should be in format of "x.x.x.x"
  EvHttpServ(std::string const &strAddr, std::uint16_t nPort);
  /// Time unit: seconds
  void SetTimeOut(int seconds = 5);
  /// Default allowed methods: GET, POST, HEAD, PUT, DELETE
  void SetAllowedMethod(HttpMethodsSet methods);
  /// Default to ((((unsigned long long)0xffffffffUL) << 32) | 0xffffffffUL)
  void SetMaxHeaderSize(std::size_t num);
  /// Default to ((((unsigned long long)0xffffffffUL) << 32) | 0xffffffffUL)
  void SetMaxBodySize(std::size_t num);
  /// Return: true if success, false if failed, check log to find failure reason
  bool RegistHandler(std::string const &strUrl, handle_t *func);
  bool UnRegistHandler(std::string const &strUrl);
  bool Start();
  bool Stop();
};

}  // namespace Network
#endif  // RPC_EVENT_HTTP_SERVER_H
