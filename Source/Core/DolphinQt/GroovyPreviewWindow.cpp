// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/GroovyPreviewWindow.h"

#include <QCloseEvent>
#include <QPainter>
#include <QTimer>

#include "Common/Config/Config.h"
#include "Core/Config/GraphicsSettings.h"
#include "Core/Core.h"
#include "Core/System.h"

#include "DolphinQt/Resources.h"
#include "DolphinQt/Settings.h"

#include "VideoCommon/Mister.h"

GroovyPreviewWindow::GroovyPreviewWindow() : QWidget(nullptr)
{
  // No parent: this needs to be its own top-level window (with a taskbar entry) so screen
  // sharing tools list it separately from the main Dolphin window.
  setWindowTitle(tr("Groovy MiSTer Preview"));
  setWindowIcon(Resources::GetAppIcon());
  setAttribute(Qt::WA_OpaquePaintEvent);
  resize(640, 480);

  m_timer = new QTimer(this);
  m_timer->setInterval(16);
  connect(m_timer, &QTimer::timeout, this, &GroovyPreviewWindow::PollFrame);

  connect(&Settings::Instance(), &Settings::EmulationStateChanged, this,
          &GroovyPreviewWindow::OnEmulationStateChanged);
  connect(&Settings::Instance(), &Settings::ConfigChanged, this,
          &GroovyPreviewWindow::UpdateVisibility);

  OnEmulationStateChanged(Core::GetState(Core::System::GetInstance()));
}

void GroovyPreviewWindow::OnEmulationStateChanged(Core::State state)
{
  const bool active = state != Core::State::Uninitialized && state != Core::State::Stopping;
  if (active && !m_emulation_active)
  {
    // A new game started: show the window again even if it was closed during the last one.
    m_closed_by_user = false;
    m_image = QImage();
    m_last_frame_id = 0;
  }
  m_emulation_active = active;
  UpdateVisibility();
}

void GroovyPreviewWindow::UpdateVisibility()
{
  const bool wanted = m_emulation_active && !m_closed_by_user &&
                      Config::Get(Config::GFX_GROOVY_ENABLE) &&
                      Config::Get(Config::GFX_GROOVY_PREVIEW);
  if (wanted)
  {
    if (!isVisible())
      show();
    if (!m_timer->isActive())
      m_timer->start();
  }
  else
  {
    m_timer->stop();
    if (isVisible())
      hide();
  }
}

void GroovyPreviewWindow::PollFrame()
{
  int width = 0;
  int height = 0;
  if (!g_mister.GetPreview(&m_buffer, &width, &height, &m_last_frame_id))
    return;

  if (width <= 0 || height <= 0 || m_buffer.size() < static_cast<size_t>(width) * height * 3)
    return;

  // The MiSTer frame is stored as B, G, R bytes per pixel. The QImage wraps m_buffer without
  // copying, which is fine because m_buffer is only changed here, right before repainting.
  m_image = QImage(m_buffer.data(), width, height, width * 3, QImage::Format_BGR888);
  update();
}

void GroovyPreviewWindow::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.fillRect(rect(), Qt::black);

  if (m_image.isNull())
  {
    painter.setPen(Qt::white);
    painter.drawText(rect(), Qt::AlignCenter, tr("Waiting for the MiSTer..."));
    return;
  }

  // A CRT shows every mode at 4:3, whatever the pixel resolution, so match that.
  QSize target(4, 3);
  target.scale(size(), Qt::KeepAspectRatio);
  const QRect dest((width() - target.width()) / 2, (height() - target.height()) / 2,
                   target.width(), target.height());

  painter.setRenderHint(QPainter::SmoothPixmapTransform);
  painter.drawImage(dest, m_image);
}

void GroovyPreviewWindow::closeEvent(QCloseEvent* event)
{
  // Closing the window only hides it until the next game starts; the setting stays on.
  m_closed_by_user = true;
  m_timer->stop();
  event->accept();
}
