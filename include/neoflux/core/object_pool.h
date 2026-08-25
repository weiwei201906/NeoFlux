// =============================================================================
// NeoFlux - object_pool.h
//
// Generic thread-safe object pool with shared_ptr integration. Acquired objects
// are returned to the pool automatically when their shared_ptr reference count
// reaches zero (via a custom deleter). Pool overflow objects are destroyed
// instead of stored, bounding memory usage.
//
// Usage:
//   ObjectPool<MyWidget> pool(64);  // max 64 cached objects
//   auto w = pool.Acquire();        // from pool or new
//   w->SetSomething(...);
//   // w goes out of scope -> returned to pool automatically
//
// Template implementation is in object_pool_impl.inc.
// =============================================================================

#ifndef NEOFLUX_CORE_OBJECT_POOL_H_
#define NEOFLUX_CORE_OBJECT_POOL_H_

#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace neoflux {

// Generic object pool for types with a default constructor.
//
// Acquired shared_ptrs use a custom deleter that returns the object to the
// pool when the last reference is released. If the pool is full (>= max_size),
// the object is destroyed instead.
//
// Thread safety: All public methods are thread-safe (internal mutex).
//
// Template parameter T must be default-constructible.
template <typename T>
class ObjectPool {
 public:
  // Creates a pool with the given maximum number of cached objects.
  explicit ObjectPool(std::size_t max_pool_size = 64);

  ~ObjectPool();

  // Non-copyable, non-movable (pool identity is stable for deleter references).
  ObjectPool(const ObjectPool&) = delete;
  ObjectPool& operator=(const ObjectPool&) = delete;
  ObjectPool(ObjectPool&&) = delete;
  ObjectPool& operator=(ObjectPool&&) = delete;

  // Acquires an object from the pool. If the pool is empty, a new object is
  // default-constructed. The returned shared_ptr automatically returns the
  // object to the pool when its reference count reaches zero.
  [[nodiscard]] std::shared_ptr<T> Acquire();

  // Returns the current number of objects in the pool.
  [[nodiscard]] std::size_t Size() const;

  // Returns the maximum number of objects the pool will cache.
  [[nodiscard]] std::size_t MaxSize() const noexcept;

  // Destroys all cached objects.
  void Clear();

 private:
  // Called by the shared_ptr deleter when an acquired object's last reference
  // is released. Returns the object to the pool if there is space, otherwise
  // destroys it.
  void Release(T* obj);

  std::vector<std::unique_ptr<T>> pool_;
  std::size_t max_pool_size_;
  mutable std::mutex mutex_;
};

}  // namespace neoflux

// Template implementation (must be visible at instantiation point).
#include "object_pool_impl.inc"

#endif  // NEOFLUX_CORE_OBJECT_POOL_H_
