// =============================================================================
// NeoFlux - object_pool.cpp
//
// Template implementation of ObjectPool and explicit instantiations for the
// types used by the framework and tests. The template method definitions
// live here (not in a separate .inc); to use ObjectPool with your own type,
// add an explicit instantiation for that type in one of your translation
// units and link against neoflux.
// =============================================================================

#include "neoflux/core/object_pool.h"

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace neoflux {

template <typename T>
ObjectPool<T>::ObjectPool(std::size_t max_pool_size)
    : max_pool_size_(max_pool_size) {
  pool_.reserve(max_pool_size);
}

template <typename T>
ObjectPool<T>::~ObjectPool() = default;  // unique_ptr elements destroy automatically.

template <typename T>
std::shared_ptr<T> ObjectPool<T>::Acquire() {
  std::unique_ptr<T> obj;
  {
    std::scoped_lock<std::mutex> lock(mutex_);
    if (!pool_.empty()) {
      obj = std::move(pool_.back());
      pool_.pop_back();
    }
  }

  if (obj == nullptr) {
    obj = std::make_unique<T>();
  }

  // Custom deleter: returns the object to this pool when the last shared_ptr
  // reference is released. Uses a raw pointer to the pool; the pool must
  // outlive all acquired objects (documented constraint).
  T* raw = obj.release();
  return std::shared_ptr<T>(raw, [this](T* ptr) { this->Release(ptr); });
}

template <typename T>
void ObjectPool<T>::Release(T* obj) {
  if (obj == nullptr) {
    return;
  }
  std::unique_ptr<T> managed(obj);
  std::scoped_lock<std::mutex> lock(mutex_);
  if (pool_.size() < max_pool_size_) {
    pool_.push_back(std::move(managed));
  }
  // If pool is full, managed goes out of scope and destroys the object.
}

template <typename T>
std::size_t ObjectPool<T>::Size() const {
  std::scoped_lock<std::mutex> lock(mutex_);
  return pool_.size();
}

template <typename T>
std::size_t ObjectPool<T>::MaxSize() const noexcept {
  return max_pool_size_;
}

template <typename T>
void ObjectPool<T>::Clear() {
  std::scoped_lock<std::mutex> lock(mutex_);
  pool_.clear();
}

// Explicit instantiations used by tests and generic workloads.
template class ObjectPool<int>;
template class ObjectPool<std::string>;

}  // namespace neoflux
