//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index_iterator.cpp
//
// Identification: src/storage/index/index_iterator.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

/**
 * index_iterator.cpp
 */
#include <cassert>
#include <utility>

#include "buffer/buffer_pool_manager.h"
#include "common/config.h"
#include "storage/index/index_iterator.h"
using std::cout, std::endl;
namespace bustub {

/**
 * @note you can change the destructor/constructor method here
 * set your own input parameters
 */
INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::IndexIterator():
  page_id_(INVALID_PAGE_ID), page_idx_(0)
{}

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::~IndexIterator() = default;  // NOLINT

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::IndexIterator(page_id_t page_id, BufferPoolManager* bpm):
  page_id_(page_id), page_idx_(0), bpm_(bpm)
{
  page_guard_ = bpm_->ReadPage(page_id);
  page_size_ = page_guard_.As
    <BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>()->GetSize();
}

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::IndexIterator(page_id_t page_id, int idx, BufferPoolManager* bpm):
  page_id_(page_id), page_idx_(idx), bpm_(bpm)
{
  page_guard_ = bpm_->ReadPage(page_id);
  page_size_ = page_guard_.As
    <BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>()->GetSize();
}

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::IsEnd() -> bool { 
  return page_id_ == INVALID_PAGE_ID;  
}

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::operator*() -> std::pair<const KeyType &, const ValueType &> {
  auto leaf_page = page_guard_.As
          <BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>();
  const KeyType& key = leaf_page->key_array_[page_idx_]; 
  const ValueType& val = leaf_page->rid_array_[page_idx_];
  return std::make_pair(key, val); 
}

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::operator++() -> INDEXITERATOR_TYPE & { 
  if(page_id_ == INVALID_PAGE_ID){
    return *this;
  }
  if(++page_idx_ >= page_size_){
    auto leaf_page = page_guard_.As
            <BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>();    
    page_id_ = leaf_page->next_page_id_;
    page_guard_.Drop();
    if(page_id_ != INVALID_PAGE_ID){
      page_guard_ = bpm_->ReadPage(page_id_);
      leaf_page = page_guard_.As
            <BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>();   
      page_size_ = leaf_page->GetSize();
    }
    page_idx_ = 0;
  }
  return *this;
}

template class IndexIterator<GenericKey<4>, RID, GenericComparator<4>>;

template class IndexIterator<GenericKey<8>, RID, GenericComparator<8>>;

template class IndexIterator<GenericKey<16>, RID, GenericComparator<16>>;

template class IndexIterator<GenericKey<32>, RID, GenericComparator<32>>;

template class IndexIterator<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
