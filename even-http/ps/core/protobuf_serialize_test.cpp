//
// Created by cds on 2020/12/5.
//

#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"
#include "../../../build/even-http/ps/core/test.pb.h"
#include "ps/core/scheduler_node.h"
using namespace mindspore::ps::core;
using namespace mindspore::ps;

void SerializeAsString() {
  CommMessage1 comm_message;
  KVMessage1 kv_message;
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

  kv_message.Clear();
  auto start11 = std::chrono::high_resolution_clock::now();
  kv_message.ParseFromString(comm_message.data());
  auto end11 = std::chrono::high_resolution_clock::now();
  std::cout << "unSerializeAsString, cost:" << (end11 - start11).count() / 1e6 << "ms" << std::endl;
}

void SerializeToArray() {
  CommMessage1 comm_message;
  KVMessage1 kv_message;
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
  auto end2 = std::chrono::high_resolution_clock::now();
  comm_message.SerializeToArray(buffer1, buff_size1);
  auto end3 = std::chrono::high_resolution_clock::now();
  std::cout << "KVMessage SerializeToArray, cost:" << (end - start).count() / 1e6 << "ms" << std::endl;
  std::cout << "set data, cost:" << (end1 - end).count() / 1e6 << "ms" << std::endl;
  std::cout << "CommMessage SerializeToArray, cost:" << (end3 - end2).count() / 1e6 << "ms" << std::endl;

  kv_message.Clear();
  auto start11 = std::chrono::high_resolution_clock::now();
  kv_message.ParseFromArray(comm_message.data().data(), comm_message.data().length());
  auto end11 = std::chrono::high_resolution_clock::now();
  std::cout << "unSerializeAsString, cost:" << (end11 - start11).count() / 1e6 << "ms" << std::endl;
}

void TestSerializeToArrayVerySmallSetData() {
  CommMessage1 comm_message;
  KVMessage1 kv_message;
  std::vector<int> keys(100000000, 1);
  std::vector<int> values(100000000, 2);
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};
  size_t buff_size = kv_message.ByteSizeLong();
  char *buffer = new char[buff_size + 1];
  auto start = std::chrono::high_resolution_clock::now();
  kv_message.SerializeToArray(buffer, buff_size);
  auto end = std::chrono::high_resolution_clock::now();
  size_t buff_size1 = kv_message.ByteSizeLong();
  char *buffer1 = new char[buff_size1 + 1];
  auto end2 = std::chrono::high_resolution_clock::now();
  kv_message.SerializeToArray(buffer1, buff_size1);
  auto end3 = std::chrono::high_resolution_clock::now();
  std::cout << "KVMessage SerializeToArray, cost:" << (end - start).count() / 1e6 << "ms" << std::endl;
  std::cout << "CommMessage SerializeToArray, cost:" << (end3 - end2).count() / 1e6 << "ms" << std::endl;
}

void TestSerializeToString() {
  KVMessage1 kv_message;
  std::vector<int> keys(100000000, 1);
  std::vector<int> values(100000000, 2);
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};
  auto start = std::chrono::high_resolution_clock::now();
  std::string res;
  kv_message.SerializeToString(&res);
  auto end = std::chrono::high_resolution_clock::now();
  auto end1 = std::chrono::high_resolution_clock::now();
  auto end2 = std::chrono::high_resolution_clock::now();
  std::cout << "KVMessage SerializeAsString, cost:" << (end - start).count() / 1e6 << "ms" << std::endl;
  std::cout << "small set data, cost:" << (end1 - end).count() / 1e6 << "ms" << std::endl;
  std::cout << "CommMessage SerializeAsString, cost:" << (end2 - end1).count() / 1e6 << "ms" << std::endl;
}

void PackTest1() {
  KVMessage1 kv_message;
  std::vector<int> keys(10000000, 1);
  std::vector<int> values(10000000, 2);
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};

  auto start = std::chrono::high_resolution_clock::now();
  PackTest test;
  // test.mutable_transport_options()->GetMaybeArenaPointer();
  test.mutable_transport_options()->PackFrom(kv_message);
  // std::cout << test.transport_options().GetArena()->SpaceUsed() << std::endl;
  auto end = std::chrono::high_resolution_clock::now();
  test.transport_options().UnpackTo(&kv_message);
  auto end1 = std::chrono::high_resolution_clock::now();
  std::cout << "PackFrom, cost:" << (end - start).count() / 1e6 << "ms" << std::endl;
  std::cout << "PackFrom + uppackto, cost:" << (end1 - start).count() / 1e6 << "ms" << std::endl;
}

