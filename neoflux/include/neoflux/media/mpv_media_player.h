// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - mpv_media_player.h
//
// libmpv-based media player implementation for desktop platforms.
// Uses the mpv render API to decode video frames into an OpenGL texture that
// can be composited by the NeoFlux render layer.
//
// Pimpl: this header leaks no mpv or GL types. All implementation state and
// the mpv/GL interaction live in MpvMediaPlayer::Impl, defined in the .cpp.
//
// All method implementations are in src/media/mpv_media_player.cpp.
// =============================================================================

#ifndef NEOFLUX_MEDIA_MPV_MEDIA_PLAYER_H_
#define NEOFLUX_MEDIA_MPV_MEDIA_PLAYER_H_

#include <cstdint>
#include <memory>

#include "neoflux/media/media_player.h"

namespace neoflux {

// libmpv-backed media player. Decodes video via libmpv and outputs frames to
// an OpenGL texture. Must be initialized on the render thread (InitRender)
// before playback starts.
class MpvMediaPlayer final : public MediaPlayer {
 public:
  MpvMediaPlayer();
  ~MpvMediaPlayer() override;

  // --- MediaPlayer interface ---
  void SetSource(std::string_view source) override;
  [[nodiscard]] std::string_view GetSource() const noexcept override;
  void Play() override;
  void Pause() override;
  void Stop() override;
  void Seek(double position_seconds) override;
  void SetVolume(double volume) override;
  [[nodiscard]] double GetVolume() const noexcept override;
  [[nodiscard]] double GetPosition() const noexcept override;
  [[nodiscard]] double GetDuration() const noexcept override;
  [[nodiscard]] MediaState GetState() const noexcept override;
  [[nodiscard]] int GetVideoWidth() const noexcept override;
  [[nodiscard]] int GetVideoHeight() const noexcept override;
  void SetStateCallback(StateCallback callback) override;
  void SetFrameCallback(FrameCallback callback) override;
  void InitRender() override;
  [[nodiscard]] std::uint32_t UpdateTexture() override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace neoflux

#endif  // NEOFLUX_MEDIA_MPV_MEDIA_PLAYER_H_
