#pragma once

#include <cstdlib>

// POSIX setenv/unsetenv are not in the Windows CRT. Tests include this header
// and keep calling setenv/unsetenv.
#if defined(_WIN32)
inline int setenv(const char* name, const char* value, int /*overwrite*/) {
  return _putenv_s(name, value ? value : "");
}

inline int unsetenv(const char* name) { return _putenv_s(name, ""); }
#endif
