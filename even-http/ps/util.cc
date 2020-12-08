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

#include "ps/util.h"
#include <unordered_map>
#include <vector>
#include "utils/ms_utils.h"

namespace mindspore {
namespace ps {
int64_t Util::rank_id_ = -1;


int64_t Util::optimizer_id(std::string name) {
  if (optimizer_to_ids.count(name) > 0) {
    return optimizer_to_ids[name];
  }
  return -1;
}

std::string Util::optimizer_name(int64_t id) {
  if (id_to_optimizers.count(id) > 0) {
    return id_to_optimizers[id];
  }
  return "";
}

std::string Util::optimizer_node_name(int64_t id) {
  if (id_to_optimizer_nodes.count(id) > 0) {
    return id_to_optimizer_nodes[id];
  }
  return "";
}

bool Util::is_optimizer(std::string name) { return optimizer_to_ids.count(name) > 0; }

int64_t Util::LocalShard(int64_t first_dim, int64_t rank_id, int64_t server_num) {
  std::map<int64_t, int64_t> shard_dims = AllRankLocalShard(first_dim, rank_id, server_num);
  if (shard_dims.count(rank_id) == 0) {
    MS_LOG(EXCEPTION) << "Invalid rank id " << rank_id;
  }
  return shard_dims[rank_id];
}

std::map<int64_t, int64_t> Util::AllRankLocalShard(int64_t first_dim, int64_t rank_id, int64_t server_num) {
  if (first_dim <= 0 || server_num <= 0 || rank_id < 0) {
    MS_LOG(EXCEPTION) << "Input values are invalid.";
  }
  if (rank_id >= server_num) {
    MS_LOG(EXCEPTION) << "The rank ID " << rank_id << " should be less than the number of servers " << server_num;
  }
  std::map<int64_t, int64_t> shard_dims;
  for (int64_t i = 0; i < server_num; i++) {
    shard_dims[i] = 0;
  }
  if (server_num != static_cast<int64_t>(shard_dims.size())) {
    MS_LOG(EXCEPTION) << "Inconsistent server num " << server_num << " shard dims counter size " << shard_dims.size();
  }
  int64_t server_index = -1;
  for (int64_t i = 0; i < first_dim; i++) {
    server_index = (server_index + 1) % server_num;
    shard_dims[server_index] = shard_dims[server_index] + 1;
  }
  if (shard_dims.count(rank_id) == 0) {
    MS_LOG(EXCEPTION) << "Invalid rank id " << rank_id << ", total server num " << server_num;
  }
  return shard_dims;
}

}  // namespace ps
}  // namespace mindspore
