// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once


#include <QWidget>

class ConfigBool;
class ConfigChoice;
class ConfigInteger;
class ConfigText;

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

  Config::Layer* m_game_layer;

  ConfigBool* m_enable;
  ConfigText* m_ip;
  ConfigChoice* m_display;
  ConfigBool* m_prefer_240p;
  ConfigBool* m_audio;
  ConfigBool* m_auto_vsync;
  ConfigInteger* m_vsync;
  ConfigBool* m_preview;
};
