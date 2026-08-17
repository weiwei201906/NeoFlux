# 简介

NeoFlux 是一个轻量级跨平台 C++20 UI 框架，采用类 Flutter 的 Widget 开发模型。

## 核心特性

- **两层架构**：Application 层运行业务逻辑与 Taitank flex 布局，Render 层通过 SPSC 无锁环形队列消费渲染命令
- **Widget 系统**：虚函数覆盖实现自定义组件，路由注册导航，状态机驱动 UI 更新
- **C++20 协程**：内置 Task\<void\> 协程，Yield() 帧同步与 Sleep() 定时器
- **跨平台**：Windows / Linux / macOS 桌面端，Android / iOS 移动端
- **现代 C++**：C++20 标准，Google 编码规范，clang-tidy 零警告
- **工程化**：glog 日志、gflags 配置、gtest 单元测试、-Werror 零警告构建

## 协议

本项目采用 **GNU General Public License v3.0 (GPL-3.0)** 开源协议。

## 下一步

- [快速开始](./quick-start)
- [架构概览](./architecture)
- [Widget 系统](./widgets)
