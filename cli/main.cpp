#include "dcmm/dcmm.h"
#include "dcmm/dcmm.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
  std::string cmd = argc > 1 ? argv[1] : "disk";
  if (cmd == "-h" || cmd == "--help") {
    std::cout << "dcmm-cli [disk|smart|junk|privacy|version|c-abi]\n";
    return 0;
  }
  if (cmd == "version") {
    std::cout << dcmm_version() << "\n";
    return 0;
  }
  if (cmd == "c-abi") {
    dcmm_disk_stats d{};
    dcmm_disk("/", &d);
    char* s = dcmm_format_bytes(d.available_bytes);
    std::cout << d.volume_name << "  free " << (s ? s : "?") << "\n";
    dcmm_string_free(s);
    return 0;
  }
  dcmm::Engine engine;
  if (cmd == "disk") {
    auto d = engine.disk("/");
    auto m = engine.memory();
    std::cout << "Volume: " << d.volumeName << "\n";
    std::cout << "Total: " << dcmm::formatBytes(d.totalBytes)
              << "  Available: " << dcmm::formatBytes(d.availableBytes) << "\n";
    std::cout << "Memory: " << dcmm::formatBytes(m.usedBytes) << " / "
              << dcmm::formatBytes(m.totalBytes) << "\n";
    return 0;
  }
  if (cmd == "smart" || cmd == "junk" || cmd == "privacy") {
    auto r = cmd == "privacy" ? engine.scanPrivacy()
             : cmd == "smart"   ? engine.scanSmart()
                                : engine.scanJunk();
    std::cout << cmd << " — " << dcmm::formatBytes(r.totalBytes()) << " in " << r.groups.size()
              << " groups (" << r.elapsedMs << " ms)\n";
    for (const auto& g : r.groups) {
      std::cout << "  " << g.title << ": " << dcmm::formatBytes(g.totalBytes()) << " ("
                << g.items.size() << " items)\n";
    }
    return 0;
  }
  std::cerr << "Unknown command.\n";
  return 1;
}
