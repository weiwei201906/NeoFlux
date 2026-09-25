// =============================================================================
// NeoFlux - macros.h
//
// Project-wide utility macros.
// =============================================================================

#ifndef NEOFLUX_CORE_MACROS_H_
#define NEOFLUX_CORE_MACROS_H_

// Marks a function parameter as intentionally unused.
#define NEOFLUX_UNUSED(x) (void)(x)

// Stringifies a macro argument.
#define NEOFLUX_STRINGIFY_INNER(x) #x
#define NEOFLUX_STRINGIFY(x) NEOFLUX_STRINGIFY_INNER(x)

// Compiler-specific branch prediction hints.
#if defined(__GNUC__) || defined(__clang__)
#define NEOFLUX_LIKELY(x) __builtin_expect(!!(x), 1)
#define NEOFLUX_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define NEOFLUX_LIKELY(x) (x)
#define NEOFLUX_UNLIKELY(x) (x)
#endif

// Forces a function to be inlined.
#if defined(__GNUC__) || defined(__clang__)
#define NEOFLUX_ALWAYS_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define NEOFLUX_ALWAYS_INLINE __forceinline
#else
#define NEOFLUX_ALWAYS_INLINE inline
#endif

#endif  // NEOFLUX_CORE_MACROS_H_
