//
// Created by cds on 2020/10/19.
//

#ifndef RPC_COMM_UTIL_H
#define RPC_COMM_UTIL_H

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
#include <string>
#include <utility>
#include "log_adapter.h"

namespace mindspore {
namespace ps {
namespace comm {

class CommUtil {
 public:
  static bool CheckIp(const std::string &ip);
  static void CheckIpAndPort(const std::string &ip, std::int16_t port);
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore

#endif  // RPC_COMM_UTIL_H
