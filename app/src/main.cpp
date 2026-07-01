// GNU General Public License v3.0
// Author: weiwei201906
// Date: 2026-07-01

#include <glog/logging.h>

#include <cstdlib>
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);

  constexpr std::string_view kHelloWorld = "Hello, NeoFlux!";
  LOG(INFO) << kHelloWorld;
  std::cout << kHelloWorld << '\n';
  return EXIT_SUCCESS;
}
