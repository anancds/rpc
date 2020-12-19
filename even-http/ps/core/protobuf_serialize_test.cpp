//
// Created by cds on 2020/12/5.
//

#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"
using namespace mindspore::ps::core;

void SerializeAsString() {
  CommMessage comm_message;
  KVMessage kv_message;
  std::vector<int> keys(100000000, 1);
  std::vector<int> values(100000000, 2);
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};
  auto start = std::chrono::high_resolution_clock::now();
  std::string res = kv_message.SerializeAsString();
  auto end = std::chrono::high_resolution_clock::now();
  comm_message.set_data(res);
  auto end1 = std::chrono::high_resolution_clock::now();
  std::string res1 = comm_message.SerializeAsString();
  auto end2 = std::chrono::high_resolution_clock::now();
  std::cout << "KVMessage SerializeAsString, cost:" << (end - start).count() / 1e6 << "ms" << std::endl;
  std::cout << "set data, cost:" << (end1 - end).count() / 1e6 << "ms" << std::endl;
  std::cout << "CommMessage SerializeAsString, cost:" << (end2 - end1).count() / 1e6 << "ms" << std::endl;
}

void SerializeToArray() {
  CommMessage comm_message;
  KVMessage kv_message;
  std::vector<int> keys(100000000, 1);
  std::vector<int> values(100000000, 2);
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};
  size_t buff_size = kv_message.ByteSizeLong();
  char *buffer = new char[buff_size + 1];
  auto start = std::chrono::high_resolution_clock::now();
  kv_message.SerializeToArray(buffer, buff_size);
  auto end = std::chrono::high_resolution_clock::now();
  comm_message.set_data(buffer, buff_size);
  auto end1 = std::chrono::high_resolution_clock::now();
  size_t buff_size1 = comm_message.ByteSizeLong();
  char *buffer1 = new char[buff_size1 + 1];
  char *buffer2 = new char[buff_size1 + 1];
  auto end2 = std::chrono::high_resolution_clock::now();
  comm_message.SerializeToArray(buffer1, buff_size1);
  comm_message.SerializeToArray(buffer2, buff_size1);
  auto end3 = std::chrono::high_resolution_clock::now();
  std::cout << "KVMessage SerializeToArray, cost:" << (end - start).count() / 1e6 << "ms" << std::endl;
  std::cout << "set data, cost:" << (end1 - end).count() / 1e6 << "ms" << std::endl;
  std::cout << "CommMessage SerializeToArray, cost:" << (end3 - end2).count() / 1e6 << "ms" << std::endl;
}

void TestSerializeToArrayVerySmallSetData() {
  CommMessage comm_message;
  KVMessage kv_message;
  std::vector<int> keys(100000000, 1);
  std::vector<int> values(100000000, 2);
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};
  size_t buff_size = kv_message.ByteSizeLong();
  char *buffer = new char[buff_size + 1];
  auto start = std::chrono::high_resolution_clock::now();
  kv_message.SerializeToArray(buffer, buff_size);
  auto end = std::chrono::high_resolution_clock::now();
  kv_message.set_command(PSCommand::PUSH);
  auto end1 = std::chrono::high_resolution_clock::now();
  size_t buff_size1 = kv_message.ByteSizeLong();
  char *buffer1 = new char[buff_size1 + 1];
  auto end2 = std::chrono::high_resolution_clock::now();
  kv_message.SerializeToArray(buffer1, buff_size1);
  auto end3 = std::chrono::high_resolution_clock::now();
  std::cout << "KVMessage SerializeToArray, cost:" << (end - start).count() / 1e6 << "ms" << std::endl;
  std::cout << "small set command, cost:" << (end1 - end).count() / 1e6 << "ms" << std::endl;
  std::cout << "CommMessage SerializeToArray, cost:" << (end3 - end2).count() / 1e6 << "ms" << std::endl;
}

void TestSerializeToString() {
  KVMessage kv_message;
  std::vector<int> keys(100000000, 1);
  std::vector<int> values(100000000, 2);
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};
  auto start = std::chrono::high_resolution_clock::now();
  std::string res = kv_message.SerializeAsString();
  auto end = std::chrono::high_resolution_clock::now();
  kv_message.set_command(PSCommand::PUSH);
  auto end1 = std::chrono::high_resolution_clock::now();
  std::string res1 = kv_message.SerializeAsString();
  auto end2 = std::chrono::high_resolution_clock::now();
  std::cout << "KVMessage SerializeAsString, cost:" << (end - start).count() / 1e6 << "ms" << std::endl;
  std::cout << "small set data, cost:" << (end1 - end).count() / 1e6 << "ms" << std::endl;
  std::cout << "CommMessage SerializeAsString, cost:" << (end2 - end1).count() / 1e6 << "ms" << std::endl;
}

void TestVector(std::vector<CommMessage> *data) {
  CommMessage commMessage;
  commMessage.set_data("abc");
  (*data).push_back(commMessage);
}

int main(int argc, char **argv) {
  std::cout<< "test1------------------"<<std::endl;
//  SerializeAsString();
  std::cout<< "test2------------------"<<std::endl;
//  SerializeToArray();
  std::cout<< "test3------------------"<<std::endl;
//  TestSerializeToArrayVerySmallSetData();
  std::cout<< "test4------------------"<<std::endl;
//  TestSerializeToString();
  std::vector<CommMessage> resp;
  TestVector(&resp);
  std::cout <<resp.at(0).data() << std::endl;

  return 0;
}