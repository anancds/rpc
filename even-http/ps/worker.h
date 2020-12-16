//
// Created by cds on 2020/12/16.
//

#ifndef RPC_WORKER_H
#define RPC_WORKER_H

#include <iostream>

#include "ps/embedding_table_shard_metadata.h"
#include "../../build/even-http/ps/core/comm.pb.h"
#include "../../build/even-http/ps/core/ps.pb.h"

using namespace mindspore::ps::core;

namespace mindspore {
namespace ps {
class Worker {
  void LookupIdSlicer(int64_t timestamp, const KVMessage &send, const std::vector<EmbeddingTableShardMetadata> &,
                      std::vector<std::pair<uint32_t, KVMessage>> *sliced, const std::map<int64_t, int64_t> &attrs);
};
}  // namespace ps
}  // namespace mindspore
#endif  // RPC_WORKER_H
