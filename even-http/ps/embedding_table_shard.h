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

#ifndef RPC_EMBEDDINGTABLESHARD_H
#define RPC_EMBEDDINGTABLESHARD_H

#include <iostream>
#include "utils/log_adapter.h"

namespace mindspore {
namespace ps {
class EmbeddingTableShard {
 public:
  explicit EmbeddingTableShard(uint64_t begin, uint64_t end)
      : embedding_table_begin_(begin), embedding_table_end_(end) {}
  virtual ~EmbeddingTableShard() = default;

  uint64_t begin() const;
  uint64_t end() const;
  uint64_t size() const;

 private:
  void CheckRange() const;

  uint64_t embedding_table_begin_;
  uint64_t embedding_table_end_;
};
}  // namespace ps
}  // namespace mindspore
#endif  // RPC_EMBEDDINGTABLESHARD_H
