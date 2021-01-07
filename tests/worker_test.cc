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

#include "common/common_test.h"
#include "ps/embedding_table_shard_metadata.h"
#include "ps/worker.h"

namespace mindspore {
namespace ps {
class TestWorker : public UT::Common {
 public:
  TestWorker() = default;
  virtual ~TestWorker() = default;

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(TestWorker, LookUpSlice) {
  Worker worker;
  worker.AddEmbeddingTable(1, 100);

  Worker::SlicedEmbeddingMessages messages;

  EmbeddingTableLookup lookup;
  lookup.set_key(1);
  lookup.add_keys(1);
  lookup.add_keys(34);
  worker.LookupIdSlicer(lookup, &messages, {});
  std::vector<uint32_t> rank_ids;
  std::vector<std::string> data;
  for (size_t i = 0; i < messages.size(); i++) {
    if (messages.at(i).first) {
      rank_ids.emplace_back(i);
      data.emplace_back(messages.at(i).second.SerializeAsString());
    }
  }
  EXPECT_EQ(messages.size(), 3);
  EXPECT_EQ(messages.at(0).first, true);
  EXPECT_EQ(messages.at(2).first, false);
  EXPECT_EQ(messages.at(1).second.keys_size(), 1);
  EXPECT_EQ(messages.at(0).second.keys_size(), 1);
}

TEST_F(TestWorker, WorkerInitEmbeddingSlicer) {
  Worker worker;
  worker.AddEmbeddingTable(1, 10);

  Worker::SlicedKVMessages messages;
  KVMessage send;
  send.add_keys(1);
  send.add_keys(2);
  send.add_keys(8);
  send.add_keys(8);
  send.add_keys(8);
  send.add_keys(8);
  send.add_keys(8);
  send.add_keys(8);
  send.add_keys(8);
  send.add_keys(8);
  send.add_values(1);
  send.add_values(2);
  send.add_values(8);
  send.add_values(8);
  send.add_values(8);
  send.add_values(8);
  send.add_values(8);
  send.add_values(8);
  send.add_values(8);
  send.add_values(8);
  worker.WorkerInitEmbeddingSlicer(send, &messages, {});

  std::vector<uint32_t> rank_ids;
  std::vector<std::string> data;
  for (size_t i = 0; i < messages.size(); i++) {
    if (messages.at(i).first) {
      rank_ids.emplace_back(i);
      data.emplace_back(messages.at(i).second.SerializeAsString());
    }
  }
  EXPECT_EQ(messages.size(), 3);
  EXPECT_EQ(messages.at(0).first, true);
  EXPECT_EQ(messages.at(1).first, true);
  EXPECT_EQ(messages.at(2).first, true);
  EXPECT_EQ(messages.at(0).second.values_size(), 4);
  EXPECT_EQ(messages.at(1).second.values_size(), 3);
  EXPECT_EQ(messages.at(2).second.values_size(), 3);
}

TEST_F(TestWorker, RoundRobinSlicer) {
  Worker worker;
  worker.AddEmbeddingTable(1, 10);


  Worker::SlicedKVMessages messages;
  KVMessage send;
  send.add_keys(1);
  send.add_keys(2);
  send.add_keys(8);
  send.add_keys(8);
  send.add_keys(8);
  send.add_keys(8);
  send.add_keys(8);
  send.add_keys(8);
  send.add_keys(8);
  send.add_keys(8);
  send.add_values(1);
  send.add_values(2);
  send.add_values(8);
  send.add_values(8);
  send.add_values(8);
  send.add_values(8);
  send.add_values(8);
  send.add_values(8);
  send.add_values(8);
  send.add_values(8);
  worker.WorkerInitEmbeddingSlicer(send, &messages, {});

  std::vector<uint32_t> rank_ids;
  std::vector<std::string> data;
  for (size_t i = 0; i < messages.size(); i++) {
    if (messages.at(i).first) {
      rank_ids.emplace_back(i);
      data.emplace_back(messages.at(i).second.SerializeAsString());
    }
  }
  EXPECT_EQ(messages.size(), 3);
  EXPECT_EQ(messages.at(0).first, true);
  EXPECT_EQ(messages.at(1).first, true);
  EXPECT_EQ(messages.at(2).first, true);
  EXPECT_EQ(messages.at(0).second.values_size(), 4);
  EXPECT_EQ(messages.at(1).second.values_size(), 3);
  EXPECT_EQ(messages.at(2).second.values_size(), 3);
}
}  // namespace ps
}  // namespace mindspore
