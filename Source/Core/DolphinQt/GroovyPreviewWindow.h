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

// Groovy MiSTer: a separate window on the PC that shows each frame exactly as it's sent to the
// MiSTer (before compression). Mostly for troubleshooting: if it looks right but the CRT doesn't,
// the problem is in sending. Enabled with "Show Sent Frame Window" in the Groovy MiSTer settings.
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
