# 安装

## 环境要求

- CMake 3.20+
- 支持 C++20 的编译器（GCC 11+ / Clang 14+ / MSVC 2022）
- Git（用于 FetchContent 下载依赖）

## 获取源码

```bash
git clone https://github.com/weiwei201906/NeoFlux.git
cd NeoFlux
```

## 构建

### Linux / macOS

```bash
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Windows (MinGW)

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j4
```

### Windows (MSVC)

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## 依赖

所有第三方依赖通过 CMake FetchContent 自动下载，无需手动安装：
- glog（日志）
- gflags（命令行参数）
- glfw（桌面端窗口）
- taitank（flex 布局）
- freetype（字体渲染）
- googletest（单元测试，可选）

## 下一步

- [快速开始](./quick-start)
