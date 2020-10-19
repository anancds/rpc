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
#include "http_message_handler.h"
class CommUtil {
 public:
  static bool CheckIp(const std::string &ip);
};

#endif  // RPC_COMM_UTIL_H
