// MinGW stub for tgfx's SystemFont.cpp.
//
// tgfx's real SystemFont.cpp resolves Windows system fonts via DirectWrite and
// calls IDWriteFontSet::GetMatchingFonts(family, weight, stretch, style, ...)
// (the DWrite 3 / Windows 10 overload). MinGW's dwrite_3.h only ships the
// older property-based overload, so the real translation unit cannot compile
// with MinGW GCC. For CI build purposes system-font lookup is not required;
// provide the two exported entry points as no-ops (return nullptr) so the
// tgfx target links. This file is used ONLY when CMake sees MINGW.
#include <string>
#include "core/vectors/freetype/SystemFont.h"

namespace tgfx {

std::shared_ptr<Typeface> SystemFont::MakeFromName(const std::string& fontFamily,
                                                   const std::string& fontStyle) {
  (void)fontFamily;
  (void)fontStyle;
  return nullptr;
}

std::shared_ptr<Typeface> SystemFont::MakeFromName(const std::string& fontFamily,
                                                   FontStyle fontStyle) {
  (void)fontFamily;
  (void)fontStyle;
  return nullptr;
}

}  // namespace tgfx
