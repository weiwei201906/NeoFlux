# Introduction

NeoFlux is a lightweight, cross-platform C++20 UI framework designed for
embedded and mobile applications. It follows a two-layer architecture inspired
by Flutter's widget model, separating business logic/layout from rendering.

## Design Goals

- **Lightweight**: Minimal dependencies, small binary footprint, suitable for
  resource-constrained devices.
- **Cross-platform**: Single codebase for desktop (Windows/Linux/macOS) and
  mobile (Android/iOS).
- **Declarative widgets**: Compose UIs from reusable widget classes. Override
  virtual functions to customize behavior.
- **Flex layout**: Powered by [Taitank](https://github.com/Tencent/taitank), a
  high-performance flexbox layout engine from Tencent.
- **Thread-safe**: Application and render layers communicate via a lock-free
  SPSC ring queue.

## Two-Layer Architecture

```
┌─────────────────────────────────────────┐
│  Application Layer                      │
│  - Business logic                       │
│  - Widget tree                          │
│  - Taitank flex layout                  │
│  - Event loop + coroutines              │
└───────────────┬─────────────────────────┘
                │  SPSC RingQueue (RenderCommand)
                ▼
┌─────────────────────────────────────────┐
│  Render Layer                           │
│  - Consumes render commands             │
│  - Mobile: tgfx direct rendering        │
│  - Desktop: GLFW bridge + OpenGL        │
└─────────────────────────────────────────┘
```

The application layer builds the widget tree, runs Taitank layout, and submits
render commands. The render layer consumes those commands on a dedicated thread
and draws to the screen. This separation keeps the UI responsive even during
heavy rendering.

## License

NeoFlux is released under the **GPL-3.0** license. See the `LICENSE` file for
details.
