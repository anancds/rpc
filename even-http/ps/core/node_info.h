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

#include <string>

#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"

#ifndef RPC_NODE_INFO_H
#define RPC_NODE_INFO_H

#endif  // RPC_NODE_INFO_H

namespace mindspore {
namespace ps {
namespace core {

enum NodeEvent { NODE_TIMEOUT = 0 };

struct NodeInfo {
  NodeInfo() : port_(0), role_(NodeRole::SCHEDULER), rank_id_(0) {}
  // ip
  std::string ip_;
  // the port of this node
  uint16_t port_;
  // the current Node unique id:0,1,2...
  std::string node_id_;
  // the role of the node: worker,server,scheduler
  NodeRole role_;
  // the current Node rank id,the worker node range is:[0,numOfWorker-1], the server node range is:[0, numOfServer-1]
  uint32_t rank_id_;
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore
