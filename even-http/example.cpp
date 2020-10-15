//
// Created by cds on 2020/9/29.
//
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <thread>
#include "event_http_server.h"
#include <sys/syscall.h>
#include <thread>

using namespace Network;
using namespace std;
int i = 0;
static void testGetHandler(EvHttpResp *resp) {
  std::string strHost = resp->GetRequestHost();
  int nPort = resp->GetUriPort();
  std::string strPath = resp->GetUriPath();
  std::string strQuery = resp->GetUriQuery();
  std::cout << "Get host:" << strHost << std::endl;
  std::cout << "Get port:" << nPort << std::endl;
  std::cout << "Get uri:" << strPath << std::endl;
  std::cout << "Get uri params:" << strQuery << std::endl;

  const std::string gKey("key1");
  const std::string hKey("header");
  const std::string pKey("hello");
  std::string strGVal = resp->GetPathParam(gKey);
  std::string strHVal = resp->GetHeadParam(hKey);
  std::string strPVal = resp->GetPostParam(pKey);
  std::string post_message = resp->GetPostMsg();
  std::cout << "Get path param:" << strGVal << std::endl;
  std::cout << "Get head param:" << strHVal << std::endl;
  std::cout << "Get post param:" << strPVal << std::endl;
  std::cout << "Get post message:" << post_message << std::endl;

  std::string strBody = resp->GetPostMsg();
i++;
  const std::string rKey("retHead");
  const std::string rVal("retValue");
  const std::string rBody("Hello World!" + strQuery + "\n");
//  const std::string rBody("Hello World!");
  resp->AddRespHeadParam(rKey, rVal);
  resp->AddRespString(rBody);
//    const std::string rBuf("Hello World is over!\n");
//    resp->AddRespBuf(rBuf.c_str(), rBuf.length());

  resp->SetRespCode(200);
  resp->SendResponse();
}
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
  Serv.InitServer();
  Serv.RegistHandler("/hello", [](EvHttpResp *resp) {
    std::cout << "id:" << std::this_thread::get_id << std::endl;

    sleep(1);
    resp->QuickResponse(200, "Hello Worlds!\n");
  });

  Serv.RegistHandler("/handler", testGetHandler);
  Serv.Start();
}
int main() {
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