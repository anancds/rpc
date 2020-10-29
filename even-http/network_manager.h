//
// Created by cds on 2020/10/29.
//

#ifndef RPC_NETWORK_MANAGER_H
#define RPC_NETWORK_MANAGER_H
#include <iostream>
#include <vector>

//相当于van
class NetWorkManager {
 public:
  void Start();
  void Stop();
  void SendMessage();
  void ReceiveMessage();
  std::vector<std::string> GetNodeIps();
  void RegisterToScheduler();
  void ConnectToServers();
  void Connect(const std::vector<std::string> &nodes);
  void Heartbeat();
};

#endif  // RPC_NETWORK_MANAGER_H
