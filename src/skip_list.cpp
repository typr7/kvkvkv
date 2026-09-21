#include <cassert>
#include <utility>

#include "kv/skip_list.h"
#include "kv/internal_key.h"


namespace kv {

bool SkipList::Insert(InternalKey key, std::string value) {
  std::array<Node*, kMaxHeight> prev;
  prev.fill(&head_);

  Node* cur = &head_;
  for (int level = height_ - 1; level >= 0; level--) {
    while (cur->next[level] != nullptr && less_(cur->next[level]->key, key)) {
      cur = cur->next[level];
    }
    prev[level] = cur;
  }
  
  Node* candidate = cur->next[0];
  if (candidate && !less_(candidate->key, key) && !less_(key, candidate->key)) { // equal key
    return false;
  }

  auto node = std::make_unique<Node>(std::move(key), std::move(value));
  auto* raw = node.get();
  nodes_.push_back(std::move(node));

  int node_height = RandomHeight();
  for (int i = 0; i < node_height; i++) {
    raw->next[i] = prev[i]->next[i];
    prev[i]->next[i] = raw;
  }

  if (node_height > height_) {
    height_ = node_height;
  }

  return true;
}

const SkipList::Node* SkipList::LowerBound(const InternalKey& target) const {
  const Node* cur = &head_;

  for (int level = height_ - 1; level >= 0; level--) {
    while (cur->next[level] != nullptr && less_(cur->next[level]->key, target)) {
      cur = cur->next[level];
    }
  }

  return cur->next[0];
}

int SkipList::RandomHeight()
{
  int height = 1;

  while (height < kMaxHeight && (rng_() & 1u) != 0) {
    height++;
  }

  return height;
}

// SkipList::Iterator

bool SkipList::Iterator::Valid() const noexcept {
  return (cur_ != nullptr);
}

void SkipList::Iterator::SeekToBegin() noexcept {
  cur_ = list_->head_.next[0];
}

void SkipList::Iterator::Seek(const InternalKey& target) {
  cur_ = list_->LowerBound(target);
}

void SkipList::Iterator::Next() noexcept {
  if (Valid()) {
    cur_ = cur_->next[0];
  }
}

const InternalKey& SkipList::Iterator::Key() const {
  assert(Valid());
  return cur_->key;
}

const std::string& SkipList::Iterator::Value() const {
  assert(Valid());
  return cur_->value;
}

}