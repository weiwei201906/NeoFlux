// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors

// The destructor must destroy every live element (no leaks) and must not
// touch empty slots (no double-destroy). Track ctor/dtor counts.
#include <cstdio>
#include <atomic>
#include "neoflux/core/ring_queue.h"
using namespace neoflux;

static std::atomic<int> g_ctor{0}, g_dtor{0};
struct Tracked {
  int v;
  explicit Tracked(int x) : v(x) { g_ctor.fetch_add(1); }
  Tracked(const Tracked& o) : v(o.v) { g_ctor.fetch_add(1); }
  Tracked(Tracked&& o) noexcept : v(o.v) { g_ctor.fetch_add(1); }
  Tracked& operator=(const Tracked&) = default;
  Tracked& operator=(Tracked&&) noexcept = default;
  ~Tracked() { g_dtor.fetch_add(1); }
};

int main() {
  {
    SpscRingQueue<Tracked> q(8);
    const int pushed = 5;
    for (int i = 0; i < pushed; ++i) q.TryPush(Tracked{i});
    Tracked out{99};
    q.TryPop(out);      // one element popped and destroyed by TryPop
    std::printf("live in queue before dtor: %zu\n", q.Size());
  }   // destructor must destroy the remaining 4
  std::printf("ctor=%d dtor=%d\n", g_ctor.load(), g_dtor.load());
  if (g_ctor.load() != g_dtor.load()) {
    std::printf("[FAIL] leak: %d survived\n", g_ctor.load() - g_dtor.load());
    return 1;
  }
  std::printf("[PASS] every constructed element was destroyed\n");
  return 0;
}
