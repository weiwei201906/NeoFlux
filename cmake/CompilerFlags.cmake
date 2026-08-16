# =============================================================================
# Compiler-specific warning and optimization flags for NeoFlux
# =============================================================================

if(MSVC)
  # MSVC (Windows)
  add_compile_options(
    /W4
    /w14242  # conversion from 'type1' to 'type2', possible loss of data
    /w14254  # operator: conversion from 'type1' to 'type2', possible loss of data
    /w14263  # member function does not override any base class virtual member
    /w14265  # class has virtual functions, but destructor is not virtual
    /w14287  # unsigned/negative constant mismatch
    /we4289  # nonstandard extension used: loop control variable used in outer scope
    /w14296  # expression is always false
    /w14311  # pointer truncation from 'type' to 'type'
    /w14545  # expression before comma evaluates to a function which is missing an argument list
    /w14546  # function call before comma missing argument list
    /w14547  # operator before comma has no effect
    /w14549  # operator before comma has no effect
    /w14555  # expression has no effect
    /w14619  # pragma warning: there is no warning number
    /w14640  # thread un-safe static member initialization
    /w14826  # Conversion is sign-extended
    /w14905  # wide string literal cast to LPSTR
    /w14906  # string literal cast to LPWSTR
    /w14928  # illegal copy-initialization
    /permissive-
    /Zc:__cplusplus
    /Zc:preprocessor
    /utf-8
  )
  # Disable specific noisy warnings
  add_compile_options(/wd4127)  # conditional expression is constant
  add_compile_options(/wd4251)  # class needs to have dll-interface
else()
  # GCC / Clang (Linux, macOS, MinGW)
  add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    -Wimplicit-fallthrough
  )

  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(
      -Wmisleading-indentation
      -Wduplicated-cond
      -Wduplicated-branches
      -Wlogical-op
      -Wuseless-cast
    )
  endif()

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
      -Wno-c++98-compat
      -Wno-c++98-compat-pedantic
      -Wno-ctad-maybe-unsupported
    )
  endif()
endif()

# ---------------------------------------------------------------------------
# Build-type specific flags
# ---------------------------------------------------------------------------
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG" CACHE STRING "" FORCE)
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "" FORCE)
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG" CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------------------
# Sanitizer support (optional)
# ---------------------------------------------------------------------------
option(NEOFLUX_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(NEOFLUX_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(NEOFLUX_ENABLE_ASAN AND NOT MSVC)
  message(STATUS "AddressSanitizer enabled")
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address)
endif()

if(NEOFLUX_ENABLE_TSAN AND NOT MSVC)
  message(STATUS "ThreadSanitizer enabled")
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()
