// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors

// Does the current destructor really require default-constructibility?
#include <cstdio>
#include <cstddef>
#include <vector>
#include "neoflux/core/ring_queue.h"
using namespace neoflux;

// A type that is move-constructible but NOT default-constructible,
// exactly what the class-level static_assert claims to accept.
struct NoDefault {
  int v;
  explicit NoDefault(int x) : v(x) {}
  NoDefault(const NoDefault&) = default;
  NoDefault(NoDefault&&) noexcept = default;
  NoDefault& operator=(const NoDefault&) = default;
  NoDefault& operator=(NoDefault&&) noexcept = default;
};

int main() {
  SpscRingQueue<NoDefault> q(8);
  q.TryPush(NoDefault{1});
  NoDefault out{0};
  q.TryPop(out);
  std::printf("pushed/popped ok, v=%d\n", out.v);
}
