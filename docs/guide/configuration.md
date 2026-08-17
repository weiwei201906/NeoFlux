# Configuration

NeoFlux uses [gflags](https://github.com/gflags/gflags) for command-line
configuration and [glog](https://github.com/google/glog) for logging.

## gflags

| Flag | Type | Default | Description |
|------|------|---------|-------------|
| `--target_fps` | int32 | `60` | Target frames per second for the event loop and render pacing |
| `--render_queue_capacity` | uint64 | `2048` | Capacity of the SPSC render command queue (rounded up to power of 2) |
| `--render_backend` | string | `"vulkan"` | Render backend: `"vulkan"`, `"gl"`, or `"cpu"`. Vulkan/CPU fall back to OpenGL with a warning when not yet implemented |
| `--verbose_logging` | bool | `false` | Enable verbose VLOG(1) output and mirror logs to stderr |

### Usage

```bash
./my_app --target_fps=120 --render_queue_capacity=4096
```

## glog

NeoFlux uses glog for logging. Logs are written to files by default.

### Built-in Flags

| Flag | Default | Description |
|------|---------|-------------|
| `--logtostderr` | `false` | Log to stderr instead of files |
| `--log_dir` | `"./logs"` | Directory for log files |
| `--minloglevel` | `0` | Minimum log level (0=INFO, 1=WARNING, 2=ERROR) |
| `--v` | `0` | Verbose log level for VLOG macros |

### Log Files

Log files are written to `./logs/` with the naming convention:

```
<app_name>.<host>.<user>.log.INFO.<timestamp>.<pid>
```

### Logging in Code

```cpp
#include <glog/logging.h>

LOG(INFO) << "Application started";
LOG(WARNING) << "Font not found, using default";
LOG(ERROR) << "Rendering failed";
VLOG(1) << "Detailed debug info";  // only with --v=1 or higher
```

### Log to Console

```bash
./my_app --logtostderr
```

### Disable Logging

```bash
./my_app --minloglevel=3
```

## Example: Full Configuration

```bash
./my_app \
  --target_fps=144 \
  --render_backend=vulkan \
  --render_queue_capacity=4096 \
  --logtostderr \
  --v=1
```

This runs at 144 FPS target, uses Vulkan backend, a 4096-entry queue, logs to
stderr, and enables verbose logging.
