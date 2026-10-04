// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors

#include <cstdio>

// Verifies that -DNEOFLUX_BACKEND_<x> and -DNEOFLUX_BACKEND_NAME="<x>" reach the
// translation unit and select the right branch. Build once per backend:
//   g++ -std=c++20 -DNEOFLUX_BACKEND_gl -DNEOFLUX_BACKEND_NAME=\"gl\" \
//       test_backend_macro.cpp -o t && ./t
#if defined(NEOFLUX_BACKEND_NAME)
static constexpr const char kName[] = NEOFLUX_BACKEND_NAME;
#else
static constexpr const char kName[] = "<undefined>";
#endif
int main() {
#if defined(NEOFLUX_BACKEND_gl)
  printf("ifdef=gl        name=%s\n", kName);
#elif defined(NEOFLUX_BACKEND_vulkan)
  printf("ifdef=vulkan    name=%s\n", kName);
#elif defined(NEOFLUX_BACKEND_d3d12)
  printf("ifdef=d3d12     name=%s\n", kName);
#elif defined(NEOFLUX_BACKEND_metal)
  printf("ifdef=metal     name=%s\n", kName);
#else
  printf("ifdef=<none>    name=%s\n", kName);
#endif
  return 0;
}
