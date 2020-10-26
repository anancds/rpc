//
// Created by cds on 2020/10/26.
//

#ifndef RPC_MESSAGE_H
#define RPC_MESSAGE_H
#include <iostream>
#include <vector>
// template <typename k, typename v>
class Message {
 public:
  const void *keys;

  const void *values;

  int key_len_;

  int value_len_;

  template <typename V>
  void AddVectorData(const std::vector<uint64_t> &key_data, const std::vector<V> &value_data) {
    //    std::vector<char> v(values);
//    keys = (void *)(&key_data);
    keys = key_data.data();
    values = value_data.data();
    key_len_ = key_data.size() * sizeof(std::vector<uint64_t>);
//    key_len = key_data.size() * sizeof(void *);
    //    value_len = value_data.size() * sizeof(std::vector<V>);
    value_len_ = value_data.size() * sizeof(void *);
    std::vector<uint64_t> *test = reinterpret_cast<std::vector<uint64_t> *>(const_cast<void *>(keys));

    //    const std::vector<float> *v = reinterpret_cast<const std::vector<float>*>(&values);
  }

  template <typename V>
  void AddArrayData(const uint64_t *key_data, const V *value_data, uint64_t key_len, uint64_t value_len) {
    //    std::vector<char> v(values);
    keys = (void *)(key_data);
    values = (void *)(value_data);
    key_len_ = key_len * sizeof(uint64_t *);
    value_len_ = value_len * sizeof(V *);
  }
};
#endif  // RPC_MESSAGE_H
