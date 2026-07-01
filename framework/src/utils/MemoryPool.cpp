#include <cstddef>
#include <memory>

namespace neoflux {
namespace utils {

class MemoryPool {
 public:
  explicit MemoryPool(std::size_t blockSize = 4096)
      : blockSize_(blockSize), data_(new std::uint8_t[blockSize]) {}

  ~MemoryPool() = default;

  void* allocate(std::size_t size) {
    if (size > blockSize_) {
      return ::operator new(size);
    }
    return data_.get();
  }

  void deallocate(void* pointer, std::size_t size) {
    if (pointer && size > blockSize_) {
      ::operator delete(pointer);
    }
  }

 private:
  std::size_t blockSize_;
  std::unique_ptr<std::uint8_t[]> data_;
};

} // namespace utils
} // namespace neoflux
