//
// Created by cds on 2020/10/16.
//
#ifndef RPC_TCP_MESSAGE_HANDLER_H
#define RPC_TCP_MESSAGE_HANDLER_H

#include <functional>
#include <memory>
#include <iostream>

using messageReceive = std::function<void(const void *buffer, size_t len)>;

class TcpMessageHandler {
 public:
  explicit TcpMessageHandler() : message_buffer_(new std::string("")) {}
  virtual ~TcpMessageHandler() = default;

  void SetCallback(messageReceive cb);
  void ReceiveMessage(const void *buffer, size_t num);

 private:
  messageReceive message_callback_;
  std::unique_ptr<std::string> message_buffer_;
};
#endif  // RPC_TCP_MESSAGE_HANDLER_H
