//
// Created by cds on 2020/10/16.
//

#include "tcp_message_handler.h"
#include <iostream>
void TcpMessageHandler::SetCallback(messageReceive message_receive) { message_callback_ = message_receive; }

void TcpMessageHandler::ReceiveMessage(const void* buffer, size_t num) {

  if (message_callback_) {
    message_callback_(buffer, num);
  }
}