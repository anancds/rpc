//
// Created by cds on 2020/11/9.
//

#include <iostream>
#include "PersonBuilder.h"
#include "person.h"
using namespace std;
int main() {
  Config::create();
  cout << Config::GetInstance()->GetHeartbeatInterval() << endl;
  Config::create().WithHeartbeatInterval(100);
  cout << Config::GetInstance()->GetHeartbeatInterval() << endl;
  return EXIT_SUCCESS;
}