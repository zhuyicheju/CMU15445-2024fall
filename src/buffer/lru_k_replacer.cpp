//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include <cstddef>
#include <exception>
#include <optional>
#include "common/config.h"
#include "common/exception.h"
#include "common/macros.h"
#include <chrono>
#include <stdexcept>

namespace bustub {

/**
 *
 * TODO(P1): Add implementation
 *
 * @brief a new LRUKReplacer.
 * @param num_frames the maximum number of frames the LRUReplacer will be required to store
 */
LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

/**
 * TODO(P1): Add implementation
 *
 * @brief Find the frame with largest backward k-distance and evict that frame. Only frames
 * that are marked as 'evictable' are candidates for eviction.
 *
 * A frame with less than k historical references is given +inf as its backward k-distance.
 * If multiple frames have inf backward k-distance, then evict frame whose oldest timestamp
 * is furthest in the past.
 *
 * Successful eviction of a frame should decrement the size of replacer and remove the frame's
 * access history.
 *
 * @return true if a frame is evicted successfully, false if no frames can be evicted.
 */
auto LRUKReplacer::Evict() -> std::optional<frame_id_t> { 
    std::optional<frame_id_t> frame = std::nullopt;
    size_t k_timestrap = 0xffffffffff3f3f3f;
    size_t lru_timestrap = 0xffffffffff3f3f3f;
    for (auto& [current_frame, node] : node_store_){
        if(!node.is_evictable_){
            continue;
        }
        auto distance = node.history_.back();
        if(node.history_.size() < k_){
            k_timestrap = 0;
            if(lru_timestrap > distance){
                lru_timestrap = distance;
                frame = current_frame;
            }
        }else{
            if(k_timestrap > distance){
                k_timestrap = distance;
                frame = current_frame;
            }
        }
    }
    if(frame.has_value()){
        node_store_.erase(frame.value());
        curr_size_--;
    }
    return frame;
 }

/**
 * TODO(P1): Add implementation
 *
 * @brief Record the event that the given frame id is accessed at current timestamp.
 * Create a new entry for access history if frame id has not been seen before.
 *
 * If frame id is invalid (ie. larger than replacer_size_), throw an exception. You can
 * also use BUSTUB_ASSERT to abort the process if frame id is invalid.
 *
 * @param frame_id id of frame that received a new access.
 * @param access_type type of access that was received. This parameter is only needed for
 * leaderboard tests.
 */
void LRUKReplacer::RecordAccess(frame_id_t frame_id, [[maybe_unused]] AccessType access_type) {
    BUSTUB_ASSERT(static_cast<size_t>(frame_id) <= replacer_size_, "frame id is invalid");

    auto iter = node_store_.find(frame_id);
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    size_t timestrap = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

    if(iter == node_store_.end()){
      auto placeholder = LRUKNode();
      placeholder.is_evictable_ = false;
      placeholder.fid_ = frame_id;
      placeholder.history_.emplace_front(timestrap);
      placeholder.k_ = k_;
      node_store_[frame_id] = std::move(placeholder);
    }else{
      iter->second.history_.emplace_front(timestrap);
      if(iter->second.history_.size() > k_){
        iter->second.history_.pop_back();
      }
    }
    // if(iter->second.history_.size() > k_){
    //     iter->second.history_.pop_back();
    // }
    //former here
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Toggle whether a frame is evictable or non-evictable. This function also
 * controls replacer's size. Note that size is equal to number of evictable entries.
 *
 * If a frame was previously evictable and is to be set to non-evictable, then size should
 * decrement. If a frame was previously non-evictable and is to be set to evictable,
 * then size should increment.
 *
 * If frame id is invalid, throw an exception or abort the process.
 *
 * For other scenarios, this function should terminate without modifying anything.
 *
 * @param frame_id id of frame whose 'evictable' status will be modified
 * @param set_evictable whether the given frame is evictable or not
 */
void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
    BUSTUB_ASSERT(static_cast<size_t>(frame_id) <= replacer_size_, "frame id is invalid");
    auto iter = node_store_.find(frame_id);
    if(iter == node_store_.end()){
        return;
    }

    bool& former = iter->second.is_evictable_;
    if(former ^ set_evictable){
        former = set_evictable;
        if(set_evictable){
            curr_size_++;
        }else{
            curr_size_--;
        }
    }
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Remove an evictable frame from replacer, along with its access history.
 * This function should also decrement replacer's size if removal is successful.
 *
 * Note that this is different from evicting a frame, which always remove the frame
 * with largest backward k-distance. This function removes specified frame id,
 * no matter what its backward k-distance is.
 *
 * If Remove is called on a non-evictable frame, throw an exception or abort the
 * process.
 *
 * If specified frame is not found, directly return from this function.
 *
 * @param frame_id id of frame to be removed
 */
void LRUKReplacer::Remove(frame_id_t frame_id) {
    BUSTUB_ASSERT(static_cast<size_t>(frame_id) <= replacer_size_, "frame id is invalid");
    auto iter = node_store_.find(frame_id);
    if(iter == node_store_.end()) {
        return;
    }

    if(!iter->second.is_evictable_){
        throw std::invalid_argument("Remove is called on a non-evictable frame");
    }

    node_store_.erase(iter);
    curr_size_--;
}

/**
 * TODO(P1): Add implementation
 *
 * @brief Return replacer's size, which tracks the number of evictable frames.
 *
 * @return size_t
 */
auto LRUKReplacer::Size() -> size_t { 
    return curr_size_;
}

}  // namespace bustub
