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
  using SlicedKVMessages = std::vector<std::pair<bool, EmbeddingTableLookup>>;
  using Slicer = std::function<void(int64_t ts, const EmbeddingTableLookup &send,
                                    const std::vector<EmbeddingTableShardMetadata> &ranges, SlicedKVMessages *sliced,
                                    const std::map<int64_t, int64_t> &attrs)>;

  void AddEmbeddingTable(const Key &key, const size_t &row_count);
  void InitPSEmbeddingTable(const size_t &key, const std::vector<size_t> &input_shape,
                            const std::vector<size_t> &indices_shape, const std::vector<size_t> &output_shape);

  void LookupIdSlicer(const EmbeddingTableLookup &send, std::vector<std::pair<bool, EmbeddingTableLookup>> *sliced,
                      const std::map<int64_t, int64_t> &attrs);

 private:
  bool IsKeyInit(const size_t &key);

  std::unordered_map<Key, std::shared_ptr<std::vector<EmbeddingTableShardMetadata>>> embedding_table_ranges_;
  int64_t server_num_{3};
  std::unordered_map<Key, size_t> embedding_row_cnt_;
  std::map<size_t, bool> init_keys_;
  WorkerNode worker_node_;
};
}  // namespace ps
}  // namespace mindspore
#endif  // RPC_WORKER_H