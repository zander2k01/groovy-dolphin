// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Config/GroovyPane.h"


#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "Common/Config/Config.h"
#include "Common/Config/Layer.h"
#include "Core/Config/GraphicsSettings.h"
#include "Core/Core.h"
#include "Core/System.h"

#include "DolphinQt/Config/ConfigControls/ConfigBool.h"
#include "DolphinQt/Config/ConfigControls/ConfigChoice.h"
#include "DolphinQt/Config/ConfigControls/ConfigInteger.h"
#include "DolphinQt/Config/ConfigControls/ConfigText.h"
#include "DolphinQt/Settings.h"

GroovyPane::GroovyPane(Config::Layer* game_layer) : m_game_layer{game_layer}
{
  CreateWidgets();

  connect(&Settings::Instance(), &Settings::EmulationStateChanged, this, [this](Core::State state) {
    OnEmulationStateChanged(state != Core::State::Uninitialized);
  });
  OnEmulationStateChanged(!Core::IsUninitialized(Core::System::GetInstance()));
}

void GroovyPane::CreateWidgets()
{
  auto* main_layout = new QVBoxLayout;

  auto* intro = new QLabel(
      tr("Plays games on a CRT through a MiSTer running the Groovy core. The picture, sound and "
         "controllers plugged into the MiSTer all go over the network cable. Picture size, "
         "compression and timing are set automatically."));
  intro->setWordWrap(true);
  main_layout->addWidget(intro);

  // Connection
  auto* connection_box = new QGroupBox(tr("Connection"));
  auto* connection_layout = new QFormLayout;
  connection_box->setLayout(connection_layout);

  m_enable = new ConfigBool(tr("Enable MiSTer Output"), Config::GFX_GROOVY_ENABLE, m_game_layer);
  m_enable->SetDescription(
      tr("Sends the game to the MiSTer instead of only showing it on the PC. Load Utility > Groovy "
         "on the MiSTer; Dolphin keeps retrying until it answers."));
  connection_layout->addRow(m_enable);

  m_ip = new ConfigText(Config::GFX_GROOVY_IP, m_game_layer);
  m_ip->setPlaceholderText(tr("e.g. 192.168.1.105"));
  connection_layout->addRow(tr("MiSTer IP:"), m_ip);

  auto* start_core = new ConfigBool(tr("Start the Groovy Core on the MiSTer Automatically"),
                                    Config::GFX_GROOVY_START_CORE, m_game_layer);
  start_core->SetDescription(
      tr("If the MiSTer doesn't answer, ask it to start the Groovy core, even if another core is "
         "running. Needs the small groovy_wake helper installed on the MiSTer (see "
         "Main_MiSTer_Groovy on GitHub); without it nothing happens."));
  connection_layout->addRow(start_core);

  // Picture
  auto* picture_box = new QGroupBox(tr("Picture"));
  auto* picture_layout = new QVBoxLayout;
  picture_box->setLayout(picture_layout);

  // Values are Config::GroovyDisplay.
  m_display = new ConfigChoice({tr("CRT TV (15 kHz), default"),
                                tr("CRT TV (15 kHz), 240p for every game"),
                                tr("PC CRT monitor (31 kHz)"),
                                tr("Modern TV through the MiSTer's HDMI"), tr("Custom")},
                               Config::GFX_GROOVY_DISPLAY, m_game_layer);
  m_display->SetTitle(tr("Display"));
  m_display->SetDescription(
      tr("What the MiSTer is connected to. Each type is a preset that fills in the Output "
         "Settings below. To change those settings yourself, choose Custom.<br><br>"
         "<b>CRT TV (15 kHz)</b>: regular CRT TVs and PVMs. 480i, 240p for retro games that have "
         "it turned on, and 576i for 50 Hz PAL games.<br><br>"
         "<b>CRT TV, 240p for every game</b>: like above, but every game plays at 240p (288p for "
         "PAL). Sharper and without interlace flicker, but with half the lines.<br><br>"
         "<b>PC CRT monitor (31 kHz)</b>: VGA monitors. 480p. Never use this with a regular CRT "
         "TV: it can't display it, and a 31 kHz signal can strain a 15 kHz TV.<br><br>"
         "<b>Modern TV through the MiSTer's HDMI</b>: sends full frames instead of interlaced "
         "fields, which the MiSTer's scaler handles better.<br><br>"
         "<b>Custom</b>: unlocks the Output Settings below, starting from the type you had "
         "selected before.<br><br>"
         "<dolphin_emphasis>If unsure, select CRT TV (15 kHz).</dolphin_emphasis>"));
  auto* display_layout = new QFormLayout;
  display_layout->addRow(tr("Display:"), m_display);
  picture_layout->addLayout(display_layout);

  m_prefer_240p = new ConfigBool(tr("Use 240p for This Game (retro/2D games)"),
                                 Config::GFX_GROOVY_PREFER_240P, m_game_layer);
  m_prefer_240p->SetDescription(
      tr("With a CRT TV display, plays this game at 240p instead of 480i, like the original "
         "consoles for retro collections and 2D games. Recommended games have this turned on "
         "automatically; turn it off in the game's Properties if you prefer 480i. Ignored for "
         "PC monitors and 50 Hz games.<br><br><dolphin_emphasis>If unsure, leave this "
         "unchecked.</dolphin_emphasis>"));
  picture_layout->addWidget(m_prefer_240p);

  // Output settings: filled in by the display type, editable with Custom.
  m_custom_box = new QGroupBox(tr("Output Settings (choose Custom in Display to change these)"));
  auto* custom_layout = new QVBoxLayout;
  m_custom_box->setLayout(custom_layout);

  // Values are Config::GroovyVideoMode.
  m_video_mode = new ConfigChoice({tr("15 kHz: 480i (576i for PAL)"),
                                   tr("15 kHz: 240p (288p for PAL)"),
                                   tr("15 kHz: 480i, always 60 Hz (NTSC only)"),
                                   tr("31 kHz: 480p (PC CRT monitors only)")},
                                  Config::GFX_GROOVY_VIDEO_MODE, m_game_layer);
  m_video_mode->SetTitle(tr("Video Mode"));
  m_video_mode->SetDescription(
      tr("The video mode the MiSTer outputs.<br><br>"
         "<b>480i</b>: what most GameCube and Wii games use. Games with \"Use 240p for This "
         "Game\" turned on play at 240p.<br><br>"
         "<b>240p</b>: every game at 240p (288p for PAL). Sharper and without interlace flicker, "
         "but with half the lines.<br><br>"
         "<b>480i, always 60 Hz</b>: for TVs that can't show 50 Hz. 50 Hz PAL games are sent at "
         "60 Hz too, so they run about 20% faster.<br><br>"
         "<b>480p</b>: 31 kHz, for VGA monitors only. Never use this with a regular CRT TV: it "
         "can't display it, and a 31 kHz signal can strain a 15 kHz TV.<br><br>"
         "<dolphin_emphasis>If unsure, select 480i.</dolphin_emphasis>"));
  auto* video_mode_layout = new QFormLayout;
  video_mode_layout->addRow(tr("Video Mode:"), m_video_mode);
  custom_layout->addLayout(video_mode_layout);

  m_prog_fb = new ConfigBool(tr("Progressive Framebuffer"), Config::GFX_GROOVY_PROG_FB,
                             m_game_layer);
  m_prog_fb->SetDescription(
      tr("Sends full frames instead of interlaced fields. Meant for the MiSTer's HDMI output or a "
         "scandoubler. On a CRT, interlaced modes look best with this unchecked."
         "<br><br><dolphin_emphasis>If unsure, leave this unchecked.</dolphin_emphasis>"));
  custom_layout->addWidget(m_prog_fb);

  m_console_size = new ConfigBool(tr("Real Console Picture Size"), Config::GFX_GROOVY_CONSOLE_SIZE,
                                  m_game_layer);
  m_console_size->SetDescription(
      tr("Uses the same picture width and position as a real GameCube or Wii, taken from the "
         "game itself. When unchecked, the picture fills the whole line, which is about 10% "
         "wider than on real hardware, so its edges can disappear into the TV's overscan."
         "<br><br><dolphin_emphasis>If unsure, leave this checked.</dolphin_emphasis>"));
  custom_layout->addWidget(m_console_size);

  m_scale = new ConfigBool(tr("Scale to Fit the MiSTer's Resolution"), Config::GFX_GROOVY_SCALE,
                           m_game_layer);
  m_scale->SetDescription(
      tr("Resizes each frame to the MiSTer's resolution when the two don't match. When "
         "unchecked, frames are sent at their own size: smaller ones get black borders and "
         "bigger ones are cropped. Frames above 1x internal resolution are always scaled."
         "<br><br><dolphin_emphasis>If unsure, leave this checked.</dolphin_emphasis>"));
  custom_layout->addWidget(m_scale);

  m_lz4 = new ConfigBool(tr("Compress Frames (LZ4)"), Config::GFX_GROOVY_LZ4, m_game_layer);
  m_lz4->SetDescription(
      tr("Compresses each frame before sending it, which uses much less network bandwidth for "
         "very little CPU time.<br><br><dolphin_emphasis>If unsure, leave this checked."
         "</dolphin_emphasis>"));
  custom_layout->addWidget(m_lz4);

  m_delta = new ConfigBool(tr("Delta Frames (send only changes)"), Config::GFX_GROOVY_LZ4_DELTA,
                           m_game_layer);
  m_delta->SetDescription(
      tr("Sends only the parts of each frame that changed, to save bandwidth on slower "
         "connections such as Wi-Fi. Requires compression. If a packet is lost, parts of an "
         "old frame can stay on screen until that area changes again.<br><br>"
         "<dolphin_emphasis>If unsure, leave this unchecked.</dolphin_emphasis>"));
  custom_layout->addWidget(m_delta);

  m_jumbo = new ConfigBool(tr("Jumbo Frames"), Config::GFX_GROOVY_JUMBO, m_game_layer);
  m_jumbo->SetDescription(
      tr("Sends bigger network packets (3800 bytes instead of 1500), which slightly lowers "
         "overhead. Only works if jumbo frames are enabled on the PC's network adapter and every "
         "switch in between; otherwise the MiSTer won't receive anything.<br><br>"
         "<dolphin_emphasis>If unsure, leave this unchecked.</dolphin_emphasis>"));
  custom_layout->addWidget(m_jumbo);

  m_last_preset = m_display->currentIndex();
  connect(m_display, &QComboBox::currentIndexChanged, this, &GroovyPane::OnDisplayChanged);
  connect(m_lz4, &QCheckBox::toggled, this, &GroovyPane::UpdateCustomState);
  // The controls re-read their stored values when any setting changes; show the preset's values
  // again afterwards (this is connected after them, so it runs last).
  connect(&Settings::Instance(), &Settings::ConfigChanged, this, &GroovyPane::UpdateCustomState);

  // Sound
  auto* sound_box = new QGroupBox(tr("Sound"));
  auto* sound_layout = new QVBoxLayout;
  sound_box->setLayout(sound_layout);

  m_audio = new ConfigBool(tr("Send Audio"), Config::GFX_GROOVY_AUDIO, m_game_layer);
  m_audio->SetDescription(
      tr("Plays the game's sound through the MiSTer. Audio also has to be turned on in the Groovy "
         "core's own menu on the MiSTer (Audio > Audio: On)."));
  sound_layout->addWidget(m_audio);

  // Advanced
  auto* advanced_box = new QGroupBox(tr("Advanced"));
  auto* advanced_layout = new QVBoxLayout;
  advanced_box->setLayout(advanced_layout);

  auto* vsync_layout = new QHBoxLayout;
  m_auto_vsync = new ConfigBool(tr("Automatic V-Sync (or choose line to sync)"),
                                Config::GFX_GROOVY_AUTO_VSYNC, m_game_layer);
  m_auto_vsync->SetDescription(
      tr("Latency tuning. When checked, the MiSTer picks when to show each frame by itself. "
         "When unchecked, frames are synced to the scanline chosen on the right."
         "<br><br><dolphin_emphasis>If unsure, leave this as it is.</dolphin_emphasis>"));
  m_vsync = new ConfigInteger(1, 400, Config::GFX_GROOVY_VSYNC, m_game_layer);
  vsync_layout->addWidget(m_auto_vsync);
  vsync_layout->addWidget(m_vsync);
  vsync_layout->addStretch();
  advanced_layout->addLayout(vsync_layout);

  m_preview = new ConfigBool(tr("Show Sent Frame Window (troubleshooting)"),
                             Config::GFX_GROOVY_PREVIEW, m_game_layer);
  m_preview->SetDescription(
      tr("Opens a separate window that shows each frame exactly as it's sent to the MiSTer, "
         "before compression. If this window looks right but the CRT doesn't, the problem is in "
         "sending the frame, not in Dolphin. Not needed for streaming: Dolphin's normal game "
         "window shows the same picture.<br><br><dolphin_emphasis>If unsure, leave this "
         "unchecked.</dolphin_emphasis>"));
  advanced_layout->addWidget(m_preview);

  main_layout->addWidget(connection_box);
  main_layout->addWidget(picture_box);
  main_layout->addWidget(m_custom_box);
  main_layout->addWidget(sound_box);
  main_layout->addWidget(advanced_box);
  main_layout->addStretch();

  setLayout(main_layout);
  UpdateCustomState();
}

bool GroovyPane::IsCustom() const
{
  return m_display->currentIndex() == static_cast<int>(Config::GroovyDisplay::Custom);
}

void GroovyPane::ShowValues(const Config::GroovyOutputSettings& s)
{
  // Only changes what's shown; nothing is saved.
  const QSignalBlocker b1(m_video_mode), b2(m_prog_fb), b3(m_console_size), b4(m_scale),
      b5(m_lz4), b6(m_delta), b7(m_jumbo);
  m_video_mode->setCurrentIndex(static_cast<int>(s.mode));
  m_prog_fb->setChecked(s.progressive_fb);
  m_console_size->setChecked(s.console_size);
  m_scale->setChecked(s.scale);
  m_lz4->setChecked(s.lz4);
  m_delta->setChecked(s.delta);
  m_jumbo->setChecked(s.jumbo);
}

template <typename T>
void GroovyPane::SaveSetting(const Config::Info<T>& info, const T& value)
{
  if (m_game_layer)
    m_game_layer->Set(info.GetLocation(), value);
  else
    Config::SetBaseOrCurrent(info, value);
}

void GroovyPane::OnDisplayChanged(int index)
{
  if (index == static_cast<int>(Config::GroovyDisplay::Custom))
  {
    // Start Custom from the preset that was selected before, so nothing changes until the user
    // changes something.
    const auto s = Config::GetGroovyPreset(static_cast<Config::GroovyDisplay>(m_last_preset));
    SaveSetting(Config::GFX_GROOVY_VIDEO_MODE, s.mode);
    SaveSetting(Config::GFX_GROOVY_PROG_FB, s.progressive_fb);
    SaveSetting(Config::GFX_GROOVY_CONSOLE_SIZE, s.console_size);
    SaveSetting(Config::GFX_GROOVY_SCALE, s.scale);
    SaveSetting(Config::GFX_GROOVY_LZ4, s.lz4);
    SaveSetting(Config::GFX_GROOVY_LZ4_DELTA, s.delta);
    SaveSetting(Config::GFX_GROOVY_JUMBO, s.jumbo);
    Config::OnConfigChanged();
  }
  else
  {
    m_last_preset = index;
  }
  UpdateCustomState();
}

void GroovyPane::UpdateCustomState()
{
  const bool custom = IsCustom();
  if (!custom && m_display->currentIndex() >= 0)
  {
    ShowValues(
        Config::GetGroovyPreset(static_cast<Config::GroovyDisplay>(m_display->currentIndex())));
  }

  m_custom_box->setEnabled(custom && !m_running);
  // Delta frames only work with compression.
  m_delta->setEnabled(m_lz4->isChecked());
}

void GroovyPane::OnEmulationStateChanged(bool running)
{
  // The connection and display type are only set up when a game starts.
  m_enable->setEnabled(!running);
  m_ip->setEnabled(!running);
  m_display->setEnabled(!running);
  m_running = running;
  UpdateCustomState();
}
