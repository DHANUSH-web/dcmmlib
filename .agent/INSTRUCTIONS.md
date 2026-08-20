# dcmmlib — agent instructions

Read this entire file before changing code. Do not ask the user to re-explain product, architecture, or safety rules.

## What this repo is

**dcmmlib** is a **GUI-free C++17 engine** for DeepCleanMyMac. It scans regenerable junk, finds large/duplicate files, lists app leftovers, reports disk usage, and moves **user-selected** paths to the platform Trash.

- **No GUI.** No AppKit, Qt, WinUI, or GTK here.
- **Cross-platform C++.** A Windows (or Linux) desktop app should link this library. Do not turn this repo into a GUI.
- **macOS GUI is a different project:** sibling/submodule consumer `dcmm-desktop` (native AppKit, mac-only).

Workspace on the original machine is often:

```
DeepCleanMyMac/          # parent folder, not necessarily a git repo
  dcmmlib/               # this repo (own git)
  dcmm-desktop/          # mac app (own git, submodule of this repo at extras/dcmmlib)
```

## Product intent

Free CleanMyMac-style cleaner: reclaim disk space **carefully**. Prefer moving to Trash over permanent delete. Never surprise-delete. Protected system and personal data must be rejected even if a caller asks.

## Public APIs

C++ (preferred):

```cpp
#include <dcmm/dcmm.hpp>
dcmm::Engine e;
auto report = e.scanSmart(progress);
auto result = e.trashPaths(paths);
```

Umbrella: `include/dcmm/dcmm.hpp`  
Facade: `include/dcmm/engine.hpp` → `dcmm::Engine`  
Types: `include/dcmm/types.hpp`  
Safety: `include/dcmm/safety.hpp`  
Paths/format: `include/dcmm/path.hpp`

C ABI for FFI (`include/dcmm/dcmm.h`): `dcmm_create`, `dcmm_scan_smart`, `dcmm_scan_junk`, `dcmm_scan_privacy`, `dcmm_trash_paths`, `dcmm_disk`, `dcmm_format_bytes`. Static builds define `DCMM_STATIC`.

Tests may set `DCMM_HOME` and `DCMM_TRASH` so scans never touch a real home directory.

## Layout

| Path | Role |
|------|------|
| `include/dcmm/` | Public headers only |
| `src/` | Implementation |
| `src/catalog.cpp` | Junk/privacy locations (per-OS `#if`) |
| `src/safety.cpp` | Allow/deny for trash |
| `src/clean.cpp` | Move to Trash (no copy+unlink fallback) |
| `src/maintenance.cpp` | Empty trash, DNS, Launch Services, Quick Look |
| `src/sha256.cpp` | Portable SHA-256 (no extra crypto deps) |
| `tests/` | GoogleTest |
| `cli/` | `dcmm-cli` smoke tool |
| `cmake/llvm-clang.cmake` | Clang toolchain file |

`dcmm::Engine` methods: `scanSmart`, `scanJunk`, `scanPrivacy`, `trashPaths`, `findLargeFiles`, `findDuplicates`, `spaceLens`, `listApps`, `attachLeftovers`, `disk`, `memory`, `maintenanceTasks`, `previewMaintenance`, `runMaintenance`.

## Build (required toolchain)

CMake + **Ninja** + **Clang/LLVM**. C++17.

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
./build/release/dcmm-cli disk
```

Debug: `--preset debug`. Presets live in `CMakePresets.json` (Ninja + Clang). Local overrides belong in `CMakeUserPresets.json` (gitignored).

Equivalent without presets:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/dcmm-cli disk
```

Optional: `-DCMAKE_TOOLCHAIN_FILE=cmake/llvm-clang.cmake`  
Shared lib: `-DDCMM_BUILD_SHARED=ON`  
Default artifact: `libdcmm.a` (static). Apple links CoreFoundation; Windows links `shell32`.

## Safety — non-negotiable

`isSafeToTrash` is the last gate. `trashPaths` must call it on every path.

**Never trash**

