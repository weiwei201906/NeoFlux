// =============================================================================
// NeoFlux - mapped_memory.cpp
//
// Cross-platform implementation of MappedMemory (mmap + guard page).
//
// POSIX: mmap(MAP_PRIVATE | MAP_ANONYMOUS) + mprotect(PROT_NONE) for guard.
// Windows: VirtualAlloc(MEM_COMMIT | MEM_RESERVE) + VirtualProtect(PAGE_NOACCESS).
// =============================================================================

#include "neoflux/core/mapped_memory.h"

#include <algorithm>
#include <cstddef>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
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
#ifdef _WIN32
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

#ifdef _WIN32
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

MappedMemory::MappedMemory(MappedMemory&& other) noexcept
    : mapping_(other.mapping_),
      usable_size_(other.usable_size_),
      total_size_(other.total_size_),
      has_guard_(other.has_guard_),
      is_file_mapping_(other.is_file_mapping_) {
  other.mapping_ = nullptr;
  other.usable_size_ = 0;
  other.total_size_ = 0;
}

MappedMemory& MappedMemory::operator=(MappedMemory&& other) noexcept {
  if (this != &other) {
    // Release current mapping first.
    if (mapping_ != nullptr) {
#ifdef _WIN32
      if (is_file_mapping_) {
        UnmapViewOfFile(mapping_);
      } else {
        VirtualFree(mapping_, 0, MEM_RELEASE);
      }
#else
      munmap(mapping_, total_size_);
#endif
    }
    mapping_ = other.mapping_;
    usable_size_ = other.usable_size_;
    total_size_ = other.total_size_;
    has_guard_ = other.has_guard_;
    is_file_mapping_ = other.is_file_mapping_;
    other.mapping_ = nullptr;
    other.usable_size_ = 0;
    other.total_size_ = 0;
  }
  return *this;
}

MappedMemory::~MappedMemory() {
  if (mapping_ == nullptr) {
    return;
  }
#ifdef _WIN32
  if (is_file_mapping_) {
    UnmapViewOfFile(mapping_);
  } else {
    VirtualFree(mapping_, 0, MEM_RELEASE);
  }
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

std::optional<MappedMemory> MappedMemory::FromFile(std::string_view path) {
  if (path.empty()) {
    return std::nullopt;
  }

#ifdef _WIN32
  // Open file for reading.
  const HANDLE file_handle = CreateFileA(
      path.data(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file_handle == INVALID_HANDLE_VALUE) {
    return std::nullopt;
  }

  // Get file size.
  LARGE_INTEGER file_size;
  if (!GetFileSizeEx(file_handle, &file_size) || file_size.QuadPart <= 0) {
    CloseHandle(file_handle);
    return std::nullopt;
  }
  const auto size = static_cast<std::size_t>(file_size.QuadPart);

  // Create file mapping object.
  const HANDLE mapping_handle = CreateFileMappingA(
      file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
  CloseHandle(file_handle);  // Mapping holds a reference; file handle no longer needed.
  if (mapping_handle == nullptr) {
    return std::nullopt;
  }

  // Map the entire file into memory (read-only).
  void* addr = MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, size);
  CloseHandle(mapping_handle);  // View holds a reference; mapping handle no longer needed.
  if (addr == nullptr) {
    return std::nullopt;
  }

  MappedMemory result;
  result.mapping_ = addr;
  result.usable_size_ = size;
  result.total_size_ = size;
  result.has_guard_ = false;
  result.is_file_mapping_ = true;
  return result;
#else
  // Open file for reading.
  const int fd = open(path.data(), O_RDONLY);
  if (fd < 0) {
    return std::nullopt;
  }

  // Get file size.
  struct stat st;
  if (fstat(fd, &st) != 0 || st.st_size <= 0) {
    close(fd);
    return std::nullopt;
  }
  const auto size = static_cast<std::size_t>(st.st_size);

  // Map the entire file (read-only, private copy-on-write).
  void* addr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);  // Mapping holds a reference; file descriptor no longer needed.
  if (addr == MAP_FAILED) {
    return std::nullopt;
  }

  MappedMemory result;
  result.mapping_ = addr;
  result.usable_size_ = size;
  result.total_size_ = size;
  result.has_guard_ = false;
  result.is_file_mapping_ = false;  // POSIX: munmap works for both anon and file
  return result;
#endif
}

}  // namespace neoflux
