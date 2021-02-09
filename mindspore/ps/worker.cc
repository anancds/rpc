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

void Worker::AddKeyToServerId(const Key &key) { AddKeyByHashMod(key); }

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

  worker_node_.Broadcast(NodeRole::SERVER, ps_message.SerializeAsString());
}

void Worker::UpdateEmbeddingTable(const std::vector<Key> &keys, const std::vector<int> &lookup_ids,
                          const std::vector<float> &vals) {
                            KVMessage kvs;
  *kvs.mutable_keys() = {keys.begin(), keys.end()};
  *kvs.mutable_len() = {lookup_ids.begin(), lookup_ids.end()};
  *kvs.mutable_values() = {vals.begin(), vals.end()};
  
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
  for (int i = 0; i < send.keys_size(); i++) {
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
    auto val_end = val_begin + len;
    for (auto it = val_begin; it != val_end; ++it) {
      server_kv_pairs.add_values(*it);
    }
    server_kv_pairs.add_len(len);
  }
}

void Worker::SparseSlicer(const KVMessage &send, SlicedKVMessages *sliced, const std::map<int64_t, int64_t> &attrs) {
  MS_EXCEPTION_IF_NULL(sliced);
  // Init variables
  float *data = const_cast<float *>(send.values().data());

  if (attrs.count(0) == 0 || attrs.count(1) == 0 || attrs.count(2) == 0 || attrs.count(3) == 0) {
    MS_LOG(EXCEPTION) << "Invalid attrs keys";
  }
  auto iter = attrs.find(0);
  size_t grad_index = static_cast<size_t>(iter->second);
  iter = attrs.find(1);
  size_t indice_index = static_cast<size_t>(iter->second);
  iter = attrs.find(2);
  size_t first_dim_size = static_cast<size_t>(iter->second);
  iter = attrs.find(3);
  size_t outer_dim_size = static_cast<size_t>(iter->second);

  int grad_size = send.len()[grad_index];
  int indice_size = send.len()[indice_index];
  int segment_size = grad_size / indice_size;

  int64_t grad_offset = 0;
  int64_t indice_offset = 0;
  for (size_t i = 0; i < grad_index; i++) {
    grad_offset += send.len()[i];
  }
  for (size_t j = 0; j < indice_index; j++) {
    indice_offset += send.len()[j];
  }

  float *grad_data = data + grad_offset;
  int *indice_data = reinterpret_cast<int *>(data) + indice_offset;

  // Build the mappings of indice to gradient
  std::vector<std::pair<int, float *>> indice_to_grads;
  for (int i = 0; i < indice_size; i++) {
    int indice = indice_data[i];
    float *grad = grad_data + i * segment_size;
    indice_to_grads.push_back(std::make_pair(indice, grad));
  }

  const Key &key = send.keys()[0];
  const std::vector<EmbeddingTableShardMetadata> &ranges = *(embedding_table_ranges_[key]);
  sliced->resize(ranges.size());

  // Construct reduced sparse data for each server
  for (size_t i = 0; i < ranges.size(); i++) {
    const EmbeddingTableShardMetadata &range = ranges[i];
    const auto &begin = range.begin();
    const auto &end = range.end();
    auto &kvs = sliced->at(i).second;
    *kvs.mutable_keys() = {send.keys().begin(), send.keys().end()};
    *kvs.mutable_len() = {send.len().begin(), send.len().end()};

    // Prepare the sparse gradient and indice
    std::vector<int> indice_ids;
    std::unordered_set<int> distinct_ids;
    for (int j = 0; j < indice_size; j++) {
      size_t indice = static_cast<size_t>(indice_data[j]);
      if (indice >= begin && indice <= end) {
        indice_ids.push_back(indice);
        distinct_ids.insert(indice);
      }
    }
    size_t indices_size = indice_ids.size();
    // if (indices_size > 0) {
    //   int slice_segment_size = indices_size * segment_size;
    //   std::vector<float> src_grad_data(slice_segment_size);
    //   std::vector<int> src_indice_data(indices_size);
    //   PrepareSparseGradient(begin, end, distinct_ids, indice_to_grads, indice_data, segment_size,
    //   src_grad_data.data(),
    //                         src_indice_data.data());

    //   // Reduce the sparse gradient and indice
    //   std::vector<float> new_grad(slice_segment_size);
    //   std::vector<int> new_indices(indices_size);
    //   Util::ReduceSparseGradient(src_grad_data.data(), src_indice_data.data(), indices_size, segment_size,
    //                              first_dim_size, outer_dim_size, &unique_sparse_grad);

    //   // Update the length of reduce sparse gradient and indice
    //   std::vector<int> reduced_lens;
    //   reduced_lens.CopyFrom(kvs.lens);
    //   reduced_lens[grad_index] = unique_sparse_grad.indices_size_ * segment_size;
    //   reduced_lens[indice_index] = unique_sparse_grad.indices_size_;

    //   // Build the sparse value to be sent
    //   size_t total_size = std::accumulate(reduced_lens.begin(), reduced_lens.end(), 0, std::plus<int>());
    //   ::ps::SArray<T> reduced_data(total_size, 0);
    //   BuildSparseValue(reduced_lens, grad_index, indice_index, data, unique_sparse_grad.value_,
    //                    unique_sparse_grad.indices_, &reduced_data);

    //   kvs.lens = reduced_lens;
    //   kvs.vals = reduced_data;
    // }

    if (indices_size <= 0) {
      std::vector<float> no_keys;
      std::vector<float> no_vals;
      std::vector<float> no_lens;
      no_keys.push_back(key);
      no_vals.push_back(-100);
      *kvs.mutable_values() = {no_vals.begin(), no_vals.end()};
      *kvs.mutable_len() = {no_lens.begin(), no_lens.end()};
    }
    sliced->at(i).first = true;
  }
}

