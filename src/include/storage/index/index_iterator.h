//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index_iterator.h
//
// Identification: src/include/storage/index/index_iterator.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

/**
 * index_iterator.h
 * For range scan of b+ tree
 */
#pragma once
#include <utility>
#include "buffer/buffer_pool_manager.h"
#include "common/config.h"
#include "storage/page/b_plus_tree_leaf_page.h"
#include "storage/page/page_guard.h"

namespace bustub {

#define INDEXITERATOR_TYPE IndexIterator<KeyType, ValueType, KeyComparator>

INDEX_TEMPLATE_ARGUMENTS
class IndexIterator {
 public:
  // you may define your own constructor based on your member variables
  IndexIterator();
  ~IndexIterator();  // NOLINT

  explicit IndexIterator(page_id_t page_id, BufferPoolManager* bpm);

  explicit IndexIterator(page_id_t page_id, int idx, BufferPoolManager* bpm);

  auto IsEnd() -> bool;

  auto operator*() -> std::pair<const KeyType &, const ValueType &>;

  auto operator++() -> IndexIterator &;

  auto operator==(const IndexIterator &itr) const -> bool { 
    return (page_id_ == itr.page_id_) && (page_idx_ == itr.page_idx_);  
  }

  auto operator!=(const IndexIterator &itr) const -> bool { 
    return (page_id_ != itr.page_id_) || (page_idx_ != itr.page_idx_);
  }

 private:
    page_id_t page_id_;
    ReadPageGuard page_guard_;
    int page_size_;
    int page_idx_;

    BufferPoolManager* bpm_;
};

}  // namespace bustub
