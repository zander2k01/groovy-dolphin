// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Config/GroovyPane.h"

#include <utility>

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "Core/Config/GraphicsSettings.h"
#include "Core/Core.h"
#include "Core/System.h"

#include "DolphinQt/Config/ConfigControls/ConfigBool.h"
#include "DolphinQt/Config/ConfigControls/ConfigInteger.h"
#include "DolphinQt/Config/ConfigControls/ConfigRadio.h"
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

  // Picture
  auto* picture_box = new QGroupBox(tr("Picture"));
  auto* picture_layout = new QVBoxLayout;
  picture_box->setLayout(picture_layout);

  // Values are Config::GroovyVideoMode. GV_NTSC ("NTSC Only") is no longer offered: Auto handles
  // 60 Hz games the same way, and NTSC Only just made 50 Hz games run 20% fast.
  const std::array<std::pair<const char*, Config::GroovyVideoMode>, 3> modes = {{
      {QT_TR_NOOP("Auto NTSC/PAL (15 kHz TV)"), Config::GroovyVideoMode::GV_15KHZ},
      {QT_TR_NOOP("Force 240p (15 kHz TV)"), Config::GroovyVideoMode::GV_15KHZ_240P},
      {QT_TR_NOOP("VGA (31 kHz monitor only)"), Config::GroovyVideoMode::GV_VGA},
  }};
  for (size_t i = 0; i < modes.size(); i++)
  {
    m_video_mode[i] = new ConfigRadioInt(tr(modes[i].first), Config::GFX_GROOVY_VIDEO_MODE,
                                         static_cast<int>(modes[i].second), m_game_layer);
    picture_layout->addWidget(m_video_mode[i]);
  }
  m_video_mode[0]->SetDescription(
      tr("For CRT TVs. 60 Hz games play at 480i (or 240p for some), 50 Hz PAL games at 576i."
         "<br><br><dolphin_emphasis>If unsure, select this.</dolphin_emphasis>"));
  m_video_mode[1]->SetDescription(
      tr("Plays every game at 240p (288p for 50 Hz games), like older consoles. Sharper and "
         "without interlace flicker, but with half the lines."));
  m_video_mode[2]->SetDescription(
      tr("For 31 kHz monitors only. Never use this with a regular CRT TV: it can't display it, and "
         "a 31 kHz signal can strain a 15 kHz TV."));

  m_prefer_240p = new ConfigBool(tr("Use 240p for This Game (retro/2D games)"),
                                 Config::GFX_GROOVY_PREFER_240P, m_game_layer);
  m_prefer_240p->SetDescription(
      tr("With Auto, plays this game at 240p instead of 480i, like the original consoles for "
         "retro collections and 2D games. Recommended games have this turned on automatically; "
         "turn it off in the game's Properties if you prefer 480i. Ignored for VGA and 50 Hz "
         "games.<br><br><dolphin_emphasis>If unsure, leave this unchecked.</dolphin_emphasis>"));
  picture_layout->addWidget(m_prefer_240p);

  m_prog_fb = new ConfigBool(tr("Progressive Framebuffer (for scandoubler)"),
                             Config::GFX_GROOVY_PROG_FB, m_game_layer);
  m_prog_fb->SetDescription(
      tr("Only needed when the MiSTer's output goes through its scandoubler or HDMI. A CRT TV "
         "shows interlaced 480i natively.<br><br><dolphin_emphasis>If unsure, leave this "
         "unchecked.</dolphin_emphasis>"));
  picture_layout->addWidget(m_prog_fb);

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
  main_layout->addWidget(sound_box);
  main_layout->addWidget(advanced_box);
  main_layout->addStretch();

  setLayout(main_layout);
}

void GroovyPane::OnEmulationStateChanged(bool running)
{
  // The connection and framebuffer type are only set up when a game starts.
  m_enable->setEnabled(!running);
  m_ip->setEnabled(!running);
  m_prog_fb->setEnabled(!running);
}
