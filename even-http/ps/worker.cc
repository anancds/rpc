//
// Created by cds on 2020/12/16.
//

#include "ps/worker.h"

namespace mindspore {
namespace ps {
void Worker::LookupIdSlicer(int64_t timestamp, const KVMessage &send, const std::vector<EmbeddingTableShardMetadata> &,
                            std::vector<std::pair<uint32_t, KVMessage>> *sliced,
                            const std::map<int64_t, int64_t> &attrs) {}
}  // namespace ps
}  // namespace mindspore