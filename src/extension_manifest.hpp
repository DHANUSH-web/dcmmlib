#pragma once

#include "dcmm/path.hpp"

#include <cctype>
#include <fstream>
#include <string>

namespace dcmm {
namespace extjson {

inline std::string readSmallFile(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::string s(64 * 1024, '\0');
  in.read(s.data(), static_cast<std::streamsize>(s.size()));
  s.resize(static_cast<std::size_t>(in.gcount()));
  return s;
}

inline void skipWs(const std::string& s, std::size_t& pos) {
  while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) ++pos;
}

inline std::string jsonQuoted(const std::string& s, std::size_t pos) {
  if (pos >= s.size() || s[pos] != '"') return {};
  ++pos;
  std::string out;
  for (; pos < s.size(); ++pos) {
    char c = s[pos];
    if (c == '"') break;
    if (c == '\\' && pos + 1 < s.size()) {
      out.push_back(s[++pos]);
      continue;
    }
    out.push_back(c);
  }
  return out;
}

inline std::string jsonStringField(const std::string& body, const char* key) {
  const std::string pat = std::string("\"") + key + "\"";
  auto pos = body.find(pat);
  if (pos == std::string::npos) return {};
  pos = body.find(':', pos + pat.size());
  if (pos == std::string::npos) return {};
  ++pos;
  skipWs(body, pos);
  return jsonQuoted(body, pos);
}

inline std::string jsonRepositoryUrl(const std::string& body) {
  const std::string pat = "\"repository\"";
  auto pos = body.find(pat);
  if (pos == std::string::npos) return {};
  pos = body.find(':', pos + pat.size());
  if (pos == std::string::npos) return {};
  ++pos;
  skipWs(body, pos);
  if (pos >= body.size()) return {};
  if (body[pos] == '"') return jsonQuoted(body, pos);
  if (body[pos] == '{') {
    auto end = body.find('}', pos);
    if (end == std::string::npos) return {};
    return jsonStringField(body.substr(pos, end - pos + 1), "url");
  }
  return {};
}

struct Manifest {
  std::string name;
  std::string version;
  std::string publisher;
  std::string repositoryUrl;
  std::string iconPath;
};

inline Manifest read(const std::string& dir, const std::string& folderName) {
  Manifest m;
  const auto json = readSmallFile(joinPath(dir, "package.json"));
  m.name = jsonStringField(json, "displayName");
  if (m.name.empty() || (m.name.size() >= 2 && m.name.front() == '%' && m.name.back() == '%')) {
    m.name = jsonStringField(json, "name");
    if (m.name.empty()) m.name = folderName;
  }
  m.version = jsonStringField(json, "version");
  m.publisher = jsonStringField(json, "publisher");
  m.repositoryUrl = jsonRepositoryUrl(json);
  const auto icon = jsonStringField(json, "icon");
  if (!icon.empty()) {
    auto p = joinPath(dir, icon);
    if (isRegularFile(p)) m.iconPath = std::move(p);
  }
  return m;
}

}  // namespace extjson
}  // namespace dcmm
