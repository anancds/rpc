//
// Created by cds on 2020/11/18.
//
#include <iostream>;
#include <stdio.h>;
#include <uuid/uuid.h>;

static std::string generate()
{
//  char buf[64] = { 0 };
//
//  uuid_t uu;
//  uuid_generate( uu );
//
//  int32_t index = 0;
//  for (int32_t i = 0; i < 16; i++)
//  {
//    int32_t len = i < 15 ?
//                  sprintf(buf + index, "%02X-", uu[i]) :
//                  sprintf(buf + index, "%02X", uu[i]);
//    if(len < 0 )
//      return std::move(std::string(""));
//    index += len;
//  }
//
//  return std::move(std::string(buf));
  uuid_t uu;
  uuid_generate_time_safe(uu);

  char uuid_str[64];
  uuid_unparse_lower(uu, uuid_str);
  return uuid_str;
}

int main(int argc, char **argv) {
 std::cout << generate() << std::endl;
  return 0;
}