void Worker::UpdateEmbeddingSlicer(const KVMessage &send, SlicedKVMessages *sliced,
                                   const std::map<int64_t, int64_t> &attrs) {
  MS_EXCEPTION_IF_NULL(sliced);
  const float *embedding_vals = send.values().data();
  const int *lookup_ids = send.len().data();
  size_t val_size = send.values_size();
  size_t id_size = send.len_size();
  size_t embedding_dim = val_size / id_size;

  const Key &key = send.keys()[0];
  const std::vector<EmbeddingTableShardMetadata> &ranges = *(embedding_table_ranges_[key]);
  sliced->resize(ranges.size());

  for (size_t i = 0; i < ranges.size(); i++) {
    const EmbeddingTableShardMetadata &range = ranges[i];
    const auto &begin = range.begin();
    const auto &end = range.end();
    auto &kvs = sliced->at(i).second;
    kvs.add_keys(key);
    for (size_t j = 0; j < id_size; j++) {
      auto lookup_id = static_cast<uint64_t>(lookup_ids[j]);
      if (lookup_id >= begin && lookup_id <= end) {
        kvs.add_keys(lookup_id);
        for (size_t k = 0; k < embedding_dim; k++) {
          kvs.add_values(embedding_vals[j * embedding_dim + k]);
        }
      }
    }

    if (kvs.keys_size() <= 1) {
      sliced->at(i).first = false;
    } else {
      sliced->at(i).first = true;
    }
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

void Worker::PrepareSparseGradient(const size_t begin, const size_t end, const std::unordered_set<int> &distinct_ids,
                                   const std::vector<std::pair<int, float *>> &indice_to_grad, const int *all_indice,
                                   const size_t segment_size, float *gradient, int *indices) {
  MS_EXCEPTION_IF_NULL(all_indice);
  MS_EXCEPTION_IF_NULL(gradient);
  MS_EXCEPTION_IF_NULL(indices);
  int64_t offset = 0;
  int64_t index = 0;
  size_t segment_data_size = segment_size * sizeof(float);
  size_t dst_size;
  size_t src_size;
  void *dst_data = nullptr;
  void *src_data = nullptr;
  for (auto &pair : indice_to_grad) {
    if (distinct_ids.count(pair.first) == 0) {
      continue;
    }
    indices[index++] = pair.first;

    dst_size = segment_data_size;
    src_size = segment_data_size;
    dst_data = gradient + offset;
    src_data = pair.second;
    MS_EXCEPTION_IF_NULL(dst_data);
    MS_EXCEPTION_IF_NULL(src_data);
    auto ret = memcpy_s(gradient + offset, dst_size, pair.second, src_size);
    if (ret != 0) {
      MS_LOG(ERROR) << "memcpy_s error, errorno(" << ret << ")";
      return;
    }
    offset += segment_size;
  }
}
}  // namespace ps
}  // namespace mindspore