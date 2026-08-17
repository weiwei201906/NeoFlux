# Configuration

NeoFlux uses [gflags](https://github.com/gflags/gflags) for command-line
configuration and [glog](https://github.com/google/glog) for logging.

## gflags

| Flag | Default | Description |
|------|---------|-------------|
| `--target_fps` | `60` | Target frames per second for the render loop |
| `--render_queue_capacity` | `2048` | Capacity of the SPSC render command queue |
| `--render_backend` | `"gl"` | Render backend: `"gl"` (OpenGL) or `"vulkan"` (fallback to GL) |
| `--verbose_logging` | `false` | Enable verbose VLOG output |

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
  --render_backend=gl \
  --render_queue_capacity=4096 \
  --logtostderr \
  --v=1
```

This runs at 144 FPS target, uses OpenGL, a 4096-entry queue, logs to stderr,
and enables verbose logging.
