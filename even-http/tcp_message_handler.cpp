//
// Created by cds on 2020/10/16.
//

#include "tcp_message_handler.h"
#include <iostream>
#include <utility>

#include <arpa/inet.h>

void TcpMessageHandler::SetCallback(messageReceive message_receive) { message_callback_ = std::move(message_receive); }

void TcpMessageHandler::SetKVCallBack(messageKVReceive message_kv_receive) {
  message_kv_callback_ = std::move(message_kv_receive);
}

void TcpMessageHandler::ReceiveMessage(const void *buffer, size_t num) {
  MS_EXCEPTION_IF_NULL(buffer);

  // Add to buffer
  message_buffer_.append(reinterpret_cast<const char *>(buffer), num);

  while (message_buffer_.size() >= message_header_.mLength ||
         (message_header_.mLength == 0xFFFFFFFF && message_buffer_.size() >= sizeof(message_header_))) {
    if (message_header_.mLength == 0xFFFFFFFF) {
      if (message_buffer_.size() >= sizeof(message_header_)) {
        message_header_.mMagic = *reinterpret_cast<const uint32_t *>(message_buffer_.c_str());
        message_header_.mMagic = ntohl(message_header_.mMagic);

        message_header_.mLength = *reinterpret_cast<const uint32_t *>(message_buffer_.c_str() + sizeof(uint32_t));
        message_header_.mLength = ntohl(message_header_.mLength);

        message_buffer_.erase(0, sizeof(message_header_));

        // Check magic
        if (message_header_.mMagic != Message::MAGIC) {
          // Send zero buffer to signal about error
          if (message_callback_) message_callback_(nullptr, 0xFFFFFFFF);
        }
      }
    } else if (message_buffer_.size() >= message_header_.mLength) {
      if (message_callback_) message_callback_(message_buffer_.c_str(), message_header_.mLength);
      message_buffer_.erase(0, message_header_.mLength);
      message_header_.mLength = 0xFFFFFFFF;
      message_header_.mMagic = 0;
    }
  }
}