//
// Created by cds on 2020/11/9.
//
#include "person.h"

#include <iostream>
#include "PersonBuilder.h"

ConfigBuilder Config::create() { return ConfigBuilder(); }

std::string Config::GetRole() { return role_; }

int32_t Config::GetNumWorkers() { return num_workers_; }

int32_t Config::GetNumServers() { return num_servers_; }

int32_t Config::GetHeartbeatInterval() { return heartbeat_interval_; }

std::string Config::GetSchedulerHost() { return scheduler_host_; }

int32_t Config::GetSchedulerPort() { return scheduler_port_; }
