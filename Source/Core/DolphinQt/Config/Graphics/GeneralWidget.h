// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include <QWidget>

class ConfigBool;
class ConfigChoice;
class ConfigInteger;
class ConfigRadioInt;
class ConfigStringChoice;
class ConfigText;
class GraphicsPane;
class QLabel;
class ToolTipComboBox;

namespace Config
{
class Layer;
}  // namespace Config

class GeneralWidget final : public QWidget
{
  Q_OBJECT
public:
  explicit GeneralWidget(GraphicsPane* gfx_pane);

signals:
  void BackendChanged(const QString& backend);

private:
  void BackendWarning();

  void CreateWidgets();
  void ToggleCustomAspectRatio(int index);
  void ConnectWidgets();
  void AddDescriptions();

  void OnBackendChanged(const QString& backend_name);
  void OnEmulationStateChanged(bool running);

  // Video
  ConfigStringChoice* m_backend_combo;
  ToolTipComboBox* m_adapter_combo;
  ConfigChoice* m_aspect_combo;
  QLabel* m_custom_aspect_label;
  ConfigInteger* m_custom_aspect_width;
  ConfigInteger* m_custom_aspect_height;
  ConfigBool* m_enable_vsync;
  ConfigBool* m_enable_fullscreen;

  // Options
  ConfigBool* m_autoadjust_window_size;
  ConfigBool* m_render_main_window;

  std::array<ConfigRadioInt*, 4> m_shader_compilation_mode{};
  ConfigBool* m_wait_for_shaders;

  // Groovy MiSTer
  ConfigBool* m_groovy_enable;
  ConfigText* m_groovy_ip;
  ConfigBool* m_groovy_fill_screen;
  ConfigBool* m_groovy_auto_vsync;
  ConfigInteger* m_groovy_vsync;
  ConfigBool* m_groovy_lz4;
  ConfigBool* m_groovy_lz4_delta;
  ConfigBool* m_groovy_jumbo;
  ConfigBool* m_groovy_prog_fb;
  ConfigBool* m_groovy_audio;
  ConfigBool* m_groovy_preview;
  ConfigBool* m_groovy_console_size;
  ConfigBool* m_groovy_prefer_240p;
  std::array<ConfigRadioInt*, 4> m_groovy_video_mode{};

  int m_previous_backend = 0;
  Config::Layer* m_game_layer = nullptr;
};
