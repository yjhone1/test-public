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

#ifndef REMOTESERVER_H
#define REMOTESERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QByteArray>
#include <QLibrary>
#include "remotedefinitions.h"

class RemoteServer : public QObject {
  Q_OBJECT

public:
  explicit RemoteServer(QObject* parent = nullptr);
  ~RemoteServer();

  bool startServer(quint16 port = 0);
  void stopServer();
  quint16 serverPort() const;
  bool isClientConnected() const;

signals:
  void clientConnected(const QString& peerAddress);
  void clientDisconnected();
  void serverStopped();

private slots:
  void onNewConnection();
  void onClientDisconnected();
  void onReadyRead();
  void captureAndSendFrame();

private:
  void processPacket(quint8 type, const QByteArray& payload);
  void handleMousePacket(const RemoteMousePayload& mouse);
  void handleKeyPacket(const RemoteKeyPayload& key);
  void sendPacket(quint8 type, const QByteArray& payload);

  void initInputSimulator();
  void cleanupInputSimulator();

  QTcpServer* m_server;
  QTcpSocket* m_clientSocket;
  QTimer* m_frameTimer;
  QByteArray m_receiveBuffer;
  int m_jpegQuality;
  int m_frameIntervalMs;

  // Dynamic X11 input simulation on Linux
#ifdef Q_OS_LINUX
  QLibrary m_x11Lib;
  QLibrary m_xtstLib;
  void* m_display;
  bool m_x11Available;
#endif
};

#endif // REMOTESERVER_H
