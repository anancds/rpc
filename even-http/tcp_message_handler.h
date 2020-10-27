//
// Created by cds on 2020/10/16.
//
#ifndef RPC_TCP_MESSAGE_HANDLER_H
#define RPC_TCP_MESSAGE_HANDLER_H

#include "log_adapter.h"
#include <functional>
#include <iostream>
#include <memory>
#include "message.h"

using messageReceive = std::function<void(const void *buffer, size_t len)>;

class TcpMessageHandler {
 public:
  TcpMessageHandler() = default;
  virtual ~TcpMessageHandler() = default;

  void SetCallback(messageReceive cb);
  void ReceiveMessage(const void *buffer, size_t num);

 private:
  messageReceive message_callback_;
  Message::MessageHeader message_header_;
  std::string message_buffer_;
};
#endif  // RPC_TCP_MESSAGE_HANDLER_H
