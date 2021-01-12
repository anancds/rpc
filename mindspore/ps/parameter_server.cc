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

inline std::mutex &ParameterServer::mutex() { return mutex_; }

void ParameterServer::ServerHandler::Init() { handlers_[kInitEmbeddingsCmd] = &ServerHandler::HandleInitEmbeddings; }

void ParameterServer::ServerHandler::operator()(std::shared_ptr<core::TcpConnection> conn,
                                                std::shared_ptr<core::CommMessage> message) {
  PSMessage ps_message;
  ps_message.ParseFromString(message->data());
  PSMessage output;
  if (ps_message.command() == PSCommand::PUSH) {
  } else if (ps_message.command() == PSCommand::PULL) {
  } else {
    auto &handler_ptr = handlers_[ps_message.command()];
    (this->*handler_ptr)(ps_message, &output);
  }
  // ps_->server_node_.Response(server, conn, *meta, output);
}

void ParameterServer::ServerHandler::HandleInitEmbeddings(const PSMessage &message, PSMessage *res) {
  std::unique_lock<std::mutex> lock(ps_->mutex());
  EmbeddingTableMeta embedding_table_meta;
  embedding_table_meta.ParseFromString(message.data());
  const Key &key = embedding_table_meta.key();
  MS_LOG(INFO) << "Initializing embedding table for key:" << key;
  std::shared_ptr<std::vector<std::shared_ptr<std::vector<size_t>>>> shapes =
    std::make_shared<std::vector<std::shared_ptr<std::vector<size_t>>>>();
  MS_EXCEPTION_IF_NULL(shapes);
  std::shared_ptr<std::vector<size_t>> input_shape = std::make_shared<std::vector<size_t>>(
    embedding_table_meta.input_shape().begin(), embedding_table_meta.input_shape().end());
  MS_EXCEPTION_IF_NULL(input_shape);
  std::shared_ptr<std::vector<size_t>> indices_shape = std::make_shared<std::vector<size_t>>(
    embedding_table_meta.indices_shape().begin(), embedding_table_meta.indices_shape().end());
  MS_EXCEPTION_IF_NULL(indices_shape);
  std::shared_ptr<std::vector<size_t>> output_shape = std::make_shared<std::vector<size_t>>(
    embedding_table_meta.output_shape().begin(), embedding_table_meta.output_shape().end());
  MS_EXCEPTION_IF_NULL(output_shape);
  shapes->push_back(input_shape);
  shapes->push_back(indices_shape);
  shapes->push_back(output_shape);

  ParamInitInfo param_init_info;
  ps_->InitEmbeddingTable(key, shapes, param_init_info);
}
}  // namespace ps
}  // namespace mindspore
