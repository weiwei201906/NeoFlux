// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 NeoFlux Authors
// =============================================================================
// NeoFlux - mpv_media_player_test.cpp
//
// Integration tests for the libmpv media player backend (desktop). Playback is
// verified against the real sample video staged at tests/data/sample.mp4
// (path injected at configure time as NEOFLUX_TEST_MEDIA_PATH).
//
// The backend renders through the mpv SOFTWARE render API into a CPU buffer, so
// these tests need no GPU context, no window and no display: they drive the
// render path exactly like the framework does (InitRender once, then UpdateFrame
// per iteration) and can therefore run headless in CI.
//
// When libmpv is not linked (no NEOFLUX_HAS_MPV), every test is skipped.
// =============================================================================

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include "neoflux/media/media_player.h"
#include "neoflux/media/mpv_media_player.h"

namespace neoflux {
namespace {

using namespace std::chrono_literals;

#ifdef NEOFLUX_HAS_MPV

// Number of milliseconds to wait for a playback state transition.
constexpr auto kStateTimeout = 15s;
// Polling interval while waiting for a state transition.
constexpr auto kPollInterval = 20ms;

// Waits until the player reaches the target state or times out. Drives the mpv
// event pump via UpdateFrame() (which also publishes decoded frames).
bool WaitForState(MpvMediaPlayer& player, MediaState target) {
  const auto deadline = std::chrono::steady_clock::now() + kStateTimeout;
  while (std::chrono::steady_clock::now() < deadline) {
    (void)player.UpdateFrame();
    if (player.GetState() == target) {
      return true;
    }
    std::this_thread::sleep_for(kPollInterval);
  }
  const bool reached = player.GetState() == target;
  if (!reached) {
    GTEST_LOG_(INFO) << "WaitForState timeout: expected "
                     << static_cast<int>(target) << ", current state="
                     << static_cast<int>(player.GetState())
                     << " source=" << player.GetSource()
                     << " duration=" << player.GetDuration()
                     << " w=" << player.GetVideoWidth()
                     << " h=" << player.GetVideoHeight();
  }
  return reached;
}

// Waits until the first decoded video frame has been published, i.e. the player
// reports a non-zero video size (written when a frame is rendered).
bool WaitForFirstFrame(MpvMediaPlayer& player) {
  const auto deadline = std::chrono::steady_clock::now() + kStateTimeout;
  while (std::chrono::steady_clock::now() < deadline) {
    (void)player.UpdateFrame();
    if (player.GetVideoWidth() > 0 && player.GetVideoHeight() > 0) {
      return true;
    }
    std::this_thread::sleep_for(kPollInterval);
  }
  return player.GetVideoWidth() > 0 && player.GetVideoHeight() > 0;
}

#endif  // NEOFLUX_HAS_MPV

class MpvMediaPlayerTest : public ::testing::Test {
 protected:
  void SetUp() override {
#ifdef NEOFLUX_HAS_MPV
    player_ = std::make_unique<MpvMediaPlayer>();
    if (player().GetState() == MediaState::kError) {
      player_.reset();
      GTEST_SKIP() << "libmpv initialization failed";
    }
#else
    GTEST_SKIP() << "NeoFlux built without libmpv (NEOFLUX_HAS_MPV undefined)";
#endif
  }

  // Returns the player under test.
  [[nodiscard]] MpvMediaPlayer& player() const noexcept { return *player_; }

