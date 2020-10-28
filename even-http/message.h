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

class Message {
 public:
  Message() : keys_(nullptr), values_(nullptr), key_len_(0), value_len_(0) {}
  virtual ~Message() = default;

  // MS: the shortcut of MindSpore
  static const uint32_t MAGIC = 0x4d53;

  struct MessageHeader {
    uint32_t message_magic_ = 0;
    uint32_t message_key_length_ = 0;
    uint32_t message_value_length_ = 0;
    uint32_t message_length_ = 0xFFFFFFFF;
  };

  const void *keys_;
  const void *values_;
  uint32_t key_len_;
  uint32_t value_len_;

  template <typename V>
  void AddVectorData(const std::vector<uint64_t> &key_data, const std::vector<V> &value_data) {
    keys_ = key_data.data();
    values_ = value_data.data();
    key_len_ = key_data.size() * sizeof(std::vector<uint32_t>);
    value_len_ = value_data.size() * sizeof(std::vector<V>);
  }

  template <typename V>
  void AddArrayData(const uint64_t *key_data, const V *value_data, uint64_t key_len, uint64_t value_len) {
    keys_ = (void *)(key_data);
    values_ = (void *)(value_data);
    key_len_ = key_len * sizeof(uint32_t *);
    value_len_ = value_len * sizeof(V *);
  }
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
#endif  // MINDSPORE_CCSRC_PS_COMM_MESSAGE_H_
