// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <vector>

#include <QImage>
#include <QWidget>

class QTimer;

namespace Core
{
enum class State;
}

// Groovy MiSTer: a separate window on the PC that shows the picture being sent to the MiSTer.
// It's a normal top-level window with its own title, so it can be shared on Discord or captured
// by streaming software. Enabled with "Show PC Preview Window" in the Groovy MiSTer settings.
class GroovyPreviewWindow final : public QWidget
{
  Q_OBJECT
public:
  GroovyPreviewWindow();

protected:
  void paintEvent(QPaintEvent* event) override;
  void closeEvent(QCloseEvent* event) override;

private:
  void OnEmulationStateChanged(Core::State state);
  void UpdateVisibility();
  void PollFrame();

  QTimer* m_timer;
  QImage m_image;
  std::vector<uint8_t> m_buffer;
  uint32_t m_last_frame_id = 0;
  bool m_emulation_active = false;
  bool m_closed_by_user = false;
};
