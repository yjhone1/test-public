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

#ifndef REMOTEVIEWERWINDOW_H
#define REMOTEVIEWERWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QScrollArea>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QToolBar>
#include "remoteclient.h"

class RemoteCanvasWidget : public QWidget {
  Q_OBJECT

public:
  explicit RemoteCanvasWidget(QWidget* parent = nullptr);

  void setPixmap(const QPixmap& pixmap);
  void setRemoteScreenSize(const QSize& size);
  void setFitToWindow(bool fit);
  void setControlEnabled(bool enabled);

  bool isControlEnabled() const { return m_controlEnabled; }
  bool isFitToWindow() const { return m_fitToWindow; }

signals:
  void mouseMoved(quint32 x, quint32 y);
  void mouseButtonPressed(RemoteMouseEventType type, Qt::MouseButton button, quint32 x, quint32 y);
  void mouseButtonReleased(RemoteMouseEventType type, Qt::MouseButton button, quint32 x, quint32 y);
  void mouseWheeled(qint32 delta, quint32 x, quint32 y);
  void keyPressed(RemoteKeyEventType type, quint32 key, Qt::KeyboardModifiers modifiers);
  void keyReleased(RemoteKeyEventType type, quint32 key, Qt::KeyboardModifiers modifiers);

protected:
  void paintEvent(QPaintEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;

private:
  bool translateCoordinates(const QPoint& localPos, QPoint& hostPos);

  QPixmap m_pixmap;
  QSize m_remoteScreenSize;
  bool m_fitToWindow;
  bool m_controlEnabled;
};

class RemoteViewerWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit RemoteViewerWindow(const QString& targetUserId, const QString& targetUserName, QWidget* parent = nullptr);
  ~RemoteViewerWindow();

  void connectToHost(const QString& host, quint16 port);

signals:
  void viewerClosed(const QString& targetUserId);

protected:
  void closeEvent(QCloseEvent* event) override;

private slots:
  void onConnected();
  void onDisconnected();
  void onConnectionError(const QString& errorMsg);
  void onInitInfoReceived(quint32 screenWidth, quint32 screenHeight);
  void onFrameReceived(const QPixmap& pixmap);

  void onControlToggled(bool checked);
  void onFitToggled(bool checked);
  void onDisconnectClicked();

private:
  QString m_targetUserId;
  QString m_targetUserName;

  RemoteClient* m_client;
  RemoteCanvasWidget* m_canvas;
  QScrollArea* m_scrollArea;

  QCheckBox* m_chkControl;
  QCheckBox* m_chkFit;
  QLabel* m_lblStatus;
  QPushButton* m_btnDisconnect;
  int m_frameCount;
};

#endif // REMOTEVIEWERWINDOW_H
