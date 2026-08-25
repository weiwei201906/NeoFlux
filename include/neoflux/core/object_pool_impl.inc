// =============================================================================
// NeoFlux - object_pool_impl.inc
//
// Template implementation of ObjectPool. Included by object_pool.h via the
// explicit instantiation in object_pool.cpp, or directly by translation units
// that need a specific instantiation.
//
// The public declaration is in include/neoflux/core/object_pool.h.
// =============================================================================
#ifndef NEOFLUX_CORE_OBJECT_POOL_IMPL_INC_
#define NEOFLUX_CORE_OBJECT_POOL_IMPL_INC_

#include <utility>

namespace neoflux {

template <typename T>
ObjectPool<T>::ObjectPool(std::size_t max_pool_size)
    : max_pool_size_(max_pool_size) {
  pool_.reserve(max_pool_size);
}

template <typename T>
ObjectPool<T>::~ObjectPool() {
  // unique_ptr elements destroy automatically.
}

template <typename T>
std::shared_ptr<T> ObjectPool<T>::Acquire() {
  std::unique_ptr<T> obj;
  {
    std::lock_guard<std::mutex> lock(mutex_);
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
  std::lock_guard<std::mutex> lock(mutex_);
  if (pool_.size() < max_pool_size_) {
    pool_.push_back(std::move(managed));
  }
  // If pool is full, managed goes out of scope and destroys the object.
}

template <typename T>
std::size_t ObjectPool<T>::Size() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return pool_.size();
}

template <typename T>
std::size_t ObjectPool<T>::MaxSize() const noexcept {
  return max_pool_size_;
}

template <typename T>
void ObjectPool<T>::Clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  pool_.clear();
}

}  // namespace neoflux

#endif  // NEOFLUX_CORE_OBJECT_POOL_IMPL_INC_
