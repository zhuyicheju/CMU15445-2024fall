//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hyperloglog.cpp
//
// Identification: src/primer/hyperloglog.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "primer/hyperloglog.h"
#include <bitset>
#include <cstddef>
#include <cstdint>
#include "common/util/hash_util.h"

namespace bustub {

/** @brief Parameterized constructor. */
template <typename KeyType>
HyperLogLog<KeyType>::HyperLogLog(int16_t n_bits)
    : cardinality_(0), buckets_(pow(2, n_bits > 0 ? n_bits : 0)), lock_(pow(2, n_bits > 0 ? n_bits : 0)) {
  if (n_bits < 0) {
    n_bits = 0;
  }
  nbits_ = n_bits;
  // buckets_(pow(2, nbits_));
}

/**
 * @brief Function that computes binary.
 *
 * @param[in] hash
 * @returns binary of a given hash
 */
template <typename KeyType>
auto HyperLogLog<KeyType>::ComputeBinary(const hash_t &hash) const -> std::bitset<BITSET_CAPACITY> {
  return {hash};
}

/**
 * @brief Function that computes leading zeros.
 *
 * @param[in] bset - binary values of a given bitset
 * @returns leading zeros of given binary set
 */
template <typename KeyType>
auto HyperLogLog<KeyType>::PositionOfLeftmostOne(const std::bitset<BITSET_CAPACITY> &bset) const -> uint64_t {
  uint64_t cnt = 0;
  for (size_t i = BITSET_CAPACITY - 1 - nbits_; i != 0; i--) {
    cnt++;
    if (bset[i]) {
      break;
    }
  }
  return cnt;
}

/**
 * @brief Adds a value into the HyperLogLog.
 *
 * @param[in] val - value that's added into hyperloglog
 */
template <typename KeyType>
auto HyperLogLog<KeyType>::AddElem(KeyType val) -> void {
  hash_t value = CalculateHash(val);
  std::bitset<64> bit = ComputeBinary(value);
  size_t idx = (nbits_ != 0) ? (value >> (64 - nbits_)) : 0;
  uint64_t num = PositionOfLeftmostOne(bit);

  lock_[idx].lock();
  buckets_[idx] = num > buckets_[idx] ? num : buckets_[idx];
  lock_[idx].unlock();
}

/**
 * @brief Function that computes cardinality.
 */
template <typename KeyType>
auto HyperLogLog<KeyType>::ComputeCardinality() -> void {
  double sum = 0;
  for (uint8_t r : buckets_) {
    sum += pow(2, -r);
  }
  uint64_t cardinality = floor(CONSTANT * pow(2, 2 * nbits_) / sum);

  cardinality_lock_.lock();
  cardinality_ = cardinality;
  cardinality_lock_.unlock();
}

template class HyperLogLog<int64_t>;
template class HyperLogLog<std::string>;

}  // namespace bustub
