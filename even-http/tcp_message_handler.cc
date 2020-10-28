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

#include "tcp_message_handler.h"

#include <arpa/inet.h>
#include <iostream>
#include <utility>

namespace mindspore {
namespace ps {
namespace comm {

void TcpMessageHandler::SetCallback(const messageReceive &message_receive) { message_callback_ = message_receive; }

void TcpMessageHandler::SetKVCallback(const messageKVReceive &message_kv_receive) {
  message_kv_callback_ = message_kv_receive;
}

void TcpMessageHandler::ReceiveMessage(const void *buffer, size_t num) {
  MS_EXCEPTION_IF_NULL(buffer);

  message_buffer_.append(reinterpret_cast<const char *>(buffer), num);

  while (message_buffer_.size() >= message_header_.message_length_ ||
         (message_header_.message_length_ == 0xFFFFFFFF && message_buffer_.size() >= sizeof(message_header_))) {
    if (message_header_.message_length_ == 0xFFFFFFFF) {
      if (message_buffer_.size() >= sizeof(message_header_)) {
        message_header_.message_magic_ = *reinterpret_cast<const uint32_t *>(message_buffer_.c_str());
        message_header_.message_magic_ = ntohl(message_header_.message_magic_);

        message_header_.message_length_ = *reinterpret_cast<const uint32_t *>(message_buffer_.c_str() + sizeof(uint32_t) * 3);
        message_header_.message_length_ = ntohl(message_header_.message_length_);

        message_buffer_.erase(0, sizeof(message_header_));

        if (message_header_.message_magic_ != Message::MAGIC) {
          // Send error
          if (message_callback_) message_callback_(nullptr, 0xFFFFFFFF);
        }
      }
    } else if (message_buffer_.size() >= message_header_.message_length_) {
      if (message_callback_) {
        message_callback_(message_buffer_.c_str(), message_header_.message_length_);
      }
      message_buffer_.erase(0, message_header_.message_length_);
      message_header_.message_length_ = 0xFFFFFFFF;
      message_header_.message_magic_ = 0;
    }
  }
}

void TcpMessageHandler::ReceiveKVMessage(const void *buffer, size_t num) {
  MS_EXCEPTION_IF_NULL(buffer);

  message_buffer_.append(reinterpret_cast<const char *>(buffer), num);

  while (message_buffer_.size() >= message_header_.message_length_ ||
         (message_header_.message_length_ == 0xFFFFFFFF && message_buffer_.size() >= sizeof(message_header_))) {
    if (message_header_.message_length_ == 0xFFFFFFFF) {
      if (message_buffer_.size() >= sizeof(message_header_)) {
        message_header_.message_magic_ = *reinterpret_cast<const uint32_t *>(message_buffer_.c_str());
        message_header_.message_magic_ = ntohl(message_header_.message_magic_);

        message_header_.message_key_length_ = *reinterpret_cast<const uint32_t *>(message_buffer_.c_str() + sizeof(uint32_t));
        message_header_.message_key_length_ = ntohl(message_header_.message_length_);

        message_header_.message_value_length_ = *reinterpret_cast<const uint32_t *>(message_buffer_.c_str() + sizeof(uint32_t) * 2);
        message_header_.message_value_length_ = ntohl(message_header_.message_length_);

        message_header_.message_length_ = *reinterpret_cast<const uint32_t *>(message_buffer_.c_str() + sizeof(uint32_t) * 3);
        message_header_.message_length_ = ntohl(message_header_.message_length_);

        message_buffer_.erase(0, sizeof(message_header_));


        if (message_header_.message_magic_ != Message::MAGIC) {
          // Send error
          if (message_callback_) message_callback_(nullptr, 0xFFFFFFFF);
        }
      }
    } else if (message_buffer_.size() >= message_header_.message_length_) {
      Message message{};
      std::string key_data = message_buffer_.substr(0, message_header_.message_key_length_);
      message.keys_ =reinterpret_cast<const void *>(key_data.c_str());
      message_buffer_.erase(0, message_header_.message_key_length_);
      message.values_ = reinterpret_cast<const void*>(message_buffer_.c_str());
      if (message_kv_callback_) {
        message_kv_callback_(message);
      }
      message_buffer_.erase(0, message_header_.message_length_);
      message_header_.message_length_ = 0xFFFFFFFF;
      message_header_.message_magic_ = 0;
    }
  }
}

}  // namespace comm
}  // namespace ps
}  // namespace mindspore
