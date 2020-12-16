//
// Created by cds on 2020/12/16.
//

#include "ps/worker.h"

namespace mindspore {
namespace ps {
void Worker::AddEmbeddingTable(const Key &key, const size_t &row_count) {
  uint64_t begin = 0;
  uint64_t end = 0;
  for (int64_t i = 0; i < server_num_; i++) {
    int64_t local_row_cnt = Util::LocalShard(row_count, i, server_num_);
    if (i == 0) {
      end = local_row_cnt - 1;
    } else {
      begin = end + 1;
      end += local_row_cnt;
    }
    EmbeddingTableShardMetadata embedding_table_shard(begin, end);
    if (embedding_table_ranges_.count(key) == 0) {
      embedding_table_ranges_[key] = std::make_shared<std::vector<EmbeddingTableShardMetadata>>();
      MS_EXCEPTION_IF_NULL(embedding_table_ranges_[key]);
    }
    embedding_table_ranges_[key]->push_back(embedding_table_shard);
  }
  embedding_row_cnt_[key] = row_count;
}

void Worker::LookupIdSlicer(int64_t timestamp, const KVMessage &send, const std::vector<EmbeddingTableShardMetadata> &,
                            std::vector<std::pair<uint32_t, KVMessage>> *sliced,
                            const std::map<int64_t, int64_t> &attrs) {
  MS_EXCEPTION_IF_NULL(sliced);
  int32_t *lookup_ids = send.lens.data();
  size_t id_size = send.lens.size();

  const Key &key = 1;
  const std::vector<EmbeddingTableShardMetadata> &ranges = *(embedding_table_ranges_[key]);
  sliced->resize(ranges.size());

  for (size_t i = 0; i < ranges.size(); i++) {
    const EmbeddingTableShardMetadata &range = ranges[i];
    const auto &begin = range.begin();
    const auto &end = range.end();
    std::unordered_set<int64_t> unique_ids;
    auto &kvs = sliced->at(i).second;

    kvs.add_keys(key);
    kvs.add_values(0.0f);

    for (size_t j = 0; j < id_size; j++) {
      auto lookup_id = static_cast<uint64_t>(lookup_ids[j]);
      // If lookup_id is out of range, like negative number, unique_ids will not contain it.
      // Servers always get lookup_ids in its embedding table range.
      if (lookup_id >= begin && lookup_id <= end) {
        unique_ids.insert(lookup_id);
      }
    }
    for (const auto &lookup_id : unique_ids) {
      kvs.add_keys(lookup_id);
      kvs.add_values(0.0f);
    }

    if (kvs.keys().size() > 1) {
      sliced->at(i).first = i;
    }
  }
}
}  // namespace ps
}  // namespace mindspore