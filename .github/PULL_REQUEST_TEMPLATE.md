## Summary

<!-- FEAT or FIX. Target branch: **dev**. -->

## Kind

- [ ] FEAT
- [ ] FIX

## Tests

- [ ] `ctest --preset release` passes
- [ ] New or updated tests for safety / catalog / trash if those changed
- [ ] Tests use `DCMM_HOME` (no real home directory)

## Safety

- [ ] `trashPaths` still calls `isSafeToTrash` on every path
- [ ] No copy-then-unlink fallback
- [ ] Windows / Linux `#if` paths still compile in my head (or on CI)

## Notes

<!-- Breaking C API / C++ API? Say so. -->
