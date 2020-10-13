//
// Created by cds on 2020/9/29.
//
#include "event_http_server.h"
#include <thread>
#include <iostream>
#include <regex>

using namespace Network;
using namespace std;
bool CheckIp(const std::string &ip) {
  std::regex pattern("((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?).){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)");
  std::smatch res;
  if (regex_match(ip, res, pattern)) {
    return true;
  }
  return false;
}
void StartHttpServer() {
  EvHttpServ Serv("0.0.0.0", 8077);
  Serv.RegistHandler("/hello", [](EvHttpResp *resp) { resp->QuickResponse(200, "Hello World!\n"); });
  Serv.Start();
}
int main()
{
  std::int16_t test = -1;
  std::cout << test << std::endl;
  cout << CheckIp("0.0.0.0") << endl;
  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  http_server_thread_.reset(new std::thread(&StartHttpServer));
  http_server_thread_->join();

//  EvHttpServ Serv("0.0.0.0", 8077);
//
//  Serv.RegistHandler("/hi/testget",  [](EvHttpResp *resp){
//    resp->QuickResponse(200,"Hello World!\n");});
//
//  Serv.Start();
  return 0;
}