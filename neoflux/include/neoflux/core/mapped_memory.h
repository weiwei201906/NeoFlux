// =============================================================================
// NeoFlux - mapped_memory.h
//
// RAII wrapper for page-aligned memory mapping with an optional guard page.
//
// On POSIX (Linux/macOS/Android/iOS): uses mmap(MAP_ANONYMOUS) + mprotect.
// On Windows: uses VirtualAlloc + VirtualProtect.
//
// The guard page is a single page of PROT_NONE / PAGE_NOACCESS placed
// immediately after the usable region. Any out-of-bounds write that reaches
// the guard page triggers an immediate access fault (SIGSEGV / access
// violation) instead of silently corrupting adjacent memory.
//
// Usage:
//   MappedMemory mem(1024 * sizeof(int), true);  // 4KB + guard page
//   int* data = static_cast<int*>(mem.Data());
//   data[0] = 42;  // OK
//   data[1024] = 0;  // CRASH: hits guard page
// =============================================================================

#ifndef NEOFLUX_NATIVE_MAPPED_MEMORY_H_
#define NEOFLUX_NATIVE_MAPPED_MEMORY_H_

#include <cstddef>
#include <optional>
#include <string_view>

namespace neoflux {

// RAII memory mapping with optional guard page.
class MappedMemory {
 public:
  // Allocates 'size' bytes of readable/writable memory.
  // If guard_page is true, an additional inaccessible page is placed after
  // the usable region to catch out-of-bounds accesses.
  // The usable region is page-aligned.
  explicit MappedMemory(std::size_t size, bool guard_page = true);

  // Default constructor: creates an empty mapping (Data() == nullptr).
  // Used by FromFile() and move operations.
  MappedMemory() = default;

  // Memory-maps an entire file for reading. Returns std::nullopt if the
  // file cannot be opened or mapped. The mapping is read-only.
  // POSIX: mmap(PROT_READ, MAP_PRIVATE) on the file descriptor.
  // Windows: CreateFileMapping + MapViewOfFile(FILE_MAP_READ).
  [[nodiscard]] static std::optional<MappedMemory> FromFile(
      std::string_view path);

  // Releases the mapping.
  ~MappedMemory();

  // Non-copyable.
  MappedMemory(const MappedMemory&) = delete;
  MappedMemory& operator=(const MappedMemory&) = delete;

  // Movable (like std::unique_ptr): transfers ownership of the mapping.
  MappedMemory(MappedMemory&& other) noexcept;
  MappedMemory& operator=(MappedMemory&& other) noexcept;

  // Returns pointer to the start of the usable (read/write) region.
  [[nodiscard]] void* Data() noexcept;

  // Returns const pointer to the usable region.
  [[nodiscard]] const void* Data() const noexcept;

  // Returns the usable size in bytes (excluding guard page).
  [[nodiscard]] std::size_t Size() const noexcept;

  // Returns the system page size in bytes.
  [[nodiscard]] static std::size_t PageSize() noexcept;

 private:
  void* mapping_ = nullptr;   // Start of the full mapping (usable + guard).
  std::size_t usable_size_ = 0;  // Usable bytes (excluding guard page).
  std::size_t total_size_ = 0;   // Total mapped bytes (usable + guard).
  bool has_guard_ = false;
  bool is_file_mapping_ = false;  // True for file-backed mappings (Windows
                                  // needs UnmapViewOfFile vs VirtualFree).
};

}  // namespace neoflux

#endif  // NEOFLUX_NATIVE_MAPPED_MEMORY_H_
