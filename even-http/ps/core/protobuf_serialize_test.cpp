//
// Created by cds on 2020/12/5.
//

#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"
#include "../../../build/even-http/ps/core/test.pb.h"
#include "ps/core/scheduler_node.h"
#include "nlohmann/json.hpp"
#include <unordered_map>

using namespace mindspore::ps::core;
using namespace mindspore::ps;
using VectorPtr = std::shared_ptr<std::vector<unsigned char>>;
std::unordered_map<std::string, int> abc = {{"a", 1}, {"b", 2}};

#define CHECK_FAIL_RETURN_UNEXPECTED(_condition)                             \
  do {                                                                       \
    if (!(_condition)) {                                                     \
      MS_LOG(ERROR) << "parse protobuf message failed.";                     \
    }                                                                        \
  } while (false)


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

void TestVoid(void *output, size_t *size) {
  KVMessage data;
  std::vector<int> keys(33, 11);
  *data.mutable_keys() = {keys.begin(), keys.end()};
}

void TestSharedPtr(std::shared_ptr<std::vector<unsigned char>> *data) {
  std::shared_ptr<std::vector<unsigned char>> temp = std::make_shared<std::vector<unsigned char>>(0);
  temp->push_back('a');

  *data = temp;
}

void TestMemcpy() {
  size_t size = 80000000;
  std::vector<unsigned char> res(size + 1, 1);

  // auto start = std::chrono::high_resolution_clock::now();
  // std::shared_ptr<std::vector<unsigned char>> test = std::make_shared<std::vector<unsigned char>>(size, 0);
  // auto end = std::chrono::high_resolution_clock::now();
  // int ret = memcpy_s(test->data(), size, res.data(), size);
  // if (ret != 0) {
  //   MS_LOG(EXCEPTION) << "The memcpy_s error, errorno(" << ret << ")";
  // }
  // auto end1 = std::chrono::high_resolution_clock::now();
  // std::cout << "init vector, cost:" << (end - start).count() / 1e6 << "ms" << std::endl;
  // std::cout << "memcpy vector, cost:" << (end1 - end).count() / 1e6 << "ms" << std::endl;

  auto start1 = std::chrono::high_resolution_clock::now();
  std::shared_ptr<unsigned char[]> test1(new unsigned char[size + 1]);
  auto end4 = std::chrono::high_resolution_clock::now();

  unsigned char *temp = res.data();

  int ret = memcpy_s(test1.get(), size + 1, temp, size);
  if (ret != 0) {
    MS_LOG(EXCEPTION) << "The memcpy_s error, errorno(" << ret << ")";
  }
  auto end5 = std::chrono::high_resolution_clock::now();
  std::cout << "init char, cost:" << (end4 - start1).count() / 1e6 << "ms" << std::endl;
  std::cout << "memcpy char, cost:" << (end5 - start1).count() / 1e6 << "ms" << std::endl;

  auto start2 = std::chrono::high_resolution_clock::now();
  unsigned char *test2 = new unsigned char[size];
  auto end8 = std::chrono::high_resolution_clock::now();
  ret = memcpy_s(test2, size, res.data(), size);
  if (ret != 0) {
    MS_LOG(EXCEPTION) << "The memcpy_s error, errorno(" << ret << ")";
  }
  auto end9 = std::chrono::high_resolution_clock::now();
  std::cout << "init char1, cost:" << (end8 - start2).count() / 1e6 << "ms" << std::endl;
  std::cout << "memcpy char1, cost:" << (end9 - start2).count() / 1e6 << "ms" << std::endl;
}

void TestCommand() {
  TestEnum temp;
  temp.set_cmd(command1::PULL);
  std::cout << "the cmd is:" << temp.cmd() << std::endl;
}

void TestVector(VectorPtr res) {
  KVMessage res_data;
  res_data.add_keys(1);
  res_data.add_values(1);
  res->resize(res_data.ByteSizeLong());
  int ret =
    memcpy_s(res->data(), res_data.ByteSizeLong(), res_data.SerializeAsString().data(), res_data.ByteSizeLong());
  if (ret != 0) {
    MS_LOG(EXCEPTION) << "The memcpy_s error, errorno(" << ret << ")";
  }
  std::cout << "test vector------------------" << res->size() << " -" << res_data.ByteSizeLong() << std::endl;
}

void TestVector1(std::vector<float> *res) {
  res->push_back(1);
  std::cout << "res:" << res->at(0) << std::endl;
}

void TestTimestamp() {
  std::cout << "the timestamp is:"
            << std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now())
                 .time_since_epoch()
                 .count()
            << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::cout << "the timestamp is:"
            << std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now())
                 .time_since_epoch()
                 .count()
            << std::endl;
}

