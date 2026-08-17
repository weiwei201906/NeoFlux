import { defineConfig } from 'vitepress'

export default defineConfig({
  title: 'NeoFlux',
  description: 'A lightweight cross-platform C++20 UI framework with Taitank flex layout and tgfx rendering',
  lang: 'en-US',
  lastUpdated: true,
  cleanUrls: true,

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
            { text: 'Configuration', link: '/guide/configuration' },
            { text: 'Cross-Platform', link: '/guide/cross-platform' },
            { text: 'Testing', link: '/guide/testing' },
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
