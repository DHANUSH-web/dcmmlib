# dcmmlib

GUI-free C++ engine for **DeepCleanMyMac** and any other front end. Scan junk, find large and duplicate files, list leftover app files, and move selected paths to the platform trash. Native binaries, LLVM/Clang, CMake + Ninja.

The library has **no GUI code**. Desktop, CLI, or another toolkit (Qt, wx, WinUI, GTK) talks to it through the C++ API or the C ABI.

## APIs

- **C++** (`#include <dcmm/dcmm.hpp>`) — `dcmm::Engine` is the facade.
- **C ABI** (`#include <dcmm/dcmm.h>`) — `dcmm_create` / `dcmm_scan_junk` / `dcmm_trash_paths` for FFI and other languages.

Override `DCMM_HOME` (and optionally `DCMM_TRASH`) in tests so scans never touch a real home directory.

Protected locations (system trees, `~/Documents` itself, keychains, `.ssh`, Photos libraries, …) are never sent to Trash.

## Build (LLVM + Clang + Ninja)

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-clang.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Produces `libdcmm.a` (default) and `dcmm-cli`.

```bash
./build/dcmm-cli disk
./build/dcmm-cli junk
```

Shared library: `-DDCMM_BUILD_SHARED=ON`.

## Layout

| Path | Role |
|------|------|
| `include/dcmm/` | Public headers |
| `src/` | Engine (scan, hash, trash, catalogs) |
| `tests/` | GoogleTest |
| `cli/` | Optional smoke tool |

Platform backends live behind `#if` in the engine (`macOS`, `Linux`, `Windows`). Trash uses `~/.Trash` / Freedesktop Trash / Recycle Bin (`FOF_ALLOWUNDO`).