void TestMemcpy_s() {
  const char *a = "abcefgh";
  const char *b = "hijk";
  int dest_size = strlen(a) + strlen(b) + 2;
  std::vector<char> c(dest_size);
  std::cout << "dest_size" << dest_size << std::endl;
  memcpy_s(c.data(), dest_size, a, strlen(a));
  char d = '?';
  std::cout << "dest_size - strlen(a)" << dest_size - strlen(a) << std::endl;
  memcpy_s(c.data() + strlen(a), dest_size - strlen(a), &d, 1);
  std::cout << "dest_size - strlen(a) - 1" << dest_size - strlen(a) - 1 << std::endl;
  memcpy_s(c.data() + strlen(a) + 1, dest_size - strlen(a) - 1, b, strlen(b));
  std::cout << c.data() << std::endl;
}

void TestMemcpyCost() {
  std::string temp(4096, 'a');
  char res[40960] = {0};
  std::cout << "the timestamp is:"
            << std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now())
                 .time_since_epoch()
                 .count()
            << std::endl;
  memcpy_s(&res, 40960, temp.data(), 40);
  std::cout << "the timestamp is:"
            << std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now())
                 .time_since_epoch()
                 .count()
            << std::endl;

  // int offset = 40;
  // for (int i = 0; i < 256; i++) {
  //   memcpy_s(&res, 40960, temp.data(), 40);
  // }
  // std::cout << "the timestamp is:"
  //           << std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now())
  //                .time_since_epoch()
  //                .count()
  //           << std::endl;
}

void TestNull(void *data) {
  char *abc = nullptr;
  std::string a;
  char *test;
}

void TestSharedPointer() {
  {
    std::unique_ptr<char[]> ptr(new char[1024 * 1024 * 10]);
    auto temp = new char[1024 * 1024 * 100];
    std::cout << ptr.get();
    std::cout << temp;
  }
  std::cout << "";
}

void TestVector() {
  std::vector<size_t> temp{1, 2, 3};

  std::shared_ptr<std::vector<size_t>> input_shape = std::make_shared<std::vector<size_t>>(temp.begin(), temp.end());
  for (auto i = 0; i < input_shape->size(); i++) {
    std::cout << input_shape->at(i) << std::endl;
  }

  EmbeddingTableMeta meta;

  std::shared_ptr<uint8_t[]> res(new uint8_t[0]);
  std::shared_ptr<std::vector<uint8_t>> temp1 = std::make_shared<std::vector<uint8_t>>();
  std::cout << temp1.get() << " " << temp1->data() << std::endl;
}

void Testtime() {
  std::cout << std::endl;
  std::string a;
  std::shared_ptr<u_int8_t> res(new uint8_t[0]);
  std::cout << res.get() << std::endl;
  std::cout << a.data() << ", " << a.length() << std::endl;

  char temp[0];
  std::cout << "temp:" << temp << std::endl;
}

void TestJson() {
  nlohmann::json js;
  nlohmann::json js_ret;
  js["server"] = 4;
  js["node_id"] = {"a", "b"};
  std::cout << js.dump() << std::endl;
  js_ret = js.parse(js.dump());
  if (js_ret.contains("server")) {
    uint32_t temp = js_ret.at("server");
    std::cout << temp << std::endl;
  }
  const nlohmann::json &res = js_ret.at("node_id");
  std::vector<std::string> vec;
  for (auto &element : res) {
    std::cout << element << '\n';
    vec.emplace_back(element);
  }
  std::cout << vec[0] << std::endl;
  SchedulerNode node;
  std::cout << node.node_id() << std::endl;
}

void TestMap() {
  nlohmann::json js;
  std::unordered_map<std::string, std::string> res;
  js["node_ids"].push_back(res);
  std::cout << js.dump() << std::endl;
}

std::unordered_map<std::string, int> &GetNode() { return abc; }

void TestGetNode() {
  std::unordered_map<std::string, int> test = GetNode();
  test["c"] = 2;
  abc.clear();
  std::unordered_map<std::string, int> test1 = GetNode();
  std::cout << test1["b"];

  CHECK_FAIL_RETURN_UNEXPECTED(true);
  CHECK_FAIL_RETURN_UNEXPECTED(false);
}

int main(int argc, char **argv) {
  // TestCommand();
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
  // std::shared_ptr<std::vector<unsigned char>> res = std::make_shared<std::vector<unsigned char>>();
  size_t size;
  // TestVoid(res->data(), &size);

  std::cout << "test10------------------" << std::endl;
  // auto res_ptr = std::make_shared<std::vector<unsigned char>>();
  // TestSharedPtr(&res_ptr);
  // std::cout << "test10------------------" << res_ptr->at(0) << std::endl;

  std::cout << "test11------------------" << std::endl;
  // TestMemcpy();
  // auto output = std::make_shared<std::vector<unsigned char>>();
  // TestVector(output);
  // std::cout << "test11------------------" << output->size() << std::endl;

  // std::vector<float> temp(1, 0);
  // TestVector1(&temp);
  // std::cout << "test12" << temp[0] << std::endl;
  // TestTimestamp();
  // TestMemcpy_s();
  for (int i = 0; i < 356; i++) {
    // TestMemcpyCost();
  }
  // TestSharedPointer();
  // TestVector();
  std::string a;
  TestNull(a.data());
  // Testtime();
  // TestJson();
  TestMap();

  TestGetNode();
  return 0;
}