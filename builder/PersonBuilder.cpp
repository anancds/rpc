//
// Created by cds on 2020/11/9.
//

#include "PersonBuilder.h"

ConfigBuilder &ConfigBuilder::WithRole(const std::string &role) {
  Config::GetInstance()->role_ = role;
  return *this;
}

ConfigBuilder &ConfigBuilder::WithNumWorkers(const int32_t &num_workers) {
  Config::GetInstance()->num_workers_ = num_workers;
  return *this;
}

ConfigBuilder &ConfigBuilder::WithNumServers(const int32_t &num_servers) {
  Config::GetInstance()->num_servers_ = num_servers;
  return *this;
}

ConfigBuilder &ConfigBuilder::WithHeartbeatInterval(const int32_t &heartbeat_interval) {
  Config::GetInstance()->heartbeat_interval_ = heartbeat_interval;
  return *this;
}

ConfigBuilder &ConfigBuilder::WithSchedulerHost(const std::string &scheduler_host) {
  Config::GetInstance()->scheduler_host_ = scheduler_host;
  return *this;
}

ConfigBuilder &ConfigBuilder::WithSchedulerPort(const int32_t &port) {
  Config::GetInstance()->scheduler_port_ = port;
  return *this;
}
