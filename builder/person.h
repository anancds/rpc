//
// Created by cds on 2020/11/9.
//

#ifndef RPC_PERSON_H
#define RPC_PERSON_H
#include <iostream>
using namespace std;

class ConfigBuilder;
class Config {
 public:
  friend class ConfigBuilder;
  static ConfigBuilder create();
  static Config *GetInstance() {
    static Config e;
    return &e;
  }

  Config(const Config &) = delete;
  Config &operator=(const Config &) = delete;
  virtual ~Config() = default;

  std::string GetRole();
  int32_t GetNumWorkers();
  int32_t GetNumServers();
  int32_t GetHeartbeatInterval();
  std::string GetSchedulerHost();
  int32_t GetSchedulerPort();

 protected:
  Config() = default;

 private:
  std::string role_;
  int32_t num_workers_{};
  int32_t num_servers_{};
  int32_t heartbeat_interval_{5};
  std::string scheduler_host_{};
  int32_t scheduler_port_{};

};
#endif  // RPC_PERSON_H
