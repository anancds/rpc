//
// Created by cds on 2020/10/26.
//

#ifndef RPC_MESSAGE_H
#define RPC_MESSAGE_H
#include <iostream>
#include <vector>

class Message {
 public:
  Message() : keys(nullptr), values(nullptr), key_len_(0), value_len_(0) {}
  virtual ~Message() = default;

  // MS: the shortcut of MindSpore
  static const uint32_t MAGIC = 0x4d53;

  struct MessageHeader {
    uint32_t mMagic = 0;
    uint32_t mLength = 0xFFFFFFFF;
  };

  const void *keys;

  const void *values;

  uint64_t key_len_;

  uint64_t value_len_;

  template <typename V>
  void AddVectorData(const std::vector<uint64_t> &key_data, const std::vector<V> &value_data) {
    //    std::vector<char> v(values);
    //    keys = (void *)(&key_data);
    keys = key_data.data();
    values = value_data.data();
    key_len_ = key_data.size() * sizeof(std::vector<uint64_t>);
    value_len_ = value_data.size() * sizeof(std::vector<V>);
    std::vector<uint64_t> *test = reinterpret_cast<std::vector<uint64_t> *>(const_cast<void *>(keys));
  }

  template <typename V>
  void AddArrayData(const uint64_t *key_data, const V *value_data, uint64_t key_len, uint64_t value_len) {
    keys = (void *)(key_data);
    values = (void *)(value_data);
    key_len_ = key_len * sizeof(uint64_t *);
    value_len_ = value_len * sizeof(V *);
  }
};
#endif  // RPC_MESSAGE_H
