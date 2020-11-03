////
//// Created by cds on 2020/10/29.
////
//
//#include "node.h"
//#include "ms_utils.h"
//
//namespace mindspore {
//namespace ps {
//namespace comm {
//
//void Node::Start() {}
//
//void Node::Stop() {}
//
//void SchedulerNode::Start() {
//  auto scheduler_host = common::GetEnv("MS_SCHED_HOST");
//  if (scheduler_host.empty()) {
//    MS_LOG(EXCEPTION) << "The MS_SCHED_HOST should not be null!";
//  }
//
//  auto scheduler_port = common::GetEnv("MS_SCHED_PORT");
//  if (scheduler_port.empty()) {
//    MS_LOG(EXCEPTION) << "The MS_SCHED_PORT should not be null!";
//  }
//
//  TcpKVServer server(scheduler_host, atoi(scheduler_port.c_str()));
//  server.InitServer();
//  PBMeta pbMeta;
//  PBMessage pbMessage;
//  std::vector<int> ints{1, 2};
//  *pbMessage.mutable_pb_kv_message()->mutable_keys() = {ints.begin(), ints.end()};
//
//  server.ReceiveKVMessage([](const TcpKVServer &server, const TcpKVConnection &conn, const PBMessage &message) {
//    MS_LOG(INFO) << "The server message size is:" << message.pb_kv_message().keys_size();
//    if (message.pb_meta().cmd() == Command::HEARTBEAT) {
//
//      std::string ip = message.pb_meta().hostname();
//      uint32_t port =
//    }
//    server.SendKVMessage(conn, message);
//  });
//
//  server.Start();
//}
//
//void SchedulerNode::Stop() {}
//}  // namespace comm
//}  // namespace ps
//}  // namespace mindspore