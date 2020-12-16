//
// Created by cds on 2020/12/16.
//

#ifndef RPC_WORKER_H
#define RPC_WORKER_H

#include <iostream>

#include "ps/embedding_table_shard_metadata.h"
#include "ps/common.h"
#include "ps/util.h"
#include "utils/log_adapter.h"
#include "../../build/even-http/ps/core/comm.pb.h"
#include "../../build/even-http/ps/core/ps.pb.h"

using namespace mindspore::ps::core;

namespace mindspore {
namespace ps {
class Worker {
 public:
  void AddEmbeddingTable(const Key &key, const size_t &row_count);

  void LookupIdSlicer(int64_t timestamp, const KVMessage &send, const std::vector<EmbeddingTableShardMetadata> &,
                      std::vector<std::pair<uint32_t, KVMessage>> *sliced, const std::map<int64_t, int64_t> &attrs);

 private:
  std::unordered_map<Key, std::shared_ptr<std::vector<EmbeddingTableShardMetadata>>> embedding_table_ranges_;
  int64_t server_num_ {2};
  std::unordered_map<Key, size_t> embedding_row_cnt_;
};
}  // namespace ps
}  // namespace mindspore
#endif  // RPC_WORKER_H
