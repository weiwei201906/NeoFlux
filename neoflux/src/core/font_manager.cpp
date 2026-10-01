// =============================================================================
// NeoFlux - font_manager.cpp
//
// Implementation of FontManager. Methods moved from header.
// =============================================================================

#include "neoflux/core/font_manager.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>

#include <glog/logging.h>

namespace neoflux {

namespace {

// Returns true if the file extension is a supported font format.
bool IsFontFile(const std::filesystem::path& path) {
  const std::string ext = path.extension().string();
  return ext == ".ttf" || ext == ".otf" || ext == ".ttc";
}

// Converts a string to lowercase for case-insensitive font name lookup.
std::string ToLower(std::string_view s) {
  std::string result(s);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return result;
}

}  // namespace

void FontManager::ScanDirectory(std::string_view directory) {
  const std::filesystem::path dir(directory);
  if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
    LOG(WARNING) << "Font directory not found: " << directory;
    return;
  }

  std::size_t count = 0;
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() || !IsFontFile(entry.path())) {
      continue;
    }
    const std::string name = ToLower(entry.path().stem().string());
    fonts_[name] = std::filesystem::absolute(entry.path()).string();
    if (default_font_.empty()) {
      default_font_ = name;
    }
    ++count;
  }
  LOG(INFO) << "FontManager scanned " << directory << ": found " << count
            << " fonts (default: " << default_font_ << ")";
}

std::string FontManager::GetPath(std::string_view font_name) const {
  const auto it = fonts_.find(ToLower(font_name));
  if (it == fonts_.end()) {
    return {};
  }
  return it->second;
}

bool FontManager::HasFont(std::string_view font_name) const {
  return fonts_.find(ToLower(font_name)) != fonts_.end();
}

std::string FontManager::GetDefaultFont() const { return default_font_; }

std::size_t FontManager::GetFontCount() const noexcept { return fonts_.size(); }

}  // namespace neoflux
