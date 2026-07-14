#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace neoflux::utils {

class MemoryPool {
 public:
  explicit MemoryPool(const std::size_t blockSize = 4096) : blockSize_(blockSize), data_(new std::uint8_t[blockSize]) {}

  ~MemoryPool() = default;

  [[nodiscard]] void* allocate(std::size_t size) const;
  void deallocate(void* pointer, std::size_t size) const;

 private:
  std::size_t blockSize_;
  std::unique_ptr<std::uint8_t[]> data_;
};

}  // namespace neoflux::utils