//
// Created by cds on 2020/10/29.
//

#include "ps/core/node.h"
#include "comm_util.h"
#include "utils/ms_utils.h"

namespace mindspore {
namespace ps {
namespace core {

void Node::Start() {}

void Node::Stop() {}

void Node::StartHeartBeatTimer(const std::shared_ptr<TcpClient> &client) {
  // 做一些判断，比如rank_id有没有拿到8
  client->SendMessageWithTimer();
}

void Node::ProcessHeartbeat(const TcpClient &client, const CommMessage &message) {
  node_id_ = message.pb_meta().node_id();
  is_system_ready_ = message.pb_meta().is_system_ready();
  is_system_synchronized_ = message.pb_meta().is_system_synchronized();
  if (is_system_ready_ && !is_system_synchronized_) {
    MessageMeta message_meta;
    CommMessage comm_message;
    message_meta.set_cmd(ClusterCommand::FETCH_SERVERS);
    comm_message.set_allocated_pb_meta(&message_meta);
    client.SendMessage(comm_message);
  }
}

void Node::ProcessFetchServers(const CommMessage &message) {}

void Node::ProcessRegister(const CommMessage &message) { node_id_ = message.pb_meta().node_id(); }

uint32_t Node::NodeId() { return node_id_; }

void ClientNode::Start() {
  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint32_t scheduler_port = ClusterConfig::scheduler_port();
  client_to_scheduler_ = std::make_shared<TcpClient>(scheduler_host, scheduler_port);
  client_to_scheduler_->SetMessageCallback([&](const TcpClient &client, const CommMessage &message) {
    if (message.pb_meta().cmd() == ClusterCommand::HEARTBEAT) {
      ProcessHeartbeat(client, message);
    } else if (message.pb_meta().cmd() == ClusterCommand::FETCH_SERVERS) {
      ProcessFetchServers(message);
    } else if (message.pb_meta().cmd() == ClusterCommand::REGISTER) {
      ProcessRegister(message);
    }
  });
  client_to_scheduler_->Init();
  client_to_scheduler_->StartWithNoBlock();
  RegisterClient(client_to_scheduler_, Role::WORKER);

  StartHeartBeatTimer(client_to_scheduler_);
}

void ClientNode::RegisterClient(const std::shared_ptr<TcpClient> &client, const Role &role) {
  MessageMeta message_meta;
  CommMessage comm_message;
  message_meta.set_cmd(ClusterCommand::REGISTER);
  message_meta.set_role(role);
  message_meta.set_node_id(node_id_);
  comm_message.set_allocated_pb_meta(&message_meta);
  client->SendMessage(comm_message);
}

void ClientNode::Stop() { client_to_scheduler_->Stop(); }

void ServerNode::Start() {
  std::string interface;
  std::string server_ip;
  CommUtil::GetAvailableInterfaceAndIP(&interface, &server_ip);
  server_ = std::make_shared<TcpServer>(server_ip, 0);
  server_->SetMessageCallback([&](const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {});
  server_->Init();
  server_->StartWithNoBlock();

  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint32_t scheduler_port = ClusterConfig::scheduler_port();
  client_to_scheduler_ = std::make_unique<TcpClient>(scheduler_host, scheduler_port);
  client_to_scheduler_->SetMessageCallback([](const TcpClient &client, const CommMessage &message) {
    if (message.pb_meta().cmd() == ClusterCommand::HEARTBEAT) {
    }
  });
  client_to_scheduler_->Init();
  client_to_scheduler_->StartWithNoBlock();

  RegisterServer(client_to_scheduler_, server_ip, server_->BoundPort(), Role::SERVER);
  StartHeartBeatTimer(client_to_scheduler_);
}

void ServerNode::RegisterServer(const std::shared_ptr<TcpClient> &client, const std::string &host, const uint32_t &port,
                                const Role &role) {
  MessageMeta message_meta;
  CommMessage comm_message;
  message_meta.set_cmd(ClusterCommand::REGISTER);
  message_meta.set_hostname(host);
  message_meta.set_port(port);
  message_meta.set_role(role);
  comm_message.set_allocated_pb_meta(&message_meta);
  client->SendMessage(comm_message);
}

void ServerNode::Stop() {
  client_to_scheduler_->Stop();
  server_->Stop();
}

void SchedulerNode::Start() {
  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint32_t scheduler_port = ClusterConfig::scheduler_port();

  server_ = std::make_unique<TcpServer>(scheduler_host, scheduler_port);
  server_->SetMessageCallback([&](const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
    if (message.pb_meta().cmd() == ClusterCommand::HEARTBEAT) {
      ProcessHeartBeat(server, conn, message);
    } else if (message.pb_meta().cmd() == ClusterCommand::REGISTER) {
      ProcessRegister(server, conn, message);
    }
  });

  server_->Init();

  server_->StartWithNoBlock();
}

void SchedulerNode::ProcessHeartBeat(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
  //  std::string ip = message.pb_meta().hostname();
  //  uint32_t port = message.pb_meta().port();
  //  std::string node_host_ip_and_port = ip + ":" + std::to_string(port);
  //  if (message.pb_meta().role() == Role::SERVER) {
  //    servers_.insert(node_host_ip_and_port);
  //  } else if (message.pb_meta().role() == Role::WORKER) {
  //    workers_.insert(node_host_ip_and_port);
  //  }

  int32_t expected_node_num = ClusterConfig::server_num() + ClusterConfig::worker_num();
  int32_t actual_node_num = servers_.size() + workers_.size();
  if (expected_node_num == actual_node_num) {
    is_system_ready_ = true;
  } else {
    is_system_ready_ = false;
  }

  MessageMeta message_meta;
  message_meta.set_cmd(ClusterCommand::HEARTBEAT);
  message_meta.set_role(Role::SCHEDULER);
  message_meta.set_is_system_ready(is_system_ready_);
  message_meta.set_is_system_synchronized(is_system_synchronized_);

  //  PBHeartBeatMessage heartbeat_message;
  //  *heartbeat_message.mutable_server_host() = {servers_.begin(), servers_.end()};
  //  *heartbeat_message.mutable_worker_host() = {workers_.begin(), workers_.end()};

  CommMessage comm_message;
  //  comm_message.set_data(heartbeat_message.SerializeAsString());
  comm_message.set_allocated_pb_meta(&message_meta);
  server.SendMessage(conn, comm_message);
}

void SchedulerNode::ProcessRegister(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
  std::string ip = message.pb_meta().hostname();
  uint32_t port = message.pb_meta().port();
  std::string node_host_ip_and_port = ip + ":" + std::to_string(port);
  if (message.pb_meta().role() == Role::SERVER) {
    servers_.insert(node_host_ip_and_port);
  } else if (message.pb_meta().role() == Role::WORKER) {
    workers_.insert(node_host_ip_and_port);
  }

  // 分配rank_id
  MessageMeta message_meta;
  CommMessage comm_message;
  message_meta.set_cmd(ClusterCommand::REGISTER);
  message_meta.set_role(Role::SCHEDULER);
  message_meta.set_node_id(1);

  comm_message.set_allocated_pb_meta(&message_meta);
  server.SendMessage(conn, comm_message);
}

void SchedulerNode::Stop() {
  // 发送terminal的指令
  server_->Stop();
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore
