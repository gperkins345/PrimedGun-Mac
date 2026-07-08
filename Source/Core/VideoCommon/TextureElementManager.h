// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Common/CommonTypes.h"
#include "VideoCommon/ShaderHunter.h"

class TextureElementManager
{
public:
  using HandlingType = ShaderHunter::HandlingType;

  struct TextureElementOverride
  {
    std::string name;
    std::string comments;
    HandlingType handling = HandlingType::Skip;
    int layer = -1;
    float element_depth = -1.0f;
    float units_per_meter = -1.0f;
    std::vector<u64> texture_hashes;
    bool enabled = true;
  };

  static TextureElementManager& GetInstance();

  static std::vector<TextureElementOverride> LoadOverridesFromINI(
      const std::string& game_id, std::optional<u16> revision = std::nullopt);
  static void SaveOverridesToINI(const std::string& game_id,
                                 const std::vector<TextureElementOverride>& overrides);

  void LoadOverrides(const std::string& game_id);
  void LoadOverridesIfNeeded(const std::string& game_id);
  bool HasOverrides() const;
  bool NeedsTextureHashes() const;

  bool ShouldSkipByTexture(const std::array<u64, 8>& bound) const;
  HandlingType GetHandlingForTextures(const std::array<u64, 8>& bound, int* layer,
                                      float* element_depth, float* units_per_meter) const;

  void OnFrameEnd() {}

private:
  TextureElementManager() = default;

  struct ResolvedHandling
  {
    HandlingType handling = HandlingType::Skip;
    int layer = -1;
    float element_depth = -1.0f;
    float units_per_meter = -1.0f;
  };

  mutable std::mutex m_mutex;
  std::vector<TextureElementOverride> m_overrides;
  std::unordered_map<u64, ResolvedHandling> m_texture_handling;
  std::string m_loaded_game_id;
  std::atomic_bool m_has_overrides = false;
};
