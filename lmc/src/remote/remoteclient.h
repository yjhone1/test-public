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

#ifndef REMOTECLIENT_H
#define REMOTECLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QPixmap>
#include <QByteArray>
#include "remotedefinitions.h"

class RemoteClient : public QObject {
  Q_OBJECT

public:
  explicit RemoteClient(QObject* parent = nullptr);
  ~RemoteClient();

  void connectToHost(const QString& host, quint16 port);
  void disconnectFromHost();
  bool isConnected() const;

  void sendMouseMove(quint32 x, quint32 y);
  void sendMouseButton(RemoteMouseEventType eventType, Qt::MouseButton button, quint32 x, quint32 y);
  void sendMouseWheel(qint32 delta, quint32 x, quint32 y);
  void sendKeyEvent(RemoteKeyEventType eventType, quint32 key, Qt::KeyboardModifiers modifiers);

signals:
  void connected();
  void disconnected();
  void connectionError(const QString& errorMessage);
  void initInfoReceived(quint32 screenWidth, quint32 screenHeight);
  void frameReceived(const QPixmap& pixmap);

private slots:
  void onConnected();
  void onDisconnected();
  void onReadyRead();
  void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
  void sendPacket(quint8 type, const QByteArray& payload);
  void processPacket(quint8 type, const QByteArray& payload);

  QTcpSocket* m_socket;
  QByteArray m_receiveBuffer;
};

#endif // REMOTECLIENT_H
