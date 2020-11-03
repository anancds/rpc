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

#ifndef MINDSPORE_CCSRC_PS_COMM_MESSAGE_H_
#define MINDSPORE_CCSRC_PS_COMM_MESSAGE_H_

#include <iostream>
#include <vector>

namespace mindspore {
namespace ps {
namespace comm {

// MS: the shortcut of MindSpore
static const uint32_t MAGIC = 0x4d53;

enum Command { EMPTY, TERMINATE, ADD_NODE, BARRIER, ACK, HEARTBEAT };

struct MessageHeader {
  uint32_t message_magic_ = 0;
  uint32_t message_length_ = 0xFFFFFFFF;
};

}  // namespace comm
}  // namespace ps
}  // namespace mindspore
#endif  // MINDSPORE_CCSRC_PS_COMM_MESSAGE_H_
