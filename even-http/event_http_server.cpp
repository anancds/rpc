//
// Created by cds on 2020/9/29.
//

#include "event_http_server.h"
#include <arpa/inet.h>
#ifdef WIN32
#include <WinSock2.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>

#include <event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_compat.h>
#include <event2/http.h>
#include <event2/http_compat.h>
#include <event2/http_struct.h>
#include <event2/listener.h>
#include <event2/util.h>

#include "event_http_request.h"

namespace Network {
EvHttpServ::EvHttpServ() : EvHttpServ("", 0) {}

EvHttpServ::~EvHttpServ() {
  //#TODO Works only if no requests are currently being served.
  if (evHttp_) {
    evhttp_free(evHttp_);
  }
  if (evBase_) {
    event_base_free(evBase_);
  }
}


EvHttpServ::EvHttpServ(std::string const &strAddr,
                       std::uint16_t nPort)  {
  if (strAddr != "") {
    unsigned long uAddr = inet_addr(strAddr.c_str());
    if (INADDR_NONE == uAddr) {
//      Utilis::LogFatal(
//        "Server address illegal, inet_addr convertion failed!\n");
//      throw EvHttpServRTEXCP(
//        "Server address illegal，inet_addr convertion failed!");
    }
    servAddr_ = strAddr;
  }
  if (nPort > 0) {
    servPort_ = nPort;
  }

  evBase_ = event_base_new();
  if (!evBase_) {
//    Utilis::LogFatal("Event base initialize failed!\n");
//    throw EvHttpServRTEXCP("Event base initialize failed!");
  }
  evHttp_ = evhttp_new(evBase_);
  if (!evHttp_) {
//    Utilis::LogFatal("Event http initialize failed!\n");
//    throw EvHttpServRTEXCP("Event http initialize failed!");
  }
  int ret = evhttp_bind_socket(evHttp_, servAddr_.c_str(), servPort_);
  if (ret != 0) {
//    Utilis::LogFatal("Http bind server addr:%s & port:%d failed!\n",
//                     servAddr_.c_str(), servPort_);
//    throw EvHttpServRTEXCP("Http bind server addr & port failed!");
  }

}

void EvHttpServ::SetTimeOut(int seconds) {
  evhttp_set_timeout(evHttp_, seconds);
}

void EvHttpServ::SetAllowedMethod(HttpMethodsSet methods) {
  evhttp_set_allowed_methods(evHttp_, methods);
}

void EvHttpServ::SetMaxHeaderSize(size_t num) {
  evhttp_set_max_headers_size(evHttp_, num);
}

void EvHttpServ::SetMaxBodySize(size_t num) {
  evhttp_set_max_body_size(evHttp_, num);
}

bool EvHttpServ::RegistHandler(std::string const &strUrl, handle_t *funcPtr) {
  HandlerFunc func = funcPtr;
  if (!func) {
    return false;
  }

  //#TODO add middleware support
  auto TransFunc = [](struct evhttp_request *req, void *arg) {
    if (nullptr == req) {
//      Utilis::LogWarn("Evhttp Request handler is NULL\n");
      return;
    }
    EvHttpResp httpReq(req);
    try {
      handle_t *f = reinterpret_cast<handle_t *>(arg);
      f(&httpReq);
    }  catch (std::exception e) {
//      Utilis::LogError("Evhttp Request handle with unexpected exception :%s\n",
//                       e.what());
    }
  };
  handle_t **pph = func.target<handle_t *>();
  if (pph != nullptr) {
    /// O SUCCESS,-1 ALREADY_EXIST,-2 FAILURE
    return (-2 != evhttp_set_cb(evHttp_, strUrl.c_str(), TransFunc,
                                reinterpret_cast<void *>(*pph)));
  } else {
//    Utilis::LogError(
//      "Evhttp regist handle of:%s with error, function pointer get failed.\n",
//      strUrl.c_str());
    return false;
  }
}

bool EvHttpServ::UnRegistHandler(std::string const &strUrl) {
  return (0 == evhttp_del_cb(evHttp_, strUrl.c_str()));
}

bool EvHttpServ::Start() {
  int ret = event_base_dispatch(evBase_);
  if (0 == ret)
    return true;
  else if (1 == ret) {
//    Utilis::LogFatal(
//      "Event base dispatch failed with no events pending or active!\n");
    return false;
  } else if (-1 == ret) {
//    Utilis::LogFatal("Event base dispatch failed with error occurred!\n");
    return false;
  } else {
//    Utilis::LogFatal("Event base dispatch with unexpect error code!\n");
//    throw EvHttpServRTEXCP("Event base dispatch with unexpect error code!");
  }
}

bool EvHttpServ::Stop() {
  if (evHttp_) {
    std::cout << evHttp_ << std::endl;
    evhttp_free(evHttp_);
  }
  if (evBase_) {
    std::cout << evBase_ << std::endl;
    event_base_free(evBase_);
  }
}

} // namespace Network
