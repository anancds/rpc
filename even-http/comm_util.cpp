//
// Created by cds on 2020/10/19.
//

#include "comm_util.h"
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <regex>

namespace mindspore {
namespace ps {
namespace comm {

bool CommUtil::CheckIp(const std::string &ip) {
  std::regex pattern("((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?).){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)");
  std::smatch res;
  if (regex_match(ip, res, pattern)) {
    return true;
  }
  return false;
}

void CommUtil::CheckIpAndPort(const std::string &ip, std::uint16_t port) {
  if (!CheckIp(ip)) {
    MS_LOG(EXCEPTION) << "Server address" << ip << " illegal!";
  }
  int64_t uAddr = inet_addr(ip.c_str());
  if (INADDR_NONE == uAddr) {
    MS_LOG(EXCEPTION) << "Server address illegal, inet_addr converting failed!";
  }
  if (port <= 0) {
    MS_LOG(EXCEPTION) << "Server port:" << port << " illegal!";
  }
}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
