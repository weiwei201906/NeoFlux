// =============================================================================
// NeoFlux - font_manager.h
//
// Font manager: scans a directory for TrueType / OpenType font files and
// maps font names (filename stem) to absolute file paths. Widgets reference
// fonts by name; the renderer resolves the name to a file path via this
// manager and loads the glyphs with FreeType.
//
// Method implementations are in src/core/font_manager.cpp.
// =============================================================================

#ifndef NEOFLUX_CORE_FONT_MANAGER_H_
#define NEOFLUX_CORE_FONT_MANAGER_H_

#include <string>
#include <string_view>
#include <unordered_map>

#include "neoflux/core/noncopyable.h"

namespace neoflux {

// Scans font directories and resolves font names to file paths.
//
// Usage:
//   FontManager fonts;
//   fonts.ScanDirectory("thirdparty/fonts");
//   std::string path = fonts.GetPath("NotoSansSC-Regular");
class FontManager : public NonCopyable {
 public:
  FontManager() = default;
  ~FontManager() = default;

  // Scans `directory` for .ttf, .otf, and .ttc files. Each file is registered
  // under its filename stem (e.g. "NotoSansSC-Regular" for
  // "NotoSansSC-Regular.ttf"). Later registrations overwrite earlier ones with
  // the same name.
  void ScanDirectory(std::string_view directory);

  // Returns the absolute path for the given font name, or an empty string if
  // the font was not found.
  [[nodiscard]] std::string GetPath(std::string_view font_name) const;

  // Returns true if a font with the given name is registered.
  [[nodiscard]] bool HasFont(std::string_view font_name) const;

  // Returns the default font name (first registered font, or empty if none).
  [[nodiscard]] std::string GetDefaultFont() const;

  // Returns the number of registered fonts.
  [[nodiscard]] std::size_t GetFontCount() const noexcept;

 private:
  // Maps font name (lowercase) to absolute file path.
  std::unordered_map<std::string, std::string> fonts_{};
  std::string default_font_{};
};

}  // namespace neoflux

#endif  // NEOFLUX_CORE_FONT_MANAGER_H_
