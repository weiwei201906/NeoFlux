#include "neoflux/utils/MemoryPool.h"
#include <cstddef>
#include <memory>

namespace neoflux::utils {

  void* MemoryPool::allocate(const std::size_t size) const {
    if (size > blockSize_) {
      return ::operator new(size);
    }
    return data_.get();
  }

  void MemoryPool::deallocate(void* pointer, const std::size_t size) const {
    if (pointer && size > blockSize_) {
      ::operator delete(pointer);
    }
  }

} // namespace neoflux::utils