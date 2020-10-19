//
// Created by cds on 2020/10/19.
//

#include "comm_util.h"
#include <arpa/inet.h>
#include <event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_compat.h>
#include <event2/http.h>
#include <event2/http_compat.h>
#include <event2/http_struct.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <regex>
#include "http_message_handler.h"
#include "http_server.h"

bool CommUtil::CheckIp(const std::string &ip) {
  std::regex pattern("((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?).){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)");
  std::smatch res;
  if (regex_match(ip, res, pattern)) {
    return true;
  }
  return false;
}