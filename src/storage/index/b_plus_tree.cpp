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
#include <cassert>
#include <memory>
#include <utility>
#include "common/config.h"
#include "storage/index/b_plus_tree_debug.h"
#include "storage/index/index_iterator.h"
#include "storage/page/b_plus_tree_header_page.h"
#include "storage/page/b_plus_tree_leaf_page.h"
#include "storage/page/b_plus_tree_page.h"
#include "storage/page/page_guard.h"
using std::cout;
using std::endl;
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
auto BPLUSTREE_TYPE::IsEmpty() const -> bool { return size_ == 0; }

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::PageSearch(page_id_t cur_page_id, const KeyType &key, std::vector<ValueType> *result) const -> bool {
  //cout<<"search"<<cur_page_id<<endl;
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
    for(int i = 0; i < size && comparator_(key, key_array[i]) >= 0; i ++) {
      if(comparator_(key_array[i], key) == 0){
        result->push_back(rid_array[i]);
        return true;
      }
    }
    return false;
  }

  if(cur_page->IsInternalPage()){
    auto internal_page = cur_page_guard.As
                        <BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
    page_id_t next_page_id = INVALID_PAGE_ID;
    auto& key_array = internal_page->key_array_;
    int i = 1;
    int cur_size = internal_page->GetSize();
    // for(int j = 1;j <= cur_size;j++){
    //   cout<<internal_page->key_array_[i]<<",";
    // }
    // cout<<endl;
    for(; i <= cur_size && comparator_(key, key_array[i]) >= 0; i ++){;}
    next_page_id = internal_page->page_id_array_[i-1];
    ///
    /// 是否要释放PAGEGUARD
    ///
    return PageSearch(next_page_id, key, result);
  }
  return false;
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
auto BPLUSTREE_TYPE::PageInsert(page_id_t cur_page_id, const KeyType &key, const ValueType &value, std::shared_ptr<Context> context) -> bool {
  if(cur_page_id == INVALID_PAGE_ID){
    return false;
  }
  WritePageGuard cur_page_guard = bpm_->WritePage(cur_page_id);
  auto cur_page = cur_page_guard.AsMut<BPlusTreePage>();

  if(cur_page->IsLeafPage()){
    auto leaf_page = cur_page_guard.AsMut<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>();
    return InsertLeaf(leaf_page, key, value, context, cur_page_id);
  }

  if(cur_page->IsInternalPage()){
    auto internal_page = cur_page_guard.As
                        <BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
    page_id_t next_page_id = INVALID_PAGE_ID;
    int i = 1;
    auto& key_array = internal_page->key_array_;
    for(; i <= internal_page->GetSize() && comparator_(key, key_array[i]) >= 0; i ++){;}
    next_page_id = internal_page->page_id_array_[i-1];

    context->write_set_.push_front(std::move(cur_page_guard));
    return PageInsert(next_page_id, key, value, context);
  }
  return false;
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::UpInsert(const std::shared_ptr<Context>& context, page_id_t left_page, page_id_t right_page, KeyType right_key) -> bool {

  if(context->write_set_.empty()){
      //cout<<"EMPTYUpinsert "<<right_key<<endl;
    //当前为根节点
    page_id_t new_page_id = bpm_->NewPage();
    WritePageGuard new_page_guard = bpm_->WritePage(new_page_id);
    auto new_page = new_page_guard.AsMut
        <BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
    new_page->Init(internal_max_size_);
    new_page->key_array_[1] = right_key;
    new_page->page_id_array_[0] = left_page;
    new_page->page_id_array_[1] = right_page;

    new_page->ChangeSizeBy(1);

    context->header_page_.value().
        AsMut<BPlusTreeHeaderPage>()->root_page_id_ = new_page_id;

    context->root_page_id_ = new_page_id;

    return true;
  }

  WritePageGuard up_page_guard = std::move(context->write_set_.front());
  context->write_set_.pop_front();
  auto up_page = up_page_guard.AsMut
        <BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  //获得context中的栈元素
  int cur_size = up_page->GetSize();
  if(cur_size == up_page->GetMaxSize()){
      //cout<<"MAXUpinsert "<<right_key<<endl;      
    //内部节点也已经满
      page_id_t new_internal_page_id = bpm_->NewPage();
      auto new_internal_page_guard = bpm_->WritePage(new_internal_page_id);
      auto new_internal_page = new_internal_page_guard.AsMut
        <BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
      new_internal_page->Init(internal_max_size_);

      int i = 1;
      int ceil = cur_size / 2;
      int left_or_right = comparator_(right_key, up_page->key_array_[ceil]) > 0 ? 1 : 0;
      int in_the_mid = comparator_(right_key, up_page->key_array_[ceil+1]) < 0 ? 1 : 0;
      //必须在leftorright==1下成立

      int already_pushed = 0;
      for(; i <= ceil; i ++){
        if(left_or_right == 0 && already_pushed == 0 && comparator_(right_key, up_page->key_array_[i]) < 0){
          new_internal_page->key_array_[i] = std::move(right_key);
          new_internal_page->page_id_array_[i - 1] = left_page;
          new_internal_page->page_id_array_[i] = right_page;
          //同时挂上两个页，此时顺序改变

          already_pushed = 1;
        }
        
        new_internal_page->key_array_[i + already_pushed] = std::move(up_page->key_array_[i]);
        new_internal_page->page_id_array_[i + 2 * already_pushed - 1] = std::move(up_page->page_id_array_[i + already_pushed - 1]);
                                          //乘2代表抵消-1, 也就是额外加入的两个指针
      }


      KeyType up_key;
      if(left_or_right == 1 && in_the_mid == 1){
        new_internal_page->page_id_array_[i - 1] = left_page;
        up_key = right_key;
      }else{
        if(already_pushed == 0){
          //新插入节点不在左边
          new_internal_page->page_id_array_[i - 1] = up_page->page_id_array_[ i - 1 ];
        }
        up_key = std::move(up_page->key_array_[i]);
        //将此键向上传
      }
      
      new_internal_page->ChangeSizeBy(i - left_or_right);

      if(left_or_right == 1 && in_the_mid == 1){
        up_page->page_id_array_[0] = right_page;
      }

      int start_point =  i + ((left_or_right==1&&in_the_mid==1)? 0 : 1);
      already_pushed = (left_or_right==1&&in_the_mid==1)? 1 : 0;
      int mid_sub = (left_or_right==1&&in_the_mid==1)? 1 : 0;
      int j = start_point;
      for(; j <= cur_size; j++){
        if(left_or_right == 1 && already_pushed == 0 && comparator_(right_key, up_page->key_array_[j]) < 0){
          up_page->key_array_[j - start_point + 1] = std::move(right_key);
          up_page->page_id_array_[j - start_point] = left_page;
          up_page->page_id_array_[j - start_point + 1] = right_page;
          //同时挂上两个页，此时顺序改变

          already_pushed = 1;
        }
        up_page->key_array_[j - start_point + already_pushed - mid_sub + 1] = std::move(up_page->key_array_[j]);
        up_page->page_id_array_[j - start_point + 2*already_pushed - mid_sub] = up_page->page_id_array_[j + already_pushed - 1];
      }

      if(left_or_right==1&&already_pushed==0){
        up_page->key_array_[j - start_point + 1] = std::move(right_key);
        up_page->page_id_array_[j-start_point] = left_page;
        up_page->page_id_array_[j-start_point+1] = right_page;
      }
      if(left_or_right == 0 && already_pushed == 0){
        //新插入节点不在右边
        up_page->page_id_array_[j - start_point] = up_page->page_id_array_[j-1];
      }

      up_page->ChangeSizeBy(-i + left_or_right);

      return UpInsert(context, new_internal_page_id,up_page_guard.GetPageId(), up_key);
  
  }
  //当前节点未满

      // cout<<"NORMUpinsert "<<right_key<<" "<<left_page<<" "<<right_page<<endl;
      // for(int i = 1;i<=cur_size;i++){
      //   cout<<up_page->key_array_[i]<<",";
      // }
      // cout<<endl;
      // for(int i = 1;i<=cur_size;i++){
      //   cout<<up_page->page_id_array_[i-1]<<",";
      // }
      // cout<<up_page->page_id_array_[cur_size]<<endl;


  int i = 1;
  auto& key_array = up_page->key_array_;
  auto& page_id_array = up_page->page_id_array_;
  for(;i <= cur_size && comparator_(right_key, key_array[i]) >= 0;i++) {;}
  
  //将内部节点往后挪
  for(int j = cur_size + 1; j > i ;j --){
    key_array[j] = std::move(key_array[j-1]);
    page_id_array[j] = page_id_array[j-1];
  }

  key_array[i] = std::move(right_key);
  page_id_array[i - 1] = left_page;
  page_id_array[i] = right_page;

  up_page->ChangeSizeBy(1);
      
      cur_size+=1;
      // for(int i = 1;i<=cur_size;i++){
      //   cout<<up_page->key_array_[i]<<",";
      // }
      // cout<<endl;
      // for(int i = 1;i<=cur_size;i++){
      //   cout<<up_page->page_id_array_[i-1]<<",";
      // }
      // cout<<up_page->page_id_array_[cur_size]<<endl;
  return true;
}


INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::InsertLeaf(LeafPage* leaf_page, const KeyType &key, const ValueType &value, std::shared_ptr<Context> context, page_id_t leaf_page_id) -> bool {
    //cout<<"Insert "<<key<<endl;
    int cur_size = leaf_page->GetSize();
    if(cur_size == leaf_page->GetMaxSize()){
      //特判节点相等情况
      for(int i = 0; i < cur_size; i ++){
        if(comparator_(key, leaf_page->key_array_[i]) == 0){
          return false;
        }
      }

      //叶子节点溢出情况   
      page_id_t new_leaf_page_id = bpm_->NewPage();
      auto new_leaf_page_guard = bpm_->WritePage(new_leaf_page_id);
      auto new_leaf_page = new_leaf_page_guard.AsMut
        <BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>();

      new_leaf_page->Init(leaf_max_size_);
      // new_leaf_page->next_page_id_ = leaf_page_id;
      // if(first_leaf_page_id_ == leaf_page_id){
      //   first_leaf_page_id_ = new_leaf_page_id;
      // }
      //设置迭代器的下一页

      int i = 0;
      int ceil = cur_size / 2;
      int left_or_right = (comparator_(key, leaf_page->key_array_[ceil-1]) > 0) ? 1 : 0;
      //决定新key插入在左边还是右边;
      //左边0右边1

      //新页存放大的key
      int already_pushed = 0;
      for(; i < cur_size-ceil; i ++ ){
        if(left_or_right==1 && already_pushed == 0 && comparator_(key, leaf_page->key_array_[ceil + i]) < 0){
          new_leaf_page->key_array_[i] = std::move(key);
          new_leaf_page->rid_array_[i] = std::move(value);
          already_pushed = 1;
        }
        assert(ceil+i < cur_size);
        new_leaf_page->key_array_[i + already_pushed] = std::move(leaf_page->key_array_[ceil + i]);
        new_leaf_page->rid_array_[i + already_pushed] = std::move(leaf_page->rid_array_[ceil + i]);   
      }
      //特判插入值是左边最大值
      if(left_or_right == 1 && already_pushed == 0){
        new_leaf_page->key_array_[i] = std::move(key);
        new_leaf_page->rid_array_[i] = std::move(value);
        already_pushed = 1;
      }

      //将一半（向上取整）的节点复制到新节点中
      new_leaf_page->ChangeSizeBy(i + left_or_right);

      //将后面的节点挪到前面
      already_pushed = 0;//没有用

      if(left_or_right == 0){
        int j = 0;
        auto& key_array = leaf_page->key_array_;
        auto& rid_array = leaf_page->rid_array_;
        for(; j < ceil && comparator_(key, key_array[j]) > 0; j ++) {;}

        //将i以后的键值对往后挪一格
        for(int k = ceil; k > j; k--){
          key_array[k] = std::move(key_array[k-1]);
          rid_array[k] = std::move(rid_array[k-1]);
        }

        key_array[j] = std::move(key);
        rid_array[j] = std::move(value);

      }

      leaf_page->ChangeSizeBy(-i + 1-left_or_right);

      size_ ++;
      
      
      return UpInsert(context, leaf_page_id, new_leaf_page_id, new_leaf_page->key_array_[0]);
      //如果context中无内容就代表是根节点要替换根节点
    }

    int i = 0;    
    auto& key_array = leaf_page->key_array_;
    auto& rid_array = leaf_page->rid_array_;
    for(; i < cur_size && comparator_(key, key_array[i]) > 0; i ++) {;}
    if(comparator_(key, key_array[i]) == 0 && cur_size != 0){
      //二者等于
      return false;
    }

    //将i以后的键值对往后挪一格
    for(int j = cur_size; j > i; j--){
      key_array[j] = std::move(key_array[j-1]);
      rid_array[j] = std::move(rid_array[j-1]);
    }

    leaf_page->ChangeSizeBy(1);
    key_array[i] = std::move(key);
    rid_array[i] = std::move(value);

    size_ ++;
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
  std::shared_ptr<Context> context = std::make_shared<Context>();
  WritePageGuard header_guard = bpm_->WritePage(header_page_id_);
  auto header_page = header_guard.AsMut<BPlusTreeHeaderPage>();
  page_id_t root_page_id = INVALID_PAGE_ID;

  //获得root_page_id
  if(header_page->root_page_id_ == INVALID_PAGE_ID){
    //当前树无根节点
    root_page_id = bpm_->NewPage();
    header_page->root_page_id_ = root_page_id;
    first_leaf_page_id_ = root_page_id;
    //header_guard.Drop();
    WritePageGuard root_page_guard = bpm_->WritePage(root_page_id);
    auto root_page = root_page_guard.AsMut<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>();
    root_page->Init(leaf_max_size_);
    root_page->next_page_id_ = INVALID_PAGE_ID;
    //设置root_page基本类型
  }else{
    root_page_id = header_page->root_page_id_;
    //header_guard.Drop();
  }

  context->header_page_ = std::move(header_guard);//只能通过右值拷贝
  context->root_page_id_ = root_page_id;

  return PageInsert(root_page_id, key, value, context);
  // WritePageGuard root_guard = bpm_->WritePage(root_page_id);
  // auto root_page = root_guard.AsMut<BPlusTreePage>();

  // if(root_page->IsLeafPage()){
  //   auto leaf_page = root_guard.AsMut<LeafPage>();
  //   return InsertLeaf(leaf_page, key, value);
  // }
  // if(root_page->IsInternalPage()){
  //   //内部节点向下遍历
  // }
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::RemoveLeaf(LeafPage* leaf_page, const KeyType &key, const std::shared_ptr<Context>& context, page_id_t leaf_page_id)  {
  int cur_size = leaf_page->GetSize();
  // if(cur_size <= leaf_page->GetMaxSize() / 2){
  //   //节点半满
  // }

  int i = 0;    
  auto& key_array = leaf_page->key_array_;
  auto& rid_array = leaf_page->rid_array_;
  for(; i < cur_size && comparator_(key, key_array[i]) > 0; i ++) {;}
  if(comparator_(key, key_array[i]) != 0){
    //无要删除键
    return;
  }


  //将i以后得键往前挪
  for(int j = i; j < cur_size - 1; j++){
    key_array[j] = key_array[j+1];
    rid_array[j] = std::move(rid_array[j+1]);
  }

  leaf_page->ChangeSizeBy(-1);
  size_--;

  if(size_ == 0){
    context->header_page_.value().AsMut<BPlusTreeHeaderPage>()->root_page_id_ = INVALID_PAGE_ID;
  }
}


INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::PageRemove(page_id_t cur_page_id, const KeyType& key,const std::shared_ptr<Context>& context) {
  if(cur_page_id == INVALID_PAGE_ID){
    return;
  }
  WritePageGuard cur_page_guard = bpm_->WritePage(cur_page_id);
  auto cur_page = cur_page_guard.As
    <BPlusTreePage>();
  
  if(cur_page->IsLeafPage()){
    auto leaf_page = cur_page_guard.AsMut<BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>>();
    RemoveLeaf(leaf_page, key, context, cur_page_id);
  }

  if(cur_page->IsInternalPage()){
    auto internal_page = cur_page_guard.As
                        <BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
    page_id_t next_page_id = INVALID_PAGE_ID;
    int i = 1;
    auto& key_array = internal_page->key_array_;
    for(; i <= internal_page->GetSize() && comparator_(key, key_array[i]) >= 0; i ++){;}
    next_page_id = internal_page->page_id_array_[i-1];

    context->write_set_.push_front(std::move(cur_page_guard));
    PageRemove(next_page_id, key, context);
  }

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
  WritePageGuard header_page_guard = bpm_->WritePage(header_page_id_);
  auto header_page = header_page_guard.As<BPlusTreeHeaderPage>();
  std::shared_ptr<Context> context = std::make_shared<Context>();
  context->root_page_id_ = header_page->root_page_id_;
  context->header_page_ = std::move(header_page_guard);//只能通过右值拷贝

  
  return PageRemove(header_page->root_page_id_, key, context);
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
auto BPLUSTREE_TYPE::Begin() -> INDEXITERATOR_TYPE { 
  return INDEXITERATOR_TYPE();
}

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
