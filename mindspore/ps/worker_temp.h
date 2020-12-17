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

#ifndef MINDSPORE_CCSRC_PS_WORKER_temp_H_
#define MINDSPORE_CCSRC_PS_WORKER_temp_H_

#include <utility>
#include <memory>
#include <vector>
#include <string>
#include <numeric>
#include <functional>
#include <algorithm>
#include <map>
#include "utils/log_adapter.h"
#include "ir/tensor.h"
#include "ps/util.h"
#include "ps/common.h"
#include "ps/worker_proxy.h"
#include "utils/shape_utils.h"
#include "ps/ps_cache/ps_data/ps_data_prefetch.h"
#include "ps/core/worker_node.h"
#include "ps/embedding_table_shard_metadata.h"

namespace mindspore {
namespace ps {
class WorkerTemp {
 public:
  static WorkerTemp &GetInstance() {
    static WorkerTemp instance;
    return instance;
  }
  using Callback = std::function<void()>;
  using SlicedKVMessages = std::vector<std::pair<bool, EmbeddingTableLookup>>;
  using Slicer = std::function<void(const EmbeddingTableLookup &send, SlicedKVMessages *sliced,
                                    const std::map<int64_t, int64_t> &attrs)>;

  void Run();
  void Push(const std::vector<size_t> &keys, std::vector<uintptr_t> addrs, const ShapeVector &sizes);
  void Pull(const size_t key, void *dev_addr, const size_t size);
  size_t SetParamKey(const std::string &param_name);
  size_t GetParamKey(const std::string &param_name);
  void SetParamInitInServer(const std::string &param_name, bool init_in_server);
  bool GetParamInitInServer(const std::string &param_name);
  void SetKeyOptimId(size_t key, const std::string &optimizer_name);
  void SetOptimInputShapes(size_t key, const ShapeVector &shape);
  void AddEmbeddingTable(const Key &key, const size_t &row_count);
  void InitPSEmbeddingTable(const std::vector<size_t> &keys, std::vector<float> shapes, const ShapeVector &sizes);
  void InitPSParamAndOptim(const AnfNodePtr &input_node, const tensor::TensorPtr &tensor);
  void DoPSEmbeddingLookup(const std::vector<Key> &keys, const std::vector<int> &lookup_ids,
                           const std::vector<int> &lens, std::vector<float> *lookup_result, int64_t cmd);
  bool running() { return running_; }
  void Finalize();

 private:
  WorkerTemp() : running_(false), key_cnt_(0) {}
  ~WorkerTemp() = default;
  WorkerTemp(const WorkerTemp &) = delete;
  WorkerTemp &operator=(const WorkerTemp &) = delete;

  bool IsKeyInit(const size_t key);
  void InitPSOptimId(const size_t param_key);
  void InitPSOptimInputShapes(const size_t key);
  void InitPSParamData(const std::vector<size_t> &keys, void *origin_addr, size_t size);

  bool running_;
  size_t key_cnt_;
  std::map<std::string, size_t> param_to_key_;
  std::map<size_t, bool> init_keys_;
  std::map<size_t, int64_t> key_to_optimId_;
  std::map<size_t, std::vector<ShapeVector>> key_to_optim_shapes_;
  std::map<std::string, bool> param_to_init_in_server_;
  core::WorkerNode worker_node_;

  Slicer lookup_slicer_;
  Slicer sparse_slicer_;
  Slicer broadcast_slicer_;
  Slicer round_robin_slicer_;
  Slicer worker_init_embedding_slicer_;
  Slicer update_embedding_slicer_;
  std::unordered_map<int64_t, Callback> lookup_callbacks_;
  std::unordered_map<int64_t, Callback> general_callbacks_;
  std::unordered_map<int64_t, int64_t> expected_result_count_;
  std::unordered_map<Key, int64_t> key_to_server_id_;
  std::unordered_map<Key, size_t> embedding_row_cnt_;
};
}  // namespace ps
}  // namespace mindspore
#endif  // MINDSPORE_CCSRC_PS_WORKER_temp_H_
