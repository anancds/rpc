//
// Created by cds on 2020/12/16.
//

#include "worker.h"

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

void Worker::InitPSEmbeddingTable(const size_t &key, const std::vector<size_t> &input_shape,
                                  const std::vector<size_t> &indices_shape, const std::vector<size_t> &output_shape) {
  bool has_init = IsKeyInit(key);
  if (has_init) {
    MS_LOG(DEBUG) << "The key embedding table of key " << key << " is initialized.";
    return;
  }

  EmbeddingTableMeta embedding_table_meta;
  embedding_table_meta.set_key(key);
  *embedding_table_meta.mutable_input_shape() = {input_shape.begin(), input_shape.end()};
  *embedding_table_meta.mutable_indices_shape() = {indices_shape.begin(), indices_shape.end()};
  *embedding_table_meta.mutable_output_shape() = {output_shape.begin(), output_shape.end()};
//  embedding_table_meta.set_command(PSCommand::INIT_EMBEDDING_TABLE);

  worker_node_.BroadcastToServers(embedding_table_meta.SerializeAsString());
}

void Worker::LookupIdSlicer(const EmbeddingTableLookup &send,
                            std::vector<std::pair<bool, EmbeddingTableLookup>> *sliced, const std::map<int64_t, int64_t> &attrs) {
  MS_EXCEPTION_IF_NULL(sliced);

  const Key &key = send.key();
  const std::vector<EmbeddingTableShardMetadata> &ranges = *(embedding_table_ranges_[key]);
  sliced->resize(ranges.size());

  for (size_t i = 0; i < ranges.size(); i++) {
    const EmbeddingTableShardMetadata &range = ranges[i];
    const auto &begin = range.begin();
    const auto &end = range.end();
    std::unordered_set<int32_t> unique_ids;
    auto &kvs = sliced->at(i).second;

    kvs.set_key(key);

    std::for_each(send.keys().begin(), send.keys().end(), [&](int32_t lookup_id){
      if (lookup_id >= begin && lookup_id <= end) {
        unique_ids.insert(lookup_id);
      }
    });

    *kvs.mutable_keys() = {unique_ids.begin(), unique_ids.end()};

    if (kvs.keys().empty()) {
      sliced->at(i).first = false;
    } else {
      sliced->at(i).first = true;
    }
  }
}

bool Worker::IsKeyInit(const size_t &key) {
  if (init_keys_.find(key) == init_keys_.end() || !init_keys_[key]) {
    return false;
  }
  return true;
}
}  // namespace ps
}  // namespace mindspore