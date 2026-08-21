# Contributing to dcmmlib

Thanks for helping. This library is the GUI-free engine behind [DeepCleanMyMac](https://github.com/DHANUSH-web/DeepCleanMyMac). It must stay **portable C++** with a strict trash allowlist.

Please read [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) and [SECURITY.md](SECURITY.md).

## Branches

| Branch | Role |
|--------|------|
| `dev` | Default for PRs and new work |
| `main` | Stable releases — do not open feature PRs against `main` |

1. Fork the repo and branch from **`dev`**.
2. Keep the PR focused (one feature or one fix).
3. Add or update GoogleTest coverage for safety, catalogs, hashing, or trash behavior.
4. Open the PR **against `dev`**.

Commit subjects: `FEAT: …` or `FIX: …` (uppercase kind, imperative subject).

## Build and test

CMake 3.21+, Ninja, Clang.

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure
```

Tests may set `DCMM_HOME` / `DCMM_TRASH` so they never touch a real home directory. Do not add tests that empty the developer’s real Trash or delete files outside the fixture.

## Rules

- **No GUI** (no AppKit, Qt, WinUI, GTK) in this repo.
- `isSafeToTrash` is the last gate. `trashPaths` must call it on every path.
- Prefer missing a junk folder over deleting user data.
- Keep Windows Recycle Bin and Linux trash paths working even if you develop on a Mac.
- Public API lives in `include/dcmm/`. New methods go on `dcmm::Engine`.

After your change lands, the desktop app submodule (`dcmm-desktop/extras/dcmmlib`) must be pointed at the new `dev` commit if the Mac UI should pick it up.
