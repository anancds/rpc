//
// Created by cds on 2020/9/29.
//
#include "event_http_server.h"

using namespace Network;

int main()
{
  EvHttpServ Serv("0.0.0.0", 8077);

  Serv.RegistHandler("/hi/testget",  [](EvHttpResp *resp){
    resp->QuickResponse(200,"Hello World!\n");});

  Serv.Start();
  return 0;
}