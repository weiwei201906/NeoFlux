// =============================================================================
// NeoFlux - mapped_memory.cpp
//
// Cross-platform implementation of MappedMemory (mmap + guard page).
//
// POSIX: mmap(MAP_PRIVATE | MAP_ANONYMOUS) + mprotect(PROT_NONE) for guard.
// Windows: VirtualAlloc(MEM_COMMIT | MEM_RESERVE) + VirtualProtect(PAGE_NOACCESS).
// =============================================================================

#include "neoflux/native/mapped_memory.h"

#include <algorithm>
#include <cstddef>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace neoflux {

namespace {

// Rounds 'value' up to the next multiple of 'alignment' (must be power of 2).
// Uses bitwise arithmetic: (value + align - 1) & ~(align - 1).
std::size_t AlignUp(std::size_t value, std::size_t alignment) noexcept {
  return (value + alignment - 1U) & ~(alignment - 1U);
}

}  // namespace

std::size_t MappedMemory::PageSize() noexcept {
#if defined(_WIN32)
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return static_cast<std::size_t>(si.dwPageSize);
#else
  const long ps = sysconf(_SC_PAGESIZE);
  return (ps > 0) ? static_cast<std::size_t>(ps) : static_cast<std::size_t>(4096);
#endif
}

MappedMemory::MappedMemory(std::size_t size, bool guard_page)
    : usable_size_(size), has_guard_(guard_page) {
  const std::size_t page_size = PageSize();
  // Round usable size up to page boundary so the guard page starts on a
  // page boundary (mprotect/VirtualProtect require page-aligned addresses).
  const std::size_t aligned_usable = AlignUp(size, page_size);
  total_size_ = aligned_usable + (guard_page ? page_size : 0U);

  if (total_size_ == 0U) {
    mapping_ = nullptr;
    return;
  }

#if defined(_WIN32)
  // VirtualAlloc returns page-aligned memory. MEM_COMMIT | MEM_RESERVE
  // allocates and commits in one call.
  mapping_ = VirtualAlloc(nullptr, total_size_, MEM_COMMIT | MEM_RESERVE,
                          PAGE_READWRITE);
  if (mapping_ == nullptr) {
    return;
  }
  if (guard_page) {
    // Guard page starts immediately after the usable region.
    void* guard_addr = static_cast<char*>(mapping_) + aligned_usable;
    DWORD old_protect = 0;
    VirtualProtect(guard_addr, page_size, PAGE_NOACCESS, &old_protect);
  }
#else
  // MAP_PRIVATE | MAP_ANONYMOUS: private anonymous mapping (not backed by
  // any file). PROT_READ | PROT_WRITE for the full region initially.
  mapping_ = mmap(nullptr, total_size_, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mapping_ == MAP_FAILED) {
    mapping_ = nullptr;
    return;
  }
  if (guard_page) {
    void* guard_addr = static_cast<char*>(mapping_) + aligned_usable;
    mprotect(guard_addr, page_size, PROT_NONE);
  }
#endif
}

MappedMemory::~MappedMemory() {
  if (mapping_ == nullptr) {
    return;
  }
#if defined(_WIN32)
  VirtualFree(mapping_, 0, MEM_RELEASE);
#else
  munmap(mapping_, total_size_);
#endif
  mapping_ = nullptr;
}

void* MappedMemory::Data() noexcept {
  return mapping_;
}

const void* MappedMemory::Data() const noexcept {
  return mapping_;
}

std::size_t MappedMemory::Size() const noexcept {
  return usable_size_;
}

}  // namespace neoflux
