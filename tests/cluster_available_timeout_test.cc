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
#include "ps/core/node.h"
#include "ps/core/scheduler_node.h"

namespace mindspore {
namespace ps {
namespace core {
class TestClusterAvailableTimeout : public UT::Common {
 public:
  TestClusterAvailableTimeout() = default;
  ~TestClusterAvailableTimeout() override = default;

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(TestClusterAvailableTimeout, TestClusterAvailableTimeout) {
  ClusterConfig::set_cluster_available_timeout(3);
  SchedulerNode node;
  node.Start();
  node.Finish();
  node.Stop();
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore