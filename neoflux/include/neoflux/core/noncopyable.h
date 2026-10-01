// =============================================================================
// NeoFlux - noncopyable.h
//
// Base class for types that should not be copied or moved.
// =============================================================================

#ifndef NEOFLUX_CORE_NONCOPYABLE_H_
#define NEOFLUX_CORE_NONCOPYABLE_H_

namespace neoflux {

// Non-copyable, non-movable base class.
//
// Inherit privately from this class to prohibit copy and move semantics.
// Uses the C++20 preferred form of explicitly deleted special members.
class NonCopyable {
 public:
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
  NonCopyable(NonCopyable&&) = delete;
  NonCopyable& operator=(NonCopyable&&) = delete;

 protected:
  NonCopyable() = default;
  ~NonCopyable() = default;
};

}  // namespace neoflux

#endif  // NEOFLUX_CORE_NONCOPYABLE_H_
