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

#include "ps/comm/tcp_message_handler.h"

#include <arpa/inet.h>
#include <iostream>
#include <utility>

namespace mindspore {
namespace ps {
namespace comm {

void TcpMessageHandler::SetCallback(const messageReceive &message_receive) { message_callback_ = message_receive; }

void TcpMessageHandler::ReceiveMessage(const void *buffer, size_t num) {
  MS_EXCEPTION_IF_NULL(buffer);

  std::vector<unsigned char> buffer_data(num);

  int ret = memcpy_s(buffer_data.data(), num, buffer, num);
  if (ret != 0) {
    MS_LOG(EXCEPTION) << "The memcpy_s error, errorno(" << ret << ")";
  }

  message_buffer_.insert(message_buffer_.end(), buffer_data.begin(), buffer_data.end());

  while ((!is_parsed_ && message_buffer_.size() >= sizeof(message_header_)) ||
         message_buffer_.size() >= message_header_.message_length_) {
    if (!is_parsed_) {
      message_header_.message_magic_ = *reinterpret_cast<const uint32_t *>(message_buffer_.data());
      message_header_.message_magic_ = ntohl(message_header_.message_magic_);

      message_header_.message_length_ = *reinterpret_cast<const uint32_t *>(message_buffer_.data() + sizeof(uint32_t));
      message_header_.message_length_ = ntohl(message_header_.message_length_);

      message_buffer_.erase(message_buffer_.begin(), message_buffer_.begin() + sizeof(message_header_));

      if (message_header_.message_magic_ != MAGIC) {
        MS_LOG(EXCEPTION) << "Message header is wrong!";
      }
      is_parsed_ = true;
    } else if (message_buffer_.size() >= message_header_.message_length_) {
      CommMessage pb_message;
      pb_message.ParseFromArray(reinterpret_cast<const void *>(message_buffer_.data()),
                                message_header_.message_length_);
      if (message_callback_) {
        message_callback_(pb_message);
      }
      message_buffer_.erase(message_buffer_.begin(), message_buffer_.begin() + message_header_.message_length_);
      message_header_.message_length_ = MAX_LENGTH;
      is_parsed_ = false;
      message_header_.message_magic_ = 0;
    }
  }
}

}  // namespace comm
}  // namespace ps
}  // namespace mindspore
