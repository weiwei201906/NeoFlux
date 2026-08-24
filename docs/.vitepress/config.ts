import { defineConfig } from 'vitepress'

export default defineConfig({
  title: 'NeoFlux',
  description: 'A lightweight cross-platform C++20 UI framework with Taitank flex layout and tgfx rendering',
  lastUpdated: true,
  cleanUrls: true,
  base: '/neoflux/',
  locales: {
    root: {
      label: 'English',
      lang: 'en-US',
      link: '/',
    },
    zh: {
      label: '简体中文',
      lang: 'zh-CN',
      link: '/zh/',
      title: 'NeoFlux',
      description: '轻量级跨平台 C++20 UI 框架，基于 Taitank flex 布局与 tgfx 渲染',
    },
  },

  themeConfig: {
    nav: [
      { text: 'Guide', link: '/guide/introduction' },
      { text: 'API', link: '/api/widget' },
      { text: 'Examples', link: '/examples/hello' },
      { text: 'GitHub', link: 'https://github.com/weiwei201906/NeoFlux' },
    ],

    sidebar: {
      '/guide/': [
        {
          text: 'Getting Started',
          items: [
            { text: 'Introduction', link: '/guide/introduction' },
            { text: 'Installation', link: '/guide/installation' },
            { text: 'Quick Start', link: '/guide/quick-start' },
          ],
        },
        {
          text: 'Core Concepts',
          items: [
            { text: 'Architecture', link: '/guide/architecture' },
            { text: 'Widget System', link: '/guide/widgets' },
            { text: 'Flex Layout', link: '/guide/layout' },
            { text: 'Input & Events', link: '/guide/input' },
            { text: 'Routing', link: '/guide/routing' },
            { text: 'Coroutines', link: '/guide/coroutines' },
          ],
        },
        {
          text: 'Advanced',
          items: [
            { text: 'Rendering Pipeline', link: '/guide/rendering' },
            { text: 'Font System', link: '/guide/fonts' },
            { text: 'Performance', link: '/guide/performance' },
            { text: 'Configuration', link: '/guide/configuration' },
            { text: 'Cross-Platform', link: '/guide/cross-platform' },
            { text: 'Testing', link: '/guide/testing' },
            { text: 'FAQ', link: '/guide/faq' },
            { text: 'Contributing', link: '/guide/contributing' },
          ],
        },
      ],
      '/api/': [
        {
          text: 'Core',
          items: [
            { text: 'Application', link: '/api/application' },
            { text: 'EventLoop', link: '/api/event-loop' },
            { text: 'Task (Coroutine)', link: '/api/task' },
            { text: 'RingQueue', link: '/api/ring-queue' },
          ],
        },
        {
          text: 'Widgets',
          items: [
            { text: 'Widget', link: '/api/widget' },
            { text: 'Container', link: '/api/container' },
            { text: 'Text', link: '/api/text' },
            { text: 'Button', link: '/api/button' },
            { text: 'ScrollView', link: '/api/scroll-view' },
            { text: 'Draggable', link: '/api/draggable' },
            { text: 'Expanded', link: '/api/expanded' },
            { text: 'SizedBox', link: '/api/sized-box' },
            { text: 'StatefulWidget', link: '/api/stateful' },
          ],
        },
        {
          text: 'Rendering',
          items: [
            { text: 'RenderContext', link: '/api/render-context' },
            { text: 'RenderCommand', link: '/api/render-command' },
          ],
        },
      ],
      '/examples/': [
        {
          text: 'Examples',
          items: [
            { text: 'Hello NeoFlux', link: '/examples/hello' },
            { text: 'Counter', link: '/examples/counter' },
            { text: 'Flex Layout', link: '/examples/flex' },
            { text: 'Font Demo', link: '/examples/font' },
            { text: 'Scroll View', link: '/examples/scroll' },
            { text: 'Loading Animation', link: '/examples/loading' },
            { text: 'Drag & Drop', link: '/examples/drag' },
          ],
        },
      ],
      '/zh/guide/': [
        {
          text: '入门',
          items: [
            { text: '简介', link: '/zh/guide/introduction' },
            { text: '安装', link: '/zh/guide/installation' },
            { text: '快速开始', link: '/zh/guide/quick-start' },
          ],
        },
        {
          text: '核心概念',
          items: [
            { text: '架构', link: '/zh/guide/architecture' },
            { text: 'Widget 系统', link: '/zh/guide/widgets' },
            { text: 'Flex 布局', link: '/zh/guide/layout' },
            { text: '输入与事件', link: '/zh/guide/input' },
            { text: '路由导航', link: '/zh/guide/routing' },
            { text: '协程', link: '/zh/guide/coroutines' },
          ],
        },
        {
          text: '进阶',
          items: [
            { text: '渲染管线', link: '/zh/guide/rendering' },
            { text: '字体系统', link: '/zh/guide/fonts' },
            { text: '性能优化', link: '/zh/guide/performance' },
            { text: '配置', link: '/zh/guide/configuration' },
            { text: '跨平台', link: '/zh/guide/cross-platform' },
            { text: '测试', link: '/zh/guide/testing' },
            { text: '常见问题', link: '/zh/guide/faq' },
            { text: '贡献指南', link: '/zh/guide/contributing' },
          ],
        },
      ],
      '/zh/api/': [
        {
          text: '核心',
          items: [
            { text: 'Application', link: '/zh/api/application' },
            { text: 'EventLoop', link: '/zh/api/event-loop' },
            { text: 'Task (协程)', link: '/zh/api/task' },
            { text: 'RingQueue', link: '/zh/api/ring-queue' },
          ],
        },
        {
          text: '组件',
          items: [
            { text: 'Widget', link: '/zh/api/widget' },
            { text: 'Container', link: '/zh/api/container' },
            { text: 'Text', link: '/zh/api/text' },
            { text: 'Button', link: '/zh/api/button' },
            { text: 'ScrollView', link: '/zh/api/scroll-view' },
            { text: 'Draggable', link: '/zh/api/draggable' },
            { text: 'Expanded', link: '/zh/api/expanded' },
            { text: 'SizedBox', link: '/zh/api/sized-box' },
            { text: 'StatefulWidget', link: '/zh/api/stateful' },
          ],
        },
        {
          text: '渲染',
          items: [
            { text: 'RenderContext', link: '/zh/api/render-context' },
            { text: 'RenderCommand', link: '/zh/api/render-command' },
          ],
        },
      ],
      '/zh/examples/': [
        {
          text: '示例',
          items: [
            { text: 'Hello NeoFlux', link: '/zh/examples/hello' },
            { text: '计数器', link: '/zh/examples/counter' },
            { text: 'Flex 布局', link: '/zh/examples/flex' },
            { text: '字体演示', link: '/zh/examples/font' },
            { text: '滚动视图', link: '/zh/examples/scroll' },
            { text: '加载动画', link: '/zh/examples/loading' },
            { text: '拖拽演示', link: '/zh/examples/drag' },
          ],
        },
      ],
    },

    socialLinks: [
      { icon: 'github', link: 'https://github.com/weiwei201906/NeoFlux' },
    ],

    footer: {
      message: 'Released under the GPL-3.0 License.',
      copyright: 'Copyright (c) 2026 NeoFlux Contributors',
    },

    search: {
      provider: 'local',
    },
  },
})
