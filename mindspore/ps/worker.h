//
// Created by cds on 2020/12/16.
//

#ifndef RPC_WORKER_H
#define RPC_WORKER_H

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

using namespace mindspore::ps::core;

namespace mindspore {
namespace ps {
class Worker {
 public:
  using SlicedEmbeddingMessages = std::vector<std::pair<bool, EmbeddingTableLookup>>;
  using SlicedKVMessages = std::vector<std::pair<bool, KVMessage>>;

  using Slicer = std::function<void(const EmbeddingTableLookup &send, SlicedEmbeddingMessages *sliced,
                                    const std::map<int64_t, int64_t> &attrs)>;
  using KVSlicer =
    std::function<void(const KVMessage &send, SlicedKVMessages *sliced, const std::map<int64_t, int64_t> &attrs)>;

  void Initialize();
  void AddEmbeddingTable(const Key &key, const size_t &row_count);
  void AddKeyToServerId(const Key &key);
  void AddKeyByHashMod(const Key &key);

  void PushData(const std::vector<Key> &keys, const std::vector<float> &vals, const std::vector<int> &lens = {},
                int64_t cmd = 0, int64_t priority = 0);

  void InitPSEmbeddingTable(const size_t &key, const std::vector<size_t> &input_shape,
                            const std::vector<size_t> &indices_shape, const std::vector<size_t> &output_shape);
  void UpdateEmbeddingTable(const std::vector<Key> &keys, const std::vector<int> &lookup_ids,
                                            const std::vector<float> &vals);
  void LookupIdSlicer(const EmbeddingTableLookup &send, SlicedEmbeddingMessages *sliced,
                      const std::map<int64_t, int64_t> &attrs);
  void WorkerInitEmbeddingSlicer(const KVMessage &send, std::vector<std::pair<bool, KVMessage>> *sliced,
                                 const std::map<int64_t, int64_t> &attrs);
  void RoundRobinSlicer(const KVMessage &send, SlicedKVMessages *sliced, const std::map<int64_t, int64_t> &attrs);
  void SparseSlicer(const KVMessage &send, SlicedKVMessages *sliced, const std::map<int64_t, int64_t> &attrs);
  void UpdateEmbeddingSlicer(const KVMessage &send, SlicedKVMessages *sliced, const std::map<int64_t, int64_t> &attrs);
  void DoPSEmbeddingLookup(const Key &key, const std::vector<int> &lookup_ids, std::vector<float> *lookup_result,
                           int64_t cmd);

 private:
  bool IsKeyInit(const size_t &key);
  void PrepareSparseGradient(const size_t begin, const size_t end, const std::unordered_set<int> &distinct_ids,
                             const std::vector<std::pair<int, float *>> &indice_to_grad, const int *all_indice,
                             const size_t segment_size, float *gradient, int *indices);

  std::unordered_map<Key, std::shared_ptr<std::vector<EmbeddingTableShardMetadata>>> embedding_table_ranges_;
  int64_t server_num_{3};
  std::unordered_map<Key, int64_t> key_to_server_id_;
  std::unordered_map<Key, size_t> embedding_row_cnt_;
  std::map<size_t, bool> init_keys_;
  WorkerNode worker_node_;
  Slicer lookup_slicer_;
};
}  // namespace ps
}  // namespace mindspore
#endif  // RPC_WORKER_H
