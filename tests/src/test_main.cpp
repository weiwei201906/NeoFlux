// GNU General Public License v3.0
// Author: weiwei201906
// Date: 2026-07-01

#include <glog/logging.h>
#include <gtest/gtest.h>

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
