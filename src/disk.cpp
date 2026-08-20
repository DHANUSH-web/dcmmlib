#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"

#include <cstring>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#else
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

namespace dcmm {

DiskStats Engine::disk(const std::string& path) const {
  DiskStats s;
  const std::string p = path.empty() ? std::string("/") : path;
  s.mountPoint = p;

#if defined(_WIN32)
  ULARGE_INTEGER avail{}, total{}, freeb{};
  std::string win = p;
  if (GetDiskFreeSpaceExA(win.c_str(), &avail, &total, &freeb)) {
    s.totalBytes = total.QuadPart;
    s.availableBytes = avail.QuadPart;
    s.freeBytes = freeb.QuadPart;
  }
  char vol[MAX_PATH]{};
  GetVolumeInformationA(win.c_str(), vol, MAX_PATH, nullptr, nullptr, nullptr, nullptr, 0);
  s.volumeName = vol;
#elif defined(__APPLE__)
  struct statfs fs {};
  if (statfs(p.c_str(), &fs) == 0) {
    s.totalBytes = static_cast<uint64_t>(fs.f_blocks) * fs.f_bsize;
    s.freeBytes = static_cast<uint64_t>(fs.f_bfree) * fs.f_bsize;
    s.availableBytes = static_cast<uint64_t>(fs.f_bavail) * fs.f_bsize;
    s.volumeName = fs.f_mntonname;
  }
#else
  struct statvfs fs {};
  if (statvfs(p.c_str(), &fs) == 0) {
    s.totalBytes = static_cast<uint64_t>(fs.f_blocks) * fs.f_frsize;
    s.freeBytes = static_cast<uint64_t>(fs.f_bfree) * fs.f_frsize;
    s.availableBytes = static_cast<uint64_t>(fs.f_bavail) * fs.f_frsize;
  }
  s.volumeName = p;
#endif
  return s;
}

MemoryStats Engine::memory() const {
  MemoryStats m;
#if defined(_WIN32)
  MEMORYSTATUSEX st{};
  st.dwLength = sizeof(st);
  if (GlobalMemoryStatusEx(&st)) {
    m.totalBytes = st.ullTotalPhys;
    m.freeBytes = st.ullAvailPhys;
    m.usedBytes = m.totalBytes - m.freeBytes;
  }
#elif defined(__APPLE__)
  int64_t memsize = 0;
  size_t len = sizeof(memsize);
  int name[2] = {CTL_HW, HW_MEMSIZE};
  sysctl(name, 2, &memsize, &len, nullptr, 0);
  m.totalBytes = static_cast<uint64_t>(memsize);
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
  vm_statistics64_data_t vm{};
  if (host_statistics64(mach_host_self(), HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vm),
                        &count) == KERN_SUCCESS) {
    vm_size_t page = 0;
    host_page_size(mach_host_self(), &page);
    uint64_t pg = static_cast<uint64_t>(page);
    m.freeBytes = vm.free_count * pg;
    m.wiredBytes = vm.wire_count * pg;
    m.compressedBytes = vm.compressor_page_count * pg;
    m.usedBytes = m.totalBytes > m.freeBytes ? m.totalBytes - m.freeBytes : 0;
  }
#else
  struct sysinfo info {};
  if (sysinfo(&info) == 0) {
    m.totalBytes = static_cast<uint64_t>(info.totalram) * info.mem_unit;
    m.freeBytes = static_cast<uint64_t>(info.freeram) * info.mem_unit;
    m.usedBytes = m.totalBytes - m.freeBytes;
  }
#endif
  return m;
}

}  // namespace dcmm
