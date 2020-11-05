//
// Created by cds on 2020/10/29.
//

#include "ps/comm/node.h"
#include "ms_utils.h"

namespace mindspore {
namespace ps {
namespace comm {

void Node::Start() {}

void Node::Stop() {}

void ServerNode::Start() {

  auto scheduler_host = common::GetEnv("MS_SCHED_HOST");
  if (scheduler_host.empty()) {
    MS_LOG(EXCEPTION) << "The MS_SCHED_HOST should not be null!";
  }

  auto port = common::GetEnv("MS_SCHED_PORT");
  if (port.empty()) {
    MS_LOG(EXCEPTION) << "The MS_SCHED_PORT should not be null!";
  }
  int32_t scheduler_port = strtol(port.c_str(), nullptr, 10);
  TcpServer server(scheduler_host, scheduler_port);
  server.Init();
  server.SetMessageCallback([&](const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {

    if (message.pb_meta().cmd() == Command::HEARTBEAT) {
      std::string ip = message.pb_meta().hostname();
      uint32_t port = message.pb_meta().port();
      std::string node_host_ip_and_port = ip + ":" + std::to_string(port);
      if (message.pb_meta().role() == Role::SERVER) {
        servers_.insert(node_host_ip_and_port);
      } else if (message.pb_meta().role() == Role::WORKER) {
        workers_.insert(node_host_ip_and_port);
      }

      MessageMeta message_meta;
      CommMessage comm_message;
      message_meta.set_cmd(Command::ACK);
      message_meta.set_hostname(scheduler_host);
      message_meta.set_port(scheduler_port);
      message_meta.set_role(Role::SCHEDULER);
      PBHeartBeatMessage heartbeat_message;
      *heartbeat_message.mutable_server_host() = {servers_.begin(), servers_.end()};
      *heartbeat_message.mutable_worker_host() = {workers_.begin(), workers_.end()};
      comm_message.set_data(heartbeat_message.SerializeAsString());
      comm_message.set_allocated_pb_meta(&message_meta);
      server.SendMessage(conn, comm_message);
    }
  });

  server.Init();

  server.StartWithNoBlock();
}

void SchedulerNode::Start() {
  auto scheduler_host = common::GetEnv("MS_SCHED_HOST");
  if (scheduler_host.empty()) {
    MS_LOG(EXCEPTION) << "The MS_SCHED_HOST should not be null!";
  }

  auto port = common::GetEnv("MS_SCHED_PORT");
  if (port.empty()) {
    MS_LOG(EXCEPTION) << "The MS_SCHED_PORT should not be null!";
  }
  int32_t scheduler_port = strtol(port.c_str(), nullptr, 10);
  TcpServer server(scheduler_host, scheduler_port);
  server.Init();
  server.SetMessageCallback([&](const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {

    if (message.pb_meta().cmd() == Command::HEARTBEAT) {
      std::string ip = message.pb_meta().hostname();
      uint32_t port = message.pb_meta().port();
      std::string node_host_ip_and_port = ip + ":" + std::to_string(port);
      if (message.pb_meta().role() == Role::SERVER) {
        servers_.insert(node_host_ip_and_port);
      } else if (message.pb_meta().role() == Role::WORKER) {
        workers_.insert(node_host_ip_and_port);
      }

      MessageMeta message_meta;
      CommMessage comm_message;
      message_meta.set_cmd(Command::ACK);
      message_meta.set_hostname(scheduler_host);
      message_meta.set_port(scheduler_port);
      message_meta.set_role(Role::SCHEDULER);
      PBHeartBeatMessage heartbeat_message;
      *heartbeat_message.mutable_server_host() = {servers_.begin(), servers_.end()};
      *heartbeat_message.mutable_worker_host() = {workers_.begin(), workers_.end()};
      comm_message.set_data(heartbeat_message.SerializeAsString());
      comm_message.set_allocated_pb_meta(&message_meta);
      server.SendMessage(conn, comm_message);
    }
  });

  server.Init();

  server.StartWithNoBlock();
}

void SchedulerNode::Stop() {}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore
