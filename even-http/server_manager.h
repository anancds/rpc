//
// Created by cds on 2020/10/24.
//

#ifndef RPC_SERVER_MANAGER_H
#define RPC_SERVER_MANAGER_H

#include <iostream>

class server_manager {
 public:
  void GetNodeIps();
  void Start();
  void Stop();
  void Connect();
  void Connect(std::string &);
};

#endif  // RPC_SERVER_MANAGER_H
