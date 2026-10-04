// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors

#include <cstdio>

// Verifies that the tgfx backend mirror macro (TGFX_USE_OPENGL / _VULKAN /
// _D3D12 / _METAL, exported by thirdparty/CMakeLists.txt from tgfx's own
// options) reaches the translation unit and selects the right branch.
// Build once per backend:
//   g++ -std=c++20 -DTGFX_USE_OPENGL=1 test_backend_macro.cpp -o t && ./t
int main() {
#if defined(TGFX_USE_OPENGL)
  printf("backend=opengl\n");
#elif defined(TGFX_USE_VULKAN)
  printf("backend=vulkan\n");
#elif defined(TGFX_USE_D3D12)
  printf("backend=d3d12\n");
#elif defined(TGFX_USE_METAL)
  printf("backend=metal\n");
#else
  printf("backend=<none>\n");
  return 1;
#endif
  return 0;
}
