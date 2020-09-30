//
// Created by cds on 2020/9/29.
//
#include "event_http_server.h"
#include <thread>

using namespace Network;

void StartHttpServer() {
  EvHttpServ Serv("0.0.0.0", 8077);
  Serv.RegistHandler("/hello", [](EvHttpResp *resp) { resp->QuickResponse(200, "Hello World!\n"); });
  Serv.Start();
}
int main()
{
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