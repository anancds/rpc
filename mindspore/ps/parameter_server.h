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

#ifndef RPC_PARAMETER_SERVER_H
#define RPC_PARAMETER_SERVER_H

#include <iostream>
#include <unordered_map>

#include "ps/common.h"

#include <iostream>
#include <algorithm>
#include <vector>

#include "ps/embedding_table_shard_metadata.h"
#include "ps/common.h"
#include "ps/util.h"
#include "ps/core/worker_node.h"
#include "utils/log_adapter.h"
#include "../../build/even-http/ps/core/comm.pb.h"
#include "../../build/even-http/ps/core/ps.pb.h"
#include "ps/core/server_node.h"

namespace mindspore {
namespace ps {

class ParameterServer {
 public:
  static ParameterServer &GetInstance() {
    static ParameterServer instance;
    return instance;
  }

 private:
  ParameterServer() {}
  ~ParameterServer() = default;
  ParameterServer(const ParameterServer &) = delete;
  ParameterServer &operator=(const ParameterServer &) = delete;

  class ServerHandler {
   public:
    explicit ServerHandler(ParameterServer *ps) : ps_(ps) {}
    ~ServerHandler() = default;
    void Init();
    void operator()(const core::TcpServer &server, const core::TcpConnection &conn, const core::MessageMeta &meta,
                    const std::string &message, std::string *res);
    void HandleInitEmbeddings(const core::MessageMeta &meta, const std::string &message, std::string *res);

   private:
    ParameterServer *ps_;
    using RequestHandler =
      std::function<void(const core::MessageMeta &meta, const std::string &message, std::string *res)>;
    std::unordered_map<int64_t, RequestHandler> handlers_;
    std::unordered_map<Key, bool> init_weights_;
    std::unordered_map<Key, bool> init_weight_to_optim_;
    std::unordered_map<Key, bool> init_optim_info_;
  };

  bool Init();
  void InitOptimInfoBuilders();
  void InitWeightKeyToOptims(const Key &key, const int64_t &optim_id);
  void InitOptimInputsShape(const Keys &keys, const Values &values, const Lengths &lengths);
  void InitWeight(const Key &key, const WeightPtr &weight);
  void InitGrad(const Key &key, const GradPtr &grad);
  void InitEmbeddingTable(const Key &key,
                          const std::shared_ptr<std::vector<std::shared_ptr<std::vector<size_t>>>> &shapes,
                          const ParamInitInfo &param_init_info);

  std::mutex &mutex();
  size_t pserver_num_;
  size_t worker_num_;
  size_t rank_id_;
  size_t grad_accum_count_;
  std::unique_ptr<ServerHandler> handler_;
  bool running_;

  std::unordered_map<Key, InputsShapePtr> optim_inputs_shape_;
  std::unordered_map<Key, InputsShapePtr> original_optim_inputs_shape_;
  std::unordered_map<Key, std::string> weight_key_to_optims_;
  std::unordered_map<Key, std::string> weight_key_to_optim_op_;
  std::unordered_map<Key, WeightPtr> weights_;
  std::unordered_map<Key, bool> is_embedding_;
  std::unordered_map<Key, WeightPtr> grads_;
  std::unordered_map<Key, size_t> grads_accum_counter_;
  std::unordered_map<Key, uint64_t> tokens_;

  std::mutex mutex_;
  std::condition_variable apply_grads_cv_;

  std::unique_ptr<std::thread> thread_;
  core::ServerNode server_node_;
};
}  // namespace ps
}  // namespace mindspore

#endif  // RPC_PARAMETER_SERVER_H