 private:
  std::unique_ptr<MpvMediaPlayer> player_;
};

#ifdef NEOFLUX_HAS_MPV

TEST_F(MpvMediaPlayerTest, InitialStateIsIdle) {
  EXPECT_EQ(player().GetState(), MediaState::kIdle);
}

TEST_F(MpvMediaPlayerTest, SourceRoundTrip) {
  constexpr std::string_view kSource = NEOFLUX_TEST_MEDIA_PATH;
  player().SetSource(kSource);
  EXPECT_EQ(player().GetSource(), kSource);
}

TEST_F(MpvMediaPlayerTest, VolumeRoundTrip) {
  player().SetVolume(0.5);
  EXPECT_DOUBLE_EQ(player().GetVolume(), 0.5);

  // Volume is clamped to [0, 1].
  player().SetVolume(2.0);
  EXPECT_DOUBLE_EQ(player().GetVolume(), 1.0);
  player().SetVolume(-1.0);
  EXPECT_DOUBLE_EQ(player().GetVolume(), 0.0);
}

TEST_F(MpvMediaPlayerTest, PlayEntersLoadingImmediately) {
  player().SetSource(NEOFLUX_TEST_MEDIA_PATH);
  player().Play();
  EXPECT_EQ(player().GetState(), MediaState::kLoading);
}

// Full playback smoke test: drive the software render path until libmpv reports
// the file loaded and a first CPU frame has been published as an image id.
TEST_F(MpvMediaPlayerTest, PlayReachesPlayingAndPublishesFrame) {
  player().SetSource(NEOFLUX_TEST_MEDIA_PATH);
  player().InitRender();
  player().Play();

  ASSERT_TRUE(WaitForState(player(), MediaState::kPlaying))
      << "Player did not reach kPlaying within timeout";
  ASSERT_TRUE(WaitForFirstFrame(player()))
      << "Player did not publish a first frame within timeout";
  EXPECT_GT(player().GetVideoWidth(), 0);
  EXPECT_GT(player().GetVideoHeight(), 0);
  EXPECT_GT(player().GetDuration(), 0.0);
  // A published frame is a non-zero, registry-backed image id.
  EXPECT_NE(player().UpdateFrame(), 0U);
}

TEST_F(MpvMediaPlayerTest, PauseTransitionsToPaused) {
  player().SetSource(NEOFLUX_TEST_MEDIA_PATH);
  player().InitRender();
  player().Play();
  ASSERT_TRUE(WaitForState(player(), MediaState::kPlaying));

  player().Pause();
  EXPECT_EQ(player().GetState(), MediaState::kPaused);
}

TEST_F(MpvMediaPlayerTest, StopTransitionsToIdle) {
  player().SetSource(NEOFLUX_TEST_MEDIA_PATH);
  player().InitRender();
  player().Play();
  ASSERT_TRUE(WaitForState(player(), MediaState::kPlaying));

  player().Stop();
  EXPECT_EQ(player().GetState(), MediaState::kIdle);
}

TEST_F(MpvMediaPlayerTest, SeekDoesNotCrash) {
  player().SetSource(NEOFLUX_TEST_MEDIA_PATH);
  player().InitRender();
  player().Play();
  ASSERT_TRUE(WaitForState(player(), MediaState::kPlaying));

  player().Seek(1.0);
  player().Seek(0.0);
  SUCCEED();
}

TEST_F(MpvMediaPlayerTest, StateCallbackFiresOnPlay) {
  MediaState observed = MediaState::kIdle;
  player().SetStateCallback([&observed](MediaState state) { observed = state; });

  player().SetSource(NEOFLUX_TEST_MEDIA_PATH);
  player().InitRender();
  player().Play();
  ASSERT_TRUE(WaitForState(player(), MediaState::kPlaying));

  EXPECT_EQ(observed, MediaState::kPlaying);
}

// ---------------------------------------------------------------------------
// Software render path observability tests.
//
// The following cases exercise the mpv_render_context lifecycle, the
// set_update_callback wiring, the frame-image publication contract and the
// frame callback hand-off. They drive the real render path against the real
// sample.mp4, exactly like MediaWidget does through the render pump.
// ---------------------------------------------------------------------------

// Render context lifecycle: InitRender() with no source must not crash and must
// yield no image; a double InitRender() must be a no-op (guarded by
// render_initialized); and tearing the player down afterwards must not crash or
// double-free the render context / frame buffers.
TEST_F(MpvMediaPlayerTest, RenderContextLifecycle) {
  // No source loaded: the render context exists but no frame has been decoded.
  player().InitRender();
  EXPECT_EQ(player().UpdateFrame(), 0U);
  EXPECT_EQ(player().GetVideoWidth(), 0);
  EXPECT_EQ(player().GetVideoHeight(), 0);

  // Re-initialization must be a no-op (render_initialized guard), not a second
  // mpv_render_context_create that would leak / double-free.
  player().InitRender();
  EXPECT_EQ(player().UpdateFrame(), 0U);

  // Reaching here means the player (and its render context) destruct cleanly in
  // TearDown. Any double-free or use-after-free would abort the test binary.
  SUCCEED();
}

// TeardownRender() before InitRender() (or twice in a row) must be a no-op: the
// framework calls it unconditionally, even for a player whose render context was
// never created because libmpv failed to initialize.
TEST_F(MpvMediaPlayerTest, TeardownRenderWithoutInitIsSafe) {
  EXPECT_EQ(player().UpdateFrame(), 0U);
  player().TeardownRender();
  player().TeardownRender();
  // Still usable afterwards: the id contract is "no frame published".
  EXPECT_EQ(player().UpdateFrame(), 0U);
  SUCCEED();
}

// The mpv render update callback must fire (and be observable via
// GetRenderUpdateCount) once playback starts producing frames. The callback
// runs on an internal mpv thread, so poll briefly until the counter lands.
TEST_F(MpvMediaPlayerTest, UpdateCallbackFires) {
  ASSERT_EQ(player().GetRenderUpdateCount(), 0U);

  player().SetSource(NEOFLUX_TEST_MEDIA_PATH);
  player().InitRender();
  player().Play();

  ASSERT_TRUE(WaitForFirstFrame(player()))
      << "Player did not publish a first frame within timeout";

  // The callback may arrive on mpv's thread slightly after the first frame is
  // published; drive the render pump a little longer to let it land.
  const auto deadline = std::chrono::steady_clock::now() + kStateTimeout;
  while (std::chrono::steady_clock::now() < deadline &&
         player().GetRenderUpdateCount() == 0U) {
    (void)player().UpdateFrame();
    std::this_thread::sleep_for(kPollInterval);
  }
  EXPECT_GT(player().GetRenderUpdateCount(), 0U);
}

// After a first frame is published the frame-image id must be valid (non-zero)
// and the reported video dimensions must be plausible (the staged sample.mp4 is
// a small clip, so assert a sane upper bound).
TEST_F(MpvMediaPlayerTest, FrameImageIdIsValid) {
  player().SetSource(NEOFLUX_TEST_MEDIA_PATH);
  player().InitRender();
  player().Play();
  ASSERT_TRUE(WaitForFirstFrame(player()))
      << "Player did not publish a first frame within timeout";

  const std::uint32_t image_id = player().UpdateFrame();
  EXPECT_NE(image_id, 0U);
  EXPECT_GT(player().GetVideoWidth(), 0);
  EXPECT_LT(player().GetVideoWidth(), 4096);
  EXPECT_GT(player().GetVideoHeight(), 0);
  EXPECT_LT(player().GetVideoHeight(), 4096);

  // The id must be stable across frames: the latest frame is re-bound to it
  // instead of a new id being handed out per frame.
  EXPECT_EQ(player().UpdateFrame(), image_id);
}

// A registered FrameCallback must be invoked on the render thread with the
// published frame-image id and its (positive) width/height for each new frame.
TEST_F(MpvMediaPlayerTest, FrameCallbackReceivesImageId) {
  int calls = 0;
  std::uint32_t seen_image_id = 0;
  int seen_width = 0;
  int seen_height = 0;
  player().SetFrameCallback([&](std::uint32_t image_id, int w, int h) {
    seen_image_id = image_id;
    seen_width = w;
    seen_height = h;
    ++calls;
  });

  player().SetSource(NEOFLUX_TEST_MEDIA_PATH);
  player().InitRender();
  player().Play();
  ASSERT_TRUE(WaitForFirstFrame(player()))
      << "Player did not publish a first frame within timeout";

  EXPECT_GT(calls, 0);
  EXPECT_NE(seen_image_id, 0U);
  EXPECT_GT(seen_width, 0);
  EXPECT_GT(seen_height, 0);
}

#else  // !NEOFLUX_HAS_MPV

// Placeholder so the test binary still links when libmpv is absent. The
// SetUp() skip above makes the suite report as skipped.
TEST_F(MpvMediaPlayerTest, SkippedWithoutLibmpv) { SUCCEED(); }

#endif  // NEOFLUX_HAS_MPV

}  // namespace
}  // namespace neoflux
