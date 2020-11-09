//
// Created by cds on 2020/11/9.
//

#ifndef RPC_PERSONBUILDER_H
#define RPC_PERSONBUILDER_H
#include "person.h"
class ConfigBuilder {
 public:
  ConfigBuilder() = default;
  virtual ~ConfigBuilder() = default;
  ConfigBuilder &WithRole(const std::string &role);
  ConfigBuilder &WithNumWorkers(const int32_t &num_workers);
  ConfigBuilder &WithNumServers(const int32_t &num_servers);
  ConfigBuilder &WithHeartbeatInterval(const int32_t &heartbeat_interval);
  ConfigBuilder &WithSchedulerHost(const std::string &scheduler_host);
  ConfigBuilder &WithSchedulerPort(const int32_t &port);
};
#endif  // RPC_PERSONBUILDER_H
