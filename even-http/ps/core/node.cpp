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

void Node::StartHeartBeatTimer(TcpClient &client) {
  // 做一些判断，比如rank_id有没有拿到8
  client.SendMessageWithTimer();
}

void Node::ProcessAck(const TcpClient &client, const CommMessage &message) {
  node_id_ = message.pb_meta().node_id();
  is_system_ready_ = message.pb_meta().is_system_ready();
  is_system_synchronized_ = message.pb_meta().is_system_synchronized();
  if (is_system_ready_ && !is_system_synchronized_) {
    MessageMeta message_meta;
    CommMessage comm_message;
    message_meta.set_cmd(Command::NODE);
    comm_message.set_allocated_pb_meta(&message_meta);
    client.SendMessage(comm_message);
  }
}

void Node::ProcessNode(const CommMessage &message) {}

void ClientNode::Start() {
  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint32_t scheduler_port = ClusterConfig::scheduler_port();
  TcpClient client(scheduler_host, scheduler_port);
  client.SetMessageCallback([&](const TcpClient &client, const CommMessage &message) {
    if (message.pb_meta().cmd() == Command::ACK) {
      ProcessAck(client, message);
    } else if (message.pb_meta().cmd() == Command::NODE) {
      ProcessNode(message);
    }
  });
  client.Init();
  client.StartWithNoBlock();
  RegisterClient(client, Role::WORKER);

  StartHeartBeatTimer(client);
}

void ClientNode::RegisterClient(const TcpClient &client, const Role &role) {
  MessageMeta message_meta;
  CommMessage comm_message;
  message_meta.set_cmd(Command::REGISTER);
  message_meta.set_role(role);
  comm_message.set_allocated_pb_meta(&message_meta);
  client.SendMessage(comm_message);
}

void ClientNode::Stop() {}

void ServerNode::Start() {
  std::string interface;
  std::string server_ip;
  CommUtil::GetAvailableInterfaceAndIP(&interface, &server_ip);
  //  int32_t server_port = CommUtil::GetAvailablePort();
  TcpServer server(server_ip, 0);
  server.Init();
  server.SetMessageCallback([&](const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {});
  server.Init();
  server.StartWithNoBlock();

  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint32_t scheduler_port = ClusterConfig::scheduler_port();
  TcpClient client(scheduler_host, scheduler_port);
  client.SetMessageCallback([](const TcpClient &client, const CommMessage &message) {
    if (message.pb_meta().cmd() == Command::HEARTBEAT) {
    }
  });
  client.Init();
  client.StartWithNoBlock();

  RegisterServer(client, server_ip, server.BoundPort(), Role::SERVER);
  StartHeartBeatTimer(client);
}

void ServerNode::RegisterServer(const TcpClient &client, const std::string &host, const uint32_t &port,
                                const Role &role) {
  MessageMeta message_meta;
  CommMessage comm_message;
  message_meta.set_cmd(Command::REGISTER);
  message_meta.set_hostname(host);
  message_meta.set_port(port);
  message_meta.set_role(role);
  comm_message.set_allocated_pb_meta(&message_meta);
  client.SendMessage(comm_message);
}

void ServerNode::Stop() {}

void SchedulerNode::Start() {
  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint32_t scheduler_port = ClusterConfig::scheduler_port();

  server_ = std::make_unique<TcpServer>(scheduler_host, scheduler_port);
  server_->SetMessageCallback([&](const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
    if (message.pb_meta().cmd() == Command::HEARTBEAT) {
      ProcessHeartBeat(server, conn, message);
    } else if (message.pb_meta().cmd() == Command::REGISTER) {
      ProcessRegister(server, conn, message);
    }
  });

  server_->Init();

  server_->StartWithNoBlock();
}

void SchedulerNode::ProcessHeartBeat(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
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
  message_meta.set_role(Role::SCHEDULER);
  PBHeartBeatMessage heartbeat_message;
  *heartbeat_message.mutable_server_host() = {servers_.begin(), servers_.end()};
  *heartbeat_message.mutable_worker_host() = {workers_.begin(), workers_.end()};
  comm_message.set_data(heartbeat_message.SerializeAsString());
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
  message_meta.set_cmd(Command::ACK);
  message_meta.set_role(Role::SCHEDULER);

  //  comm_message.set_data(heartbeat_message.SerializeAsString());
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
