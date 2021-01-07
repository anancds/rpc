//
// Created by cds on 2020/12/16.
//

#include "worker.h"

namespace mindspore {
namespace ps {
void Worker::Initialize() {
  lookup_slicer_ = [this](auto &&send, auto &&sliced, auto &&attrs) { LookupIdSlicer(send, sliced, attrs); };
}

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

void Worker::AddKeyToServerId(const Key &key) {}

void Worker::AddKeyByHashMod(const Key &key) {
  if (server_num_ == 0) {
    MS_LOG(EXCEPTION) << "Server number is invalid:0";
  }
  key_to_server_id_[key] = static_cast<int64_t>(key % server_num_);
  MS_LOG(INFO) << "The server id of key " << key << " is " << key_to_server_id_[key];
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

  PSMessage ps_message;
  ps_message.set_command(PSCommand::INIT_EMBEDDING_TABLE);
  ps_message.set_data(embedding_table_meta.SerializeAsString());

  worker_node_.Broadcast(NodeRole::SERVER, ps_message.SerializeAsString());
}

void Worker::LookupIdSlicer(const EmbeddingTableLookup &send, SlicedEmbeddingMessages *sliced,
                            const std::map<int64_t, int64_t> &attrs) {
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

    std::for_each(send.keys().begin(), send.keys().end(), [&](int32_t lookup_id) {
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

void Worker::WorkerInitEmbeddingSlicer(const KVMessage &send, std::vector<std::pair<bool, KVMessage>> *sliced,
                                       const std::map<int64_t, int64_t> &attrs) {
  MS_EXCEPTION_IF_NULL(sliced);
  sliced->resize(server_num_);
  auto keys = send.keys();
  auto vals = send.values();

  size_t col_cnt = send.values_size() / embedding_row_cnt_[keys[0]];
  const std::vector<EmbeddingTableShardMetadata> &ranges = *(embedding_table_ranges_[keys[0]]);
  for (size_t i = 0; i < ranges.size(); i++) {
    size_t offset_begin = ranges[i].begin() * col_cnt;
    size_t offset_end = (ranges[i].end() + 1) * col_cnt;
    KVMessage kvs;
    *kvs.mutable_keys() = {keys.begin(), keys.end()};
    *kvs.mutable_values() = {vals.begin() + offset_begin, vals.begin() + offset_end};
    sliced->at(i).first = true;
    sliced->at(i).second = kvs;
  }
}

void Worker::RoundRobinSlicer(const KVMessage &send, SlicedKVMessages *sliced,
                              const std::map<int64_t, int64_t> &attrs) {
  MS_EXCEPTION_IF_NULL(sliced);
  sliced->resize(server_num_);
  auto keys = send.keys();
  auto values = send.values();
  auto lens = send.len();

  int64_t server_id, len;
  Key param_key;
  for (size_t i = 0; i < send.keys_size(); i++) {
    param_key = keys[i];
    server_id = key_to_server_id_[param_key];
    if (!sliced->at(server_id).first) {
      sliced->at(server_id).first = true;
    }

    KVMessage &server_kv_pairs = sliced->at(server_id).second;
    server_kv_pairs.add_keys(param_key);
    len = lens[i];
    int64_t offset = std::accumulate(lens.begin(), lens.begin() + i, 0);
    auto val_begin = values.begin() + offset;
    auto val_end = values.end() + len;
    for (auto it = val_begin; it != val_end; ++it) {
      server_kv_pairs.add_values(*it);
    }
    server_kv_pairs.add_len(len);
  }
}

void Worker::DoPSEmbeddingLookup(const Key &key, const std::vector<int> &lookup_ids, std::vector<float> *lookup_result,
                                 int64_t cmd) {
  MS_EXCEPTION_IF_NULL(lookup_result);
  EmbeddingTableLookup embedding_table_lookup;
  embedding_table_lookup.set_key(key);
  *embedding_table_lookup.mutable_keys() = {lookup_ids.begin(), lookup_ids.end()};

  SlicedEmbeddingMessages messages;
  lookup_slicer_(embedding_table_lookup, &messages, {});
  std::vector<uint32_t> rank_ids;
  std::vector<std::string> data;
  for (size_t i = 0; i < messages.size(); i++) {
    if (messages.at(i).first) {
      rank_ids.emplace_back(i);
      data.emplace_back(messages.at(i).second.SerializeAsString());
    }
  }
  //  worker_node_.Send()
}

bool Worker::IsKeyInit(const size_t &key) {
  if (init_keys_.find(key) == init_keys_.end() || !init_keys_[key]) {
    return false;
  }
  return true;
}
}  // namespace ps
}  // namespace mindspore