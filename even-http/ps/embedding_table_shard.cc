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

#include "embedding_table_shard.h"

namespace mindspore {
namespace ps {
uint64_t EmbeddingTableShard::begin() const {
  CheckRange();
  return embedding_table_begin_;
}

uint64_t EmbeddingTableShard::end() const {
  CheckRange();
  return embedding_table_end_;
}

uint64_t EmbeddingTableShard::size() const {
  CheckRange();
  return embedding_table_end_ - embedding_table_begin_;
}

void EmbeddingTableShard::CheckRange() const {
  if (embedding_table_begin_ > embedding_table_end_) {
    MS_LOG(EXCEPTION) << "The embedding table shard begin" << embedding_table_begin_
                      << " should not less than the embedding table shard end" << embedding_table_end_;
  }
}
}  // namespace ps
}  // namespace mindspore
