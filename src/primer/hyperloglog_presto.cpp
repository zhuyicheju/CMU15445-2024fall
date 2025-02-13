//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hyperloglog_presto.cpp
//
// Identification: src/primer/hyperloglog_presto.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "primer/hyperloglog_presto.h"
#include <sys/types.h>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include "common/util/hash_util.h"

namespace bustub {

/** @brief Parameterized constructor. */
template <typename KeyType>
HyperLogLogPresto<KeyType>::HyperLogLogPresto(int16_t n_leading_bits)
    : dense_bucket_(pow(2, n_leading_bits < 0 ? 0 : n_leading_bits)),
      cardinality_(0),
      nbits_(n_leading_bits < 0 ? 0 : n_leading_bits) {}

/** @brief Element is added for HLL calculation. */
template <typename KeyType>
auto HyperLogLogPresto<KeyType>::AddElem(KeyType val) -> void {
  hash_t value = CalculateHash(val);
  std::bitset<64> bit(value);
  size_t idx = (nbits_ != 0) ? (value >> (64 - nbits_)) : 0;
  uint64_t num = 0;
  for (size_t i = 0; i < 64 - nbits_ && (!bit[i]); i++) {
    num++;
  }

  std::bitset<DENSE_BUCKET_SIZE> dense(num & 0xF);
  std::bitset<OVERFLOW_BUCKET_SIZE> overflow((num >> 4) & 0x7);

  if (overflow_bucket_.find(idx) == overflow_bucket_.end()) {
    if (num > 0xF) {
      overflow_bucket_[idx] = overflow;
      dense_bucket_[idx] = dense;
    } else {
      dense_bucket_[idx] = dense_bucket_[idx].to_ullong() > dense.to_ullong() ? dense_bucket_[idx] : dense;
    }
  } else {
    if (overflow.to_ullong() > overflow_bucket_[idx].to_ullong()) {
      overflow_bucket_[idx] = overflow;
      dense_bucket_[idx] = dense;
    } else if (overflow.to_ullong() == overflow_bucket_[idx].to_ullong()) {
      dense_bucket_[idx] = dense_bucket_[idx].to_ullong() > dense.to_ullong() ? dense_bucket_[idx] : dense;
    }
  }
}

/** @brief Function to compute cardinality. */
template <typename T>
auto HyperLogLogPresto<T>::ComputeCardinality() -> void {
  double sum = 0;
  size_t m = pow(2, nbits_ < 0 ? 0 : nbits_);
  for (size_t idx = 0; idx < m; idx++) {
    size_t r = dense_bucket_[idx].to_ullong();
    if (overflow_bucket_.find(idx) != overflow_bucket_.end()) {
      r = r + (overflow_bucket_[idx].to_ullong() << DENSE_BUCKET_SIZE);
    }
    sum += pow(2, -static_cast<u_int8_t>(r));
  }
  cardinality_ = floor(CONSTANT * pow(2, 2 * nbits_) / sum);
}

template class HyperLogLogPresto<int64_t>;
template class HyperLogLogPresto<std::string>;
}  // namespace bustub