void PackTest2() {
  KVMessage kv_message;
  std::vector<int> keys(100000000, 1);
  std::vector<int> values(100000000, 2);
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};
  auto start = std::chrono::high_resolution_clock::now();
  Inner in;
  in.mutable_data()->PackFrom(kv_message);
  auto end = std::chrono::high_resolution_clock::now();
  PackTest test;
  test.mutable_transport_options()->PackFrom(in);
  auto end1 = std::chrono::high_resolution_clock::now();
  std::cout << "PackFrom, cost:" << (end - start).count() / 1e6 << "ms" << std::endl;
  std::cout << "PackFrom1, cost:" << (end1 - start).count() / 1e6 << "ms" << std::endl;
}

struct header {
  Protos message_proto_ = Protos::PROTOBUF;
  uint32_t message_meta_length = 0;
  uint64_t message_length_ = 0;
};
void TestStruct() {
  header temp;
  std::cout << sizeof(temp) << std::endl;
  Protos protos;
  std::cout << sizeof(protos) << std::endl;
}

void TestVoid(void **output) {
  unsigned char a[1] = {'a'};
  *output = a;
}

void TestSharedPtr(std::shared_ptr<std::vector<unsigned char>> data) { data->push_back('a'); }

void TestMemcpy() {
  size_t size = 80000000;
  std::vector<unsigned char> res(size, 1);

  auto start = std::chrono::high_resolution_clock::now();
  std::shared_ptr<std::vector<unsigned char>> test = std::make_shared<std::vector<unsigned char>>(size, 0);
  auto end = std::chrono::high_resolution_clock::now();
  memcpy_s(test->data(), size, res.data(), size);
  auto end1 = std::chrono::high_resolution_clock::now();
  std::cout << "init vector, cost:" << (end - start).count() / 1e6 << "ms" << std::endl;
  std::cout << "memcpy vector, cost:" << (end1 - end).count() / 1e6 << "ms" << std::endl;

  auto start1 = std::chrono::high_resolution_clock::now();
  std::shared_ptr<unsigned char[]> test1(new unsigned char[size]);
  auto end4 = std::chrono::high_resolution_clock::now();

  memcpy_s(test1.get(), size, res.data(), size);
  auto end5 = std::chrono::high_resolution_clock::now();
  std::cout << "init char, cost:" << (end4 - start1).count() / 1e6 << "ms" << std::endl;
  std::cout << "memcpy char, cost:" << (end5 - start1).count() / 1e6 << "ms" << std::endl;

  auto start2 = std::chrono::high_resolution_clock::now();
  unsigned char *test2 = new unsigned char[size];
  auto end8 = std::chrono::high_resolution_clock::now();
  memcpy_s(test2, size, res.data(), size);
  auto end9 = std::chrono::high_resolution_clock::now();
  std::cout << "init char1, cost:" << (end8 - start2).count() / 1e6 << "ms" << std::endl;
  std::cout << "memcpy char1, cost:" << (end9 - start2).count() / 1e6 << "ms" << std::endl;
}

int main(int argc, char **argv) {
  std::cout << "test1------------------" << std::endl;
  // SerializeAsString();
  std::cout << "test2------------------" << std::endl;
  // SerializeToArray();
  std::cout << "test3------------------" << std::endl;
  //  TestSerializeToArrayVerySmallSetData();
  std::cout << "test4------------------" << std::endl;
  // TestSerializeToString();

  std::cout << "test5------------------" << std::endl;
  // PackTest1();
  std::cout << "test6------------------" << std::endl;
  // PackTest2();
  std::cout << "test7------------------" << std::endl;
  std::cout << "test8------------------" << std::endl;
  // TestStruct();
  std::cout << "test9------------------" << std::endl;
  void *res;
  TestVoid(&res);
  char *test = reinterpret_cast<char *>(res);
  std::string a(test, 1);
  std::cout << "test9:" << a << std::endl;

  std::cout << "test10------------------" << std::endl;
  auto res_ptr = std::make_shared<std::vector<unsigned char>>();
  TestSharedPtr(res_ptr);
  std::cout << "test10------------------" << res_ptr->at(0) << std::endl;

  std::cout << "test11------------------" << std::endl;
  TestMemcpy();

  return 0;
}