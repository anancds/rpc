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

#include "ps/core/node.h"
#include "comm_util.h"
#include "utils/ms_utils.h"

namespace mindspore {
namespace ps {
namespace core {

void Node::Heartbeat(const std::shared_ptr<TcpClient> &client) {
  std::thread heartbeat_thread([&]() {
    MS_LOG(INFO) << "The node: " << node_id_ << " begin send heartbeat to the scheduler!";
    const uint32_t heartbeat_timer = ClusterConfig::heartbeat_interval();
    if (heartbeat_timer == 0) {
      MS_LOG(EXCEPTION) << "The heartbeat interval should not be 0!";
    }
    while (is_system_ready_) {
      MessageMeta meta;
      meta.set_cmd(ClusterCommand::HEARTBEAT);
      meta.set_node_id(client->NodeId());
      CommMessage message;
      *message.mutable_pb_meta() = {meta};
      client->SendMessage(message);
      std::this_thread::sleep_for(std::chrono::seconds(heartbeat_timer));
    }
  });
  heartbeat_thread.detach();
}

void Node::ProcessHeartbeat() {
  struct timeval current_time {};
  (void)gettimeofday(&current_time, nullptr);
  UpdateHeartbeat(kSchedulerNodeId, current_time);
}

void Node::UpdateHeartbeat(const uint32_t &node_id, const timeval &time) {
  std::lock_guard<std::mutex> lock(heartbeat_mutex_);

  heartbeats_[node_id] = time;
  MS_LOG(INFO) << "The node id is: " << node_id << " the current time is: " << time.tv_sec;
}

uint32_t Node::NodeId() const { return node_id_; }

int Node::RankId() const { return CommUtil::IDtoRank(node_id_); }

void ClientNode::Start() {
  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint32_t scheduler_port = ClusterConfig::scheduler_port();
  client_to_scheduler_ = std::make_shared<TcpClient>(scheduler_host, scheduler_port);
  client_to_scheduler_->SetMessageCallback([&](const TcpClient &client, const CommMessage &message) {
    if (message.pb_meta().cmd() == ClusterCommand::HEARTBEAT) {
      ProcessHeartbeat();
    } else if (message.pb_meta().cmd() == ClusterCommand::TERMINATE) {
      ProcessTerminal(message);
    } else if (message.pb_meta().cmd() == ClusterCommand::REGISTER) {
      ProcessRegister(message);
    }
  });
  client_to_scheduler_->Init();

  Register(client_to_scheduler_, NodeRole::WORKER);
  client_to_scheduler_->Start();
  while (!is_system_ready_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  MS_LOG(INFO) << "The system is ready to use!";
  client_to_scheduler_->SetNodeId(node_id_);
  Heartbeat(client_to_scheduler_);
}

void ClientNode::Register(const std::shared_ptr<TcpClient> &client, const NodeRole &role) {
  struct timeval current_time {};
  (void)gettimeofday(&current_time, nullptr);
  UpdateHeartbeat(kSchedulerNodeId, current_time);
  MessageMeta message_meta;
  CommMessage comm_message;
  message_meta.set_cmd(ClusterCommand::REGISTER);
  message_meta.set_role(role);
  message_meta.set_node_id(node_id_);
  *comm_message.mutable_pb_meta() = {message_meta};
  client->SendMessage(comm_message);
}

void ClientNode::ProcessRegister(const CommMessage &message) {
  RegisterMessage register_message;
  register_message.ParseFromString(message.data());
  node_id_ = register_message.node_id();
  MS_LOG(INFO) << "The client node id is:" << node_id_;

  is_system_ready_ = message.pb_meta().is_system_ready();

  for (auto it = register_message.register_message_meta().begin(); it != register_message.register_message_meta().end();
       ++it) {
    server_node_ids_[it->node_id()] = it->server_host();
  }
  MS_LOG(INFO) << "The all server host size is:" << server_node_ids_.size();
}

void ClientNode::ProcessTerminal(const CommMessage &message) {
  MS_LOG(INFO) << "The client node id:" << node_id_ << " is process terminal message!";
  client_to_scheduler_->Stop();
  client_to_scheduler_->StopEventBase();
}

void ClientNode::Stop() {
  is_system_ready_ = false;
  client_to_scheduler_->Stop();
  client_to_scheduler_->StopEventBase();
}

void ServerNode::Start() {
  std::string interface;
  std::string server_ip;
  CommUtil::GetAvailableInterfaceAndIP(&interface, &server_ip);
  server_ = std::make_shared<TcpServer>(server_ip, 0);
  server_->SetMessageCallback([&](const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {});
  server_->Init();
  server_->Start();

  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint32_t scheduler_port = ClusterConfig::scheduler_port();
  client_to_scheduler_ = std::make_unique<TcpClient>(scheduler_host, scheduler_port);
  client_to_scheduler_->SetMessageCallback([&](const TcpClient &client, const CommMessage &message) {
    if (message.pb_meta().cmd() == ClusterCommand::TERMINATE) {
      ProcessTerminal(message);
    } else if (message.pb_meta().cmd() == ClusterCommand::REGISTER) {
      ProcessRegister(message);
    } else if (message.pb_meta().cmd() == ClusterCommand::HEARTBEAT) {
      ProcessHeartbeat();
    }
  });
  client_to_scheduler_->Init();
  client_to_scheduler_->Start();

  Register(client_to_scheduler_, server_ip, server_->BoundPort(), NodeRole::SERVER);
  while (!is_system_ready_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  MS_LOG(INFO) << "The system is ready to use!";
  client_to_scheduler_->SetNodeId(node_id_);
  Heartbeat(client_to_scheduler_);
}

void ServerNode::Register(const std::shared_ptr<TcpClient> &client, const std::string &host, const uint32_t &port,
                          const NodeRole &role) {
  struct timeval current_time {};
  (void)gettimeofday(&current_time, nullptr);
  UpdateHeartbeat(kSchedulerNodeId, current_time);
  MessageMeta message_meta;
  CommMessage comm_message;
  message_meta.set_cmd(ClusterCommand::REGISTER);
  message_meta.set_hostname(host);
  message_meta.set_port(port);
  message_meta.set_role(role);
  message_meta.set_node_id(node_id_);
  *comm_message.mutable_pb_meta() = {message_meta};
  client->SendMessage(comm_message);
}

void ServerNode::ProcessRegister(const CommMessage &message) {
  RegisterMessage register_message;
  register_message.ParseFromString(message.data());
  node_id_ = register_message.node_id();
  MS_LOG(INFO) << "The server node id is:" << node_id_;

  is_system_ready_ = message.pb_meta().is_system_ready();

  for (auto it = register_message.register_message_meta().begin(); it != register_message.register_message_meta().end();
       ++it) {
    server_node_ids_[it->node_id()] = it->server_host();
  }
  MS_LOG(INFO) << "The all server host size is:" << server_node_ids_.size();
}

void ServerNode::ProcessTerminal(const CommMessage &message) {
  MS_LOG(INFO) << "The server node id:" << node_id_ << " is process terminal message!";
  server_->Stop();
  client_to_scheduler_->Stop();
  client_to_scheduler_->StopEventBase();
}

void ServerNode::Stop() {
  server_->Stop();
  client_to_scheduler_->Stop();
  client_to_scheduler_->StopEventBase();
}

void SchedulerNode::Start() {
  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint32_t scheduler_port = ClusterConfig::scheduler_port();

  server_ = std::make_unique<TcpServer>(scheduler_host, scheduler_port);
  server_->SetMessageCallback([&](const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
    if (message.pb_meta().cmd() == ClusterCommand::HEARTBEAT) {
      ProcessHeartBeat(server, conn, message);
    } else if (message.pb_meta().cmd() == ClusterCommand::REGISTER) {
      MS_LOG(INFO) << "The scheduler receive a register message!";
      ProcessRegister(server, conn, message);
    }
  });

  server_->Init();

  server_->Start();

  while (!is_system_ready_ || server_->ConnectionNum() != 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  MS_LOG(INFO) << "The scheduler is ready to quit!";
}

void SchedulerNode::HeartBeat(const TcpServer &server, const TcpConnection &conn) {
  MessageMeta message_meta;
  message_meta.set_cmd(ClusterCommand::HEARTBEAT);
  message_meta.set_role(NodeRole::SCHEDULER);
  message_meta.set_hostname(ClusterConfig::scheduler_host());
  message_meta.set_port(ClusterConfig::scheduler_port());
  message_meta.set_node_id(kSchedulerNodeId);

  CommMessage comm_message;
  *comm_message.mutable_pb_meta() = {message_meta};
  server.SendMessage(conn, comm_message);
}

void SchedulerNode::ProcessHeartBeat(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
  const MessageMeta &message_meta = message.pb_meta();
  struct timeval current_time {};
  (void)gettimeofday(&current_time, nullptr);
  UpdateHeartbeat(message_meta.node_id(), current_time);

  HeartBeat(server, conn);

  for (auto it = heartbeats_.begin(); it != heartbeats_.end(); ++it) {
    if (it->second.tv_sec + ClusterConfig::heartbeat_interval() < current_time.tv_sec) {
      MS_LOG(ERROR) << "There is a node failed, should terminal the whole system!";
      Terminal(server);
    }
  }
}

void SchedulerNode::ProcessRegister(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
  if (message.pb_meta().role() == NodeRole::SERVER) {
    const_cast<TcpConnection &>(conn).SetRole(NodeRole::SERVER);

    std::string ip = message.pb_meta().hostname();
    uint32_t port = message.pb_meta().port();
    std::string node_host_ip_and_port = ip + ":" + std::to_string(port);
    const_cast<TcpConnection &>(conn).SetNodeHost(node_host_ip_and_port);

  } else if (message.pb_meta().role() == NodeRole::WORKER) {
    const_cast<TcpConnection &>(conn).SetRole(NodeRole::WORKER);
  }

  uint32_t total_node_num = ClusterConfig::server_num() + ClusterConfig::worker_num();
  if (total_node_num == server.ConnectionNum()) {
    MS_LOG(INFO) << "All worker nodes and server nodes have registered to scheduler!";

    const std::map<evutil_socket_t, const TcpConnection *> &connections = server.Connections();
    uint32_t node_id = 0;

    MessageMeta message_meta;
    message_meta.set_cmd(ClusterCommand::REGISTER);
    message_meta.set_role(NodeRole::SCHEDULER);
    message_meta.set_hostname(ClusterConfig::scheduler_host());
    message_meta.set_port(ClusterConfig::scheduler_port());
    message_meta.set_node_id(kSchedulerNodeId);
    is_system_ready_ = true;
    message_meta.set_is_system_ready(is_system_ready_);

    RegisterMessage register_message;
    std::vector<RegisterMessageMeta> register_message_meta_list;

    // assign worker node and server node rank id
    for (auto it = connections.begin(); it != connections.end(); ++it) {
      if (it->second->Role() == NodeRole::SERVER) {
        node_id = CommUtil::ServerRankToID(++current_server_rank_id_);
        const_cast<TcpConnection *>(it->second)->SetNodeId(node_id);
        RegisterMessageMeta register_message_meta;
        register_message_meta.set_node_id(node_id);
        register_message_meta.set_server_host(conn.NodeHost());
        register_message_meta_list.push_back(register_message_meta);

        MS_LOG(INFO) << "The server node:" << conn.NodeHost() << " is registered to scheduler! "
                     << "The rank id is:" << current_server_rank_id_ << " and the node id is: " << node_id;
      } else if (it->second->Role() == NodeRole::WORKER) {
        node_id = CommUtil::WorkerRankToID(++current_worker_rank_id_);
        const_cast<TcpConnection *>(it->second)->SetNodeId(node_id);
        MS_LOG(INFO) << "The worker node is registered to scheduler! "
                     << "The rank id is:" << current_worker_rank_id_ << " and the node id is: " << node_id;
      }
    }

    *register_message.mutable_register_message_meta() = {register_message_meta_list.begin(),
                                                         register_message_meta_list.end()};

    // send register message to all worker nodes and server nodes;
    for (auto it = connections.begin(); it != connections.end(); ++it) {
      CommMessage comm_message;
      *comm_message.mutable_pb_meta() = {message_meta};
      register_message.set_node_id(it->second->NodeId());
      comm_message.set_data(register_message.SerializeAsString());
      struct timeval current_time {};
      (void)gettimeofday(&current_time, nullptr);
      UpdateHeartbeat(it->second->NodeId(), current_time);
      server.SendMessage(*it->second, comm_message);
    }
  }
}

void SchedulerNode::Terminal(const TcpServer &server) {
  MessageMeta message_meta;
  message_meta.set_cmd(ClusterCommand::TERMINATE);
  message_meta.set_role(NodeRole::SCHEDULER);
  message_meta.set_hostname(ClusterConfig::scheduler_host());
  message_meta.set_port(ClusterConfig::scheduler_port());
  message_meta.set_node_id(0);

  CommMessage comm_message;
  *comm_message.mutable_pb_meta() = {message_meta};
  const_cast<TcpServer &>(server).SendMessage(comm_message);
}

void SchedulerNode::Stop() { server_->Stop(); }
}  // namespace core
}  // namespace ps
}  // namespace mindspore
