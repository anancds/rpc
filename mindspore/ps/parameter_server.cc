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

#include "parameter_server.h"

namespace mindspore {
namespace ps {

void ParameterServer::ServerHandler::Init() { handlers_[kInitEmbeddingsCmd] = &ServerHandler::HandleInitEmbeddings; }

void ParameterServer::ServerHandler::operator()(const core::TcpServer &server, const core::TcpConnection &conn,
                                                const core::MessageMeta &meta, const std::string &message) {
  PSMessage ps_message;
  ps_message.ParseFromString(message);
  if (ps_message.command() == PSCommand::PUSH) {

  } else if (ps_message.command() == PSCommand::PULL) {

  } else {
    auto &handler_ptr = handlers_[ps_message.command()];
    (this->*handler_ptr)(server, conn, meta, message);
  }
  ps_->server_node_.Response(server, conn, meta, message);
}
}  // namespace ps
}  // namespace mindspore
