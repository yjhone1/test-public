/****************************************************************************
**
** This file is part of LAN Messenger.
** 
** Copyright (c) 2010 - 2012 Qualia Digital Solutions.
** 
** Contact:  qualiatech@gmail.com
** 
** LAN Messenger is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** LAN Messenger is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with LAN Messenger.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "remoteviewerwindow.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>

RemoteCanvasWidget::RemoteCanvasWidget(QWidget* parent)
  : QWidget(parent),
    m_fitToWindow(true),
    m_controlEnabled(false)
{
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);
  setStyleSheet("background-color: #202020;");
}

void RemoteCanvasWidget::setPixmap(const QPixmap& pixmap) {
  m_pixmap = pixmap;
  if(m_remoteScreenSize.isEmpty()) {
    m_remoteScreenSize = pixmap.size();
  }
  if(!m_fitToWindow) {
    setFixedSize(m_pixmap.size());
  }
  update();
}

void RemoteCanvasWidget::setRemoteScreenSize(const QSize& size) {
  m_remoteScreenSize = size;
  update();
}

void RemoteCanvasWidget::setFitToWindow(bool fit) {
  m_fitToWindow = fit;
  if(m_fitToWindow) {
    setMinimumSize(320, 240);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  } else if(!m_pixmap.isNull()) {
    setFixedSize(m_pixmap.size());
  }
  update();
}

void RemoteCanvasWidget::setControlEnabled(bool enabled) {
  m_controlEnabled = enabled;
  if(m_controlEnabled) {
    setCursor(Qt::ArrowCursor);
  } else {
    setCursor(Qt::ForbiddenCursor);
  }
}

bool RemoteCanvasWidget::translateCoordinates(const QPoint& localPos, QPoint& hostPos) {
  if(m_pixmap.isNull() || m_remoteScreenSize.isEmpty())
    return false;

  if(m_fitToWindow) {
    QSize scaledSize = m_pixmap.size().scaled(size(), Qt::KeepAspectRatio);
    int offsetX = (width() - scaledSize.width()) / 2;
    int offsetY = (height() - scaledSize.height()) / 2;

    if(localPos.x() < offsetX || localPos.x() > offsetX + scaledSize.width() ||
       localPos.y() < offsetY || localPos.y() > offsetY + scaledSize.height()) {
      return false;
    }

    int hostX = (localPos.x() - offsetX) * m_remoteScreenSize.width() / scaledSize.width();
    int hostY = (localPos.y() - offsetY) * m_remoteScreenSize.height() / scaledSize.height();
    hostPos = QPoint(hostX, hostY);
    return true;
  } else {
    hostPos = localPos;
    return (hostPos.x() >= 0 && hostPos.x() < m_remoteScreenSize.width() &&
            hostPos.y() >= 0 && hostPos.y() < m_remoteScreenSize.height());
  }
}

void RemoteCanvasWidget::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event);
  QPainter painter(this);
  painter.fillRect(rect(), QColor(32, 32, 32));

  if(m_pixmap.isNull()) {
    painter.setPen(Qt::white);
    painter.drawText(rect(), Qt::AlignCenter, tr("Waiting for remote screen..."));
    return;
  }

  if(m_fitToWindow) {
    QSize scaledSize = m_pixmap.size().scaled(size(), Qt::KeepAspectRatio);
    int offsetX = (width() - scaledSize.width()) / 2;
    int offsetY = (height() - scaledSize.height()) / 2;
    QRect targetRect(offsetX, offsetY, scaledSize.width(), scaledSize.height());
    painter.drawPixmap(targetRect, m_pixmap);
  } else {
    painter.drawPixmap(0, 0, m_pixmap);
  }
}

void RemoteCanvasWidget::mouseMoveEvent(QMouseEvent* event) {
  if(!m_controlEnabled)
    return;

  QPoint hostPos;
  if(translateCoordinates(event->pos(), hostPos)) {
    emit mouseMoved(hostPos.x(), hostPos.y());
  }
}

void RemoteCanvasWidget::mousePressEvent(QMouseEvent* event) {
  setFocus();
  if(!m_controlEnabled)
    return;

  QPoint hostPos;
  if(translateCoordinates(event->pos(), hostPos)) {
    emit mouseButtonPressed(REMOTE_MOUSE_PRESS, event->button(), hostPos.x(), hostPos.y());
  }
}

void RemoteCanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
  if(!m_controlEnabled)
    return;

  QPoint hostPos;
  if(translateCoordinates(event->pos(), hostPos)) {
    emit mouseButtonReleased(REMOTE_MOUSE_RELEASE, event->button(), hostPos.x(), hostPos.y());
  }
}

void RemoteCanvasWidget::wheelEvent(QWheelEvent* event) {
  if(!m_controlEnabled)
    return;

  QPoint hostPos;
  if(translateCoordinates(event->position().toPoint(), hostPos)) {
    emit mouseWheeled(event->angleDelta().y(), hostPos.x(), hostPos.y());
  }
}

void RemoteCanvasWidget::keyPressEvent(QKeyEvent* event) {
  if(!m_controlEnabled)
    return;

  emit keyPressed(REMOTE_KEY_PRESS, event->key(), event->modifiers());
  event->accept();
}

void RemoteCanvasWidget::keyReleaseEvent(QKeyEvent* event) {
  if(!m_controlEnabled)
    return;

  emit keyReleased(REMOTE_KEY_RELEASE, event->key(), event->modifiers());
  event->accept();
}

// --------------------------------------------------------------------------
// RemoteViewerWindow Implementation
// --------------------------------------------------------------------------

RemoteViewerWindow::RemoteViewerWindow(const QString& targetUserId, const QString& targetUserName, QWidget* parent)
  : QMainWindow(parent),
    m_targetUserId(targetUserId),
    m_targetUserName(targetUserName),
    m_client(new RemoteClient(this)),
    m_canvas(new RemoteCanvasWidget(this)),
    m_scrollArea(new QScrollArea(this)),
    m_frameCount(0)
{
  setWindowTitle(tr("Remote Desktop - %1").arg(m_targetUserName.isEmpty() ? m_targetUserId : m_targetUserName));
  resize(1024, 768);

  // Setup toolbar
  QToolBar* toolBar = addToolBar(tr("Remote Controls"));
  toolBar->setMovable(false);

  m_chkControl = new QCheckBox(tr("Enable Control"), this);
  m_chkControl->setChecked(false);
  toolBar->addWidget(m_chkControl);
  toolBar->addSeparator();

  m_chkFit = new QCheckBox(tr("Fit to Window"), this);
  m_chkFit->setChecked(true);
  toolBar->addWidget(m_chkFit);
  toolBar->addSeparator();

  m_lblStatus = new QLabel(tr("Connecting..."), this);
  toolBar->addWidget(m_lblStatus);

  QWidget* spacer = new QWidget(this);
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  toolBar->addWidget(spacer);

  m_btnDisconnect = new QPushButton(tr("Disconnect"), this);
  toolBar->addWidget(m_btnDisconnect);

  // Setup canvas in scroll area
  m_scrollArea->setWidget(m_canvas);
  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setAlignment(Qt::AlignCenter);
  setCentralWidget(m_scrollArea);

  // Connect UI controls
  connect(m_chkControl, &QCheckBox::toggled, this, &RemoteViewerWindow::onControlToggled);
  connect(m_chkFit, &QCheckBox::toggled, this, &RemoteViewerWindow::onFitToggled);
  connect(m_btnDisconnect, &QPushButton::clicked, this, &RemoteViewerWindow::onDisconnectClicked);

  // Connect canvas inputs to client
  connect(m_canvas, &RemoteCanvasWidget::mouseMoved, m_client, &RemoteClient::sendMouseMove);
  connect(m_canvas, &RemoteCanvasWidget::mouseButtonPressed, m_client, &RemoteClient::sendMouseButton);
  connect(m_canvas, &RemoteCanvasWidget::mouseButtonReleased, m_client, &RemoteClient::sendMouseButton);
  connect(m_canvas, &RemoteCanvasWidget::mouseWheeled, m_client, &RemoteClient::sendMouseWheel);
  connect(m_canvas, &RemoteCanvasWidget::keyPressed, m_client, &RemoteClient::sendKeyEvent);
  connect(m_canvas, &RemoteCanvasWidget::keyReleased, m_client, &RemoteClient::sendKeyEvent);

  // Connect client events
  connect(m_client, &RemoteClient::connected, this, &RemoteViewerWindow::onConnected);
  connect(m_client, &RemoteClient::disconnected, this, &RemoteViewerWindow::onDisconnected);
  connect(m_client, &RemoteClient::connectionError, this, &RemoteViewerWindow::onConnectionError);
  connect(m_client, &RemoteClient::initInfoReceived, this, &RemoteViewerWindow::onInitInfoReceived);
  connect(m_client, &RemoteClient::frameReceived, this, &RemoteViewerWindow::onFrameReceived);

  m_canvas->setControlEnabled(false);
  m_canvas->setFitToWindow(true);
}

RemoteViewerWindow::~RemoteViewerWindow() {
  if(m_client) {
    m_client->disconnectFromHost();
  }
}

void RemoteViewerWindow::connectToHost(const QString& host, quint16 port) {
  m_lblStatus->setText(tr("Connecting to %1:%2...").arg(host).arg(port));
  m_client->connectToHost(host, port);
}

void RemoteViewerWindow::closeEvent(QCloseEvent* event) {
  m_client->disconnectFromHost();
  emit viewerClosed(m_targetUserId);
  QMainWindow::closeEvent(event);
}

void RemoteViewerWindow::onConnected() {
  m_lblStatus->setText(tr("Connected"));
}

void RemoteViewerWindow::onDisconnected() {
  m_lblStatus->setText(tr("Disconnected"));
  QMessageBox::information(this, tr("Remote Desktop"), tr("The remote connection was closed."));
}

void RemoteViewerWindow::onConnectionError(const QString& errorMsg) {
  m_lblStatus->setText(tr("Error: %1").arg(errorMsg));
  QMessageBox::warning(this, tr("Connection Error"), tr("Failed to connect: %1").arg(errorMsg));
}

void RemoteViewerWindow::onInitInfoReceived(quint32 screenWidth, quint32 screenHeight) {
  m_canvas->setRemoteScreenSize(QSize(screenWidth, screenHeight));
  m_lblStatus->setText(tr("Connected (%1x%2)").arg(screenWidth).arg(screenHeight));
}

void RemoteViewerWindow::onFrameReceived(const QPixmap& pixmap) {
  m_frameCount++;
  m_canvas->setPixmap(pixmap);
}

void RemoteViewerWindow::onControlToggled(bool checked) {
  m_canvas->setControlEnabled(checked);
}

void RemoteViewerWindow::onFitToggled(bool checked) {
  m_canvas->setFitToWindow(checked);
}

void RemoteViewerWindow::onDisconnectClicked() {
  m_client->disconnectFromHost();
  close();
}
