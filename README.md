# NeoFlux++ — 轻量 · 极速 · 声明式 C++20 UI 框架

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/compiler_support)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

**NeoFlux++** 是一个基于 **Taitank**（布局引擎）和 **Skia**（图形渲染引擎）的高性能 UI 框架，提供类似 Flutter 的声明式、组合式开发体验，但底层采用 **多线程布局**、**无锁环形队列**、**mmap 零拷贝** 和 **SIMD 加速**，专为追求极致性能和轻量级的桌面/移动应用而设计。

## ✨ 核心特性

- **声明式 UI**：通过组合 
