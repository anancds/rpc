//
// Created by cds on 2020/10/29.
//

#include "node.h"
#include "ms_utils.h"

namespace mindspore {
namespace ps {
namespace comm {

void SchedulerNode::Start() {
  auto scheduler_host = common::GetEnv("MS_SCHED_HOST");
  if (scheduler_host.empty()) {
    MS_LOG(EXCEPTION) << "The MS_SCHED_HOST should not be null!";
  }

  auto scheduler_port = common::GetEnv("MS_SCHED_PORT");
  if (scheduler_port.empty()) {
    MS_LOG(EXCEPTION) << "The MS_SCHED_PORT should not be null!";
  }

  TcpKVServer server(scheduler_host, atoi(scheduler_port.c_str()));
  server.InitServer();

  server.Start();
}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
