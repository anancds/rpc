//
// Created by cds on 2020/12/8.
//
#include <iostream>
#include "util.h"
using namespace mindspore::ps;
int main(int /*argc*/, char** /*argv*/) {
std::cout << Util::LocalShard(3, 0, 2) << std::endl;
}