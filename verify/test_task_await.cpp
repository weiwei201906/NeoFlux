// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors

// Standalone verification of Task<T> await semantics after the P1-1 fix.
// Exercises: lazy start, nested co_await returning a value, completion
// ordering, and exception propagation.
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "neoflux/core/task.h"

using namespace neoflux;

// Provided by yield_stub.cpp: drains handles deferred by Yield()/Sleep(),
// modelling one event-loop frame.
extern "C" void DrainDeferredFrame();

static std::vector<std::string> g_log;

// A task that returns a value without suspending.
static Task<int> AddAsync(int a, int b) {
  g_log.push_back("AddAsync:start");
  co_return a + b;
}

// A task that awaits another task (nested co_await) and yields once.
static Task<int> NestedAsync(int a, int b) {
  g_log.push_back("Nested:start");
  const int sum = co_await AddAsync(a, b);  // must resume here with the value
  g_log.push_back("Nested:resumed-with-" + std::to_string(sum));
  co_await Yield();                          // suspend to the event loop
  g_log.push_back("Nested:after-yield");
  co_return sum * 2;
}

// A task that throws.
static Task<int> ThrowingAsync() {
  throw std::runtime_error("boom");
  co_return 0;
}

int main() {
  // --- Case 1: simple value-returning await ---
  {
    g_log.clear();
    Task<int> t = AddAsync(2, 3);
    assert(!t.Done() && "task must be lazy (not started before resume)");
    t.Resume();
    assert(t.Done() && "task must be done after a single resume");
    assert(t.Result() == 5);
    assert(g_log.size() == 1 && g_log[0] == "AddAsync:start");
    std::puts("[PASS] case 1: lazy start + value return");
  }

  // --- Case 2: nested co_await returns the inner value to the outer task ---
  {
    g_log.clear();
    Task<int> t = NestedAsync(4, 6);
    t.Resume();  // runs until co_await Yield()
    assert(!t.Done() && "task must be suspended at co_await Yield()");
    // Expected: outer starts, inner starts, outer resumed with inner's value.
    assert(g_log.size() == 3 && "outer must resume after the inner task ran");
    assert(g_log[2] == "Nested:resumed-with-10" &&
           "outer task must observe the inner task's return value");
    DrainDeferredFrame();  // next event-loop frame: resume past Yield()
    assert(t.Done());
    assert(t.Result() == 20);
    assert(g_log.size() == 4 && g_log[3] == "Nested:after-yield");
    std::puts("[PASS] case 2: nested co_await value + yield");
  }

  // --- Case 3: exception propagation ---
  {
    Task<int> t = ThrowingAsync();
    t.Resume();
    assert(t.Done());
    bool caught = false;
    try {
      (void)t.Result();
    } catch (const std::runtime_error& e) {
      caught = std::string(e.what()) == "boom";
    }
    assert(caught && "exception must propagate through Result()");
    std::puts("[PASS] case 3: exception propagation");
  }

  // --- Case 4: Task<void> ---
  {
    g_log.clear();
    auto v = []() -> Task<void> {
      co_await AddAsync(1, 1);
      g_log.push_back("void-task-done");
      co_return;
    }();
    v.Resume();
    assert(v.Done());
    v.Result();  // must not throw
    assert(g_log.size() == 2 && g_log[1] == "void-task-done");
    std::puts("[PASS] case 4: Task<void> completes and resumes correctly");
  }

  std::puts("\nAll Task await-semantics checks passed.");
  return 0;
}