- `/`, `/System`, `/bin`, `/usr/bin`, home directory itself
- `Documents` / `Desktop` / `Downloads` / `Pictures` / `Movies` / `Music` / `Library` **as whole folders**
- Keychains, Mail, Messages, Safari data, Photos libraries (`.photoslibrary`), iCloud / CloudStorage
- `.ssh`, `.gnupg`, `.aws`, `.kube`, `.config`, password stores
- Sensitive names/extensions: `id_rsa`, `.pem`, `.key`, `.p12`, `.kdbx`, etc.
- Entire `~/Library/Application Support` or generic children like `Google`
- `com.apple.*` leftovers under Application Support / Preferences / Containers

**May trash only if selected by the caller**

- Regenerable junk: user Caches, Logs, Saved Application State, HTTPStorages, Xcode DerivedData / DeviceSupport (review), Homebrew/pip/npm/gradle/cargo caches, per-user tmp, Trash contents as a scan category
- User-selected **regular files** under home (large files / duplicates), still subject to protected trees
- Third-party `.app` under `/Applications` or `~/Applications` — never `/System/Applications`
- Skip files **not owned by the current user** (admin/system-owned). Never `/Library/Caches`.

**Smart vs detailed catalogs**

- `smartCatalog()` / `scanSmart()`: recommended groups only (on Apple: `~/Library/Caches`, Logs, Saved Application State as **one item each**, not per-child). Items start selected. No npm/cargo/Xcode/darwin tmp.
- `junkCatalog()` / `scanJunk()`: every child folder for System Junk. Items start unselected. May include package-manager caches and review-first Xcode trees.
- Trashing a **category root** (e.g. `~/Library/Caches`) expands to owned, safe children; the folder itself stays.

**Trash behavior**

- Move to `~/.Trash` (macOS), Freedesktop Trash (Linux), Recycle Bin `FOF_ALLOWUNDO` (Windows).
- **Do not** copy-then-`remove_all` if rename fails. Fail the item instead.
- Empty Trash (`runMaintenance("empty_trash")`) is the only permanent delete. It must stay inside the user trash dir and skip symlinks that resolve outside it.
- `previewMaintenance` **before** empty trash / Quick Look: if nothing is there, return `nothingToDo` and message **"Nothing to clean..."**. Do not pretend work happened.
- After real work, `MaintenanceResult` / `CleanResult` must report **bytes freed** and item counts.

**Defaults**

- `scanSmart` / `scanJunk` / `scanPrivacy`: `ScanItem.selected` is **false** (opt-in).
- `scanSmart`: items start **selected** (recommended groups).
- Uninstaller leftovers default unselected.

## Maintenance IDs

| id | What it does |
|----|----------------|
| `empty_trash` | Permanently delete user Trash contents (measure first) |
| `flush_dns` | DNS cache only — no files |
| `launch_services` | User-domain `lsregister` — no files (Apple) |
| `quicklook` | Move Quick Look caches to Trash (Apple) |

## Platform

Use `#if defined(_WIN32)` / `__APPLE__` / else Linux. Keep Windows Recycle Bin and Program Files catalogs working even if you develop on a Mac. Do not add Objective-C to this repo.

## Tests

GoogleTest via FetchContent (v1.15.2). Add tests when changing safety, hashing, format, or catalogs. Existing coverage includes safety (root, Documents, SSH, Application Support, cache children, sensitive names), SHA-256 vectors, C ABI, duplicates, disk.

## Commits

Every commit subject must be `KIND: message` with **KIND in uppercase**:

- **FEAT:** new API, catalog, tests, or capability
- **FIX:** a bug, safety hole, wrong size/path, or regression

Examples: `FEAT: Split Smart Scan groups from System Junk.` / `FIX: Do not double-count Homebrew cache.`

Do not use Conventional Commits (`feat:` lowercase, scopes, types like `chore`). Split mixed work into a FEAT commit and a FIX commit. Subject after the colon is imperative, like the rest of this repo.

## How to change this library

- Put new public types in `types.hpp` and new engine methods on `Engine`.
- Keep catalogs conservative; prefer a missed junk folder over deleting user data.
- If `dcmm-desktop` uses a **submodule copy** at `extras/dcmmlib`, copy or commit+update that submodule after engine changes or the app will build stale sources.

## Out of scope

- GUI, AppKit, windows, buttons, theming
- Telemetry, accounts, network APIs for “cloud clean”
- Permanent delete of user documents
- Building DeepCleanMyMac.app (that is `dcmm-desktop`)
