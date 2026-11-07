// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once


#include <QWidget>

class ConfigBool;
class ConfigChoice;
class ConfigInteger;
class ConfigText;
class QGroupBox;

#include "Core/Config/GraphicsSettings.h"

namespace Config
{
class Layer;
}  // namespace Config

// Groovy MiSTer settings: streams the picture, sound and controller input between Dolphin and a
// MiSTer running the Groovy core, to play on a CRT. Shown in Settings, and in a game's Properties
// (with game_layer set) for per-game settings.
class GroovyPane final : public QWidget
{
  Q_OBJECT
public:
  explicit GroovyPane(Config::Layer* game_layer = nullptr);

private:
  void CreateWidgets();
  void OnEmulationStateChanged(bool running);
  void OnDisplayChanged(int index);
  void UpdateCustomState();
  bool IsCustom() const;
  void ShowValues(const Config::GroovyOutputSettings& s);
  template <typename T>
  void SaveSetting(const Config::Info<T>& info, const T& value);

  Config::Layer* m_game_layer;
  bool m_running = false;
  // Last non-Custom display type; Custom starts from its values.
  int m_last_preset = 0;

  ConfigBool* m_enable;
  ConfigText* m_ip;
  ConfigChoice* m_display;
  ConfigBool* m_prefer_240p;
  QGroupBox* m_custom_box;
  ConfigChoice* m_video_mode;
  ConfigBool* m_prog_fb;
  ConfigBool* m_console_size;
  ConfigBool* m_scale;
  ConfigBool* m_lz4;
  ConfigBool* m_delta;
  ConfigBool* m_jumbo;
  ConfigBool* m_audio;
  ConfigBool* m_auto_vsync;
  ConfigInteger* m_vsync;
  ConfigBool* m_preview;
};
