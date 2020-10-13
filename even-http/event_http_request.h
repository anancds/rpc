//
// Created by cds on 2020/9/29.
//

#ifndef RPC_EVENT_HTTP_REQUEST_H
#define RPC_EVENT_HTTP_REQUEST_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <list>
#include <map>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>


namespace Network {
typedef std::list<std::string> HttpHeadVal;
typedef std::map<std::string, HttpHeadVal> HttpHeaders;

class EvHttpResp{
  /// #TODO to support large / dynamic length
  // following MAX_POST_BODY_LEN is defined by libevent
  // const size_t               MAX_POST_BODY_LEN{1024*64};
 private:
  struct evhttp_request *evReq_{nullptr};
  const struct evhttp_uri *evUri_;
  struct evkeyvalq pathParams_ {
    0
  };
  struct evkeyvalq *headParams_{nullptr};
  struct evkeyvalq postParams_ {
    0
  };
  bool postParamParsed{false};
  std::string strBody_;

  struct evkeyvalq *respHeaders_{nullptr};
  struct evbuffer *respBuf_{nullptr};
  int respCode_{HTTP_OK};

 private:
  // Body length should no more than MAX_POST_BODY_LEN, default 64kB
  void parsePostParam();

 public:
  EvHttpResp(struct evhttp_request *req);
  ~EvHttpResp();

  std::string GetRequestUri();
  std::string GetRequestHost();  //#TODO add const of this
  // It will return -1 if no port set
  int GetUriPort();
  std::string GetUriPath();
  std::string GetUriQuery();
  // Useless to get from a request url, fragment is only for browser to locate
  /// sth.
  std::string GetUriFragment();

  std::string GetHeadParam(std::string const &strKey);
  std::string GetPathParam(std::string const &strKey);
  std::string GetPostParam(std::string const &strKey);

  std::string GetPostMsg();

  bool AddRespHeadParam(std::string const &key, std::string const &val);
  void AddRespHeaders(HttpHeaders &headers);
  bool AddRespString(std::string const &str);
  /// This will cause data memcpy, if not so, user have to make sure data
  /// lifetime last until be read #TODO: This func is dangerious, NOT RECOMMEND
  /// to use, for if len is larger than actual, cause overflow
  bool AddRespBuf(void const *data, std::size_t len);
  bool AddRespFile(std::string const &fileName);

  void SetRespCode(int code);
  /// Make sure code and all response body has finished set
  void SendResponse();
  void QuickResponse(int code, std::string const &strBody);
  void SimpleResponse(int code, HttpHeaders &headers, std::string const &strBody);
  /// If strMsg is empty, libevent will use default error code message instead
  void RespError(int nCode, std::string const &strMsg);
  void Init();
};

}  // namespace Network

#endif  // RPC_EVENT_HTTP_REQUEST_H
