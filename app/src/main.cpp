// GNU General Public License v3.0
// Author: weiwei201906
// Date: 2026-07-01

#include <glog/logging.h>
#include <iostream>

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  LOG(INFO) << "neoflux App started";
  std::cout << "neoflux App started\n";
  return 0;
}
