//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// b_plus_tree.cpp
//
// Identification: src/storage/index/b_plus_tree.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/index/b_plus_tree.h"
#include <algorithm>
#include "common/config.h"
#include "storage/index/b_plus_tree_debug.h"
#include "storage/page/b_plus_tree_header_page.h"
#include "storage/page/b_plus_tree_leaf_page.h"
#include "storage/page/b_plus_tree_page.h"
#include "storage/page/page_guard.h"

namespace bustub {

INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::BPlusTree(std::string name, page_id_t header_page_id, BufferPoolManager *buffer_pool_manager,
                          const KeyComparator &comparator, int leaf_max_size, int internal_max_size)
    : index_name_(std::move(name)),
      bpm_(buffer_pool_manager),
      comparator_(std::move(comparator)),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size),
      header_page_id_(header_page_id) {
  WritePageGuard guard = bpm_->WritePage(header_page_id_);
  auto root_page = guard.AsMut<BPlusTreeHeaderPage>();
  root_page->root_page_id_ = INVALID_PAGE_ID;
}

/**
 * @brief Helper function to decide whether current b+tree is empty
 * @return Returns true if this B+ tree has no keys and values.
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsEmpty() const -> bool { return is_empty_; }

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::PageSearch(page_id_t cur_page_id, const KeyType &key, std::vector<ValueType> *result) -> bool {
  if(cur_page_id == INVALID_PAGE_ID){
    return false;
  }
  ReadPageGuard cur_page_guard = bpm_->ReadPage(cur_page_id);
  auto cur_page = cur_page_guard.As<BPlusTreePage>();
  if(cur_page->IsLeafPage()){
    auto leaf_page = cur_page_guard.As
                      <BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>();
    
    int size = leaf_page->GetSize();
    auto& key_array = leaf_page->key_array_;
    auto& rid_array = leaf_page->rid_array_;
    for(int i = 0; i < size; i ++) {
      if(comparator_(key_array[i], key) == 0){
        result->push_back(rid_array[i]);
        return true;
      }
    }

    return false;
    
  }

  if(cur_page->IsInternalPage()){
    auto internal_page = cur_page_guard.As
                        <BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>>();
    page_id_t next_page_id = INVALID_PAGE_ID;
    auto& key_array = internal_page->key_array_;
    int i = 1;
    for(i < internal_page->GetSize() && comparator_(key, key_array[i]); i ++){;}
    next_page_id = internal_page->page_id_array_[i-1];
    ///
    /// 是否要释放PAGEGUARD
    ///
    return PageSearch(next_page_id, key, result);
  }

}


/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/**
 * @brief Return the only value that associated with input key
 *
 * This method is used for point query
 *
 * @param key input key
 * @param[out] result vector that stores the only value that associated with input key, if the value exists
 * @return : true means key exists
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetValue(const KeyType &key, std::vector<ValueType> *result) -> bool {
  ReadPageGuard header_page_guard = bpm_->ReadPage(header_page_id_);
  auto header_page = header_page_guard.As<BPlusTreeHeaderPage>();
  return PageSearch(header_page->root_page_id_, key, result);
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::InsertLeaf(LeafPage* leaf_page, const KeyType &key, const ValueType &value) -> bool {
    int cur_size = leaf_page->GetSize();
    if(cur_size == leaf_page->GetMaxSize()){
      return false;
    }
    int i = 0;    
    auto& key_array = leaf_page->key_array_;
    auto& rid_array = leaf_page->rid_array_;
    for(; i < cur_size && comparator_(key, key_array[i]) < 0; i ++) {;}
    if(comparator_(key, key_array[i]) == 0){
      //二者等于
      return false;
    }

    //将i以后的键值对往后挪一格
    for(int j = cur_size; j > i; j--){
      key_array[j] = key_array[j-1];
      rid_array[j] = rid_array[j-1];
    }

    leaf_page->ChangeSizeBy(1);
    key_array[i] = std::move(key);
    rid_array[i] = std::move(value);

    return true;
}



/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/**
 * @brief Insert constant key & value pair into b+ tree
 *
 * if current tree is empty, start new tree, update root page id and insert
 * entry, otherwise insert into leaf page.
 *
 * @param key the key to insert
 * @param value the value associated with key
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value) -> bool {
  WritePageGuard header_guard = bpm_->WritePage(header_page_id_);
  auto header_page = header_guard.AsMut<BPlusTreeHeaderPage>();
  page_id_t root_page_id = INVALID_PAGE_ID;

  if(header_page->root_page_id_ == INVALID_PAGE_ID){
    //当前树无根节点
    page_id_t page = bpm_->NewPage();
    header_page->root_page_id_ = page;
    root_page_id = page;
    header_guard.Drop();
    WritePageGuard root_guard = bpm_->WritePage(page);
    auto root_page = root_guard.AsMut<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>();
    root_page->Init(leaf_max_size_);
    //设置root_page基本类型
  }else{
    root_page_id = header_page->root_page_id_;
    header_guard.Drop();
  }

  WritePageGuard root_guard = bpm_->WritePage(root_page_id);
  auto root_page = root_guard.AsMut<BPlusTreePage>();

  if(root_page->IsLeafPage()){
    auto leaf_page = root_guard.AsMut<LeafPage>();
    return InsertLeaf(leaf_page, key, value);
  }
  if(root_page->IsInternalPage()){
    //内部节点向下遍历
  }
  return false;
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/**
 * @brief Delete key & value pair associated with input key
 * If current tree is empty, return immediately.
 * If not, User needs to first find the right leaf page as deletion target, then
 * delete entry from leaf page. Remember to deal with redistribute or merge if
 * necessary.
 *
 * @param key input key
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key) {
  // Declaration of context instance.
  Context ctx;
  (void)ctx;
}

/*****************************************************************************
 * INDEX ITERATOR
 *****************************************************************************/
/**
 * @brief Input parameter is void, find the leftmost leaf page first, then construct
 * index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin() -> INDEXITERATOR_TYPE { return INDEXITERATOR_TYPE(); }

/**
 * @brief Input parameter is low key, find the leaf page that contains the input key
 * first, then construct index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin(const KeyType &key) -> INDEXITERATOR_TYPE { return INDEXITERATOR_TYPE(); }

/**
 * @brief Input parameter is void, construct an index iterator representing the end
 * of the key/value pair in the leaf node
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::End() -> INDEXITERATOR_TYPE { return INDEXITERATOR_TYPE(); }

/**
 * @return Page id of the root of this tree
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetRootPageId() -> page_id_t { 
  ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
  auto header_page = guard.As<BPlusTreeHeaderPage>();
  return header_page->root_page_id_;
}

template class BPlusTree<GenericKey<4>, RID, GenericComparator<4>>;

template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>>;

template class BPlusTree<GenericKey<16>, RID, GenericComparator<16>>;

template class BPlusTree<GenericKey<32>, RID, GenericComparator<32>>;

template class BPlusTree<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
