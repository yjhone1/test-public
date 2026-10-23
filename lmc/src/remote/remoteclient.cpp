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

#include "remoteclient.h"
#include <QDebug>

RemoteClient::RemoteClient(QObject* parent)
  : QObject(parent),
    m_socket(new QTcpSocket(this))
{
  connect(m_socket, &QTcpSocket::connected, this, &RemoteClient::onConnected);
  connect(m_socket, &QTcpSocket::disconnected, this, &RemoteClient::onDisconnected);
  connect(m_socket, &QTcpSocket::readyRead, this, &RemoteClient::onReadyRead);
  connect(m_socket, &QTcpSocket::errorOccurred, this, &RemoteClient::onErrorOccurred);
}

RemoteClient::~RemoteClient() {
  disconnectFromHost();
}

void RemoteClient::connectToHost(const QString& host, quint16 port) {
  disconnectFromHost();
  m_receiveBuffer.clear();
  m_socket->connectToHost(host, port);
}

void RemoteClient::disconnectFromHost() {
  if(m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
    sendPacket(REMOTE_PKT_CLOSE, QByteArray());
    m_socket->disconnectFromHost();
  }
}

bool RemoteClient::isConnected() const {
  return (m_socket && m_socket->state() == QAbstractSocket::ConnectedState);
}

void RemoteClient::onConnected() {
  emit connected();
}

void RemoteClient::onDisconnected() {
  m_receiveBuffer.clear();
  emit disconnected();
}

void RemoteClient::onErrorOccurred(QAbstractSocket::SocketError socketError) {
  Q_UNUSED(socketError);
  emit connectionError(m_socket->errorString());
}

void RemoteClient::sendMouseMove(quint32 x, quint32 y) {
  RemoteMousePayload mouse;
  mouse.eventType = REMOTE_MOUSE_MOVE;
  mouse.button = 0;
  mouse.x = x;
  mouse.y = y;
  mouse.delta = 0;

  QByteArray payload(reinterpret_cast<const char*>(&mouse), sizeof(mouse));
  sendPacket(REMOTE_PKT_MOUSE, payload);
}

void RemoteClient::sendMouseButton(RemoteMouseEventType eventType, Qt::MouseButton button, quint32 x, quint32 y) {
  RemoteMousePayload mouse;
  mouse.eventType = eventType;
  mouse.button = static_cast<quint8>(button);
  mouse.x = x;
  mouse.y = y;
  mouse.delta = 0;

  QByteArray payload(reinterpret_cast<const char*>(&mouse), sizeof(mouse));
  sendPacket(REMOTE_PKT_MOUSE, payload);
}

void RemoteClient::sendMouseWheel(qint32 delta, quint32 x, quint32 y) {
  RemoteMousePayload mouse;
  mouse.eventType = REMOTE_MOUSE_WHEEL;
  mouse.button = 0;
  mouse.x = x;
  mouse.y = y;
  mouse.delta = delta;

  QByteArray payload(reinterpret_cast<const char*>(&mouse), sizeof(mouse));
  sendPacket(REMOTE_PKT_MOUSE, payload);
}

void RemoteClient::sendKeyEvent(RemoteKeyEventType eventType, quint32 key, Qt::KeyboardModifiers modifiers) {
  RemoteKeyPayload keyPayload;
  keyPayload.eventType = eventType;
  keyPayload.key = key;
  keyPayload.modifiers = static_cast<quint32>(modifiers);

  QByteArray payload(reinterpret_cast<const char*>(&keyPayload), sizeof(keyPayload));
  sendPacket(REMOTE_PKT_KEY, payload);
}

void RemoteClient::sendPacket(quint8 type, const QByteArray& payload) {
  if(!isConnected())
    return;

  RemotePacketHeader header;
  header.magic = REMOTE_MAGIC;
  header.type = type;
  header.payloadSize = payload.size();

  m_socket->write(reinterpret_cast<const char*>(&header), sizeof(header));
  if(!payload.isEmpty()) {
    m_socket->write(payload);
  }
}

void RemoteClient::onReadyRead() {
  m_receiveBuffer.append(m_socket->readAll());

  while(m_receiveBuffer.size() >= (int)sizeof(RemotePacketHeader)) {
    const RemotePacketHeader* pHeader = reinterpret_cast<const RemotePacketHeader*>(m_receiveBuffer.constData());
    if(pHeader->magic != REMOTE_MAGIC) {
      // Find magic
      int idx = m_receiveBuffer.indexOf("\x54\x4D\x45\x52"); // little endian "REMT"
      if(idx > 0) {
        m_receiveBuffer.remove(0, idx);
      } else {
        m_receiveBuffer.remove(0, 1);
      }
      continue;
    }

    int totalSize = sizeof(RemotePacketHeader) + pHeader->payloadSize;
    if(m_receiveBuffer.size() < totalSize) {
      // Waiting for remaining bytes
      break;
    }

    quint8 type = pHeader->type;
    QByteArray payload = m_receiveBuffer.mid(sizeof(RemotePacketHeader), pHeader->payloadSize);
    m_receiveBuffer.remove(0, totalSize);

    processPacket(type, payload);
  }
}

void RemoteClient::processPacket(quint8 type, const QByteArray& payload) {
  switch(type) {
  case REMOTE_PKT_INIT:
    if(payload.size() >= (int)sizeof(RemoteInitPayload)) {
      const RemoteInitPayload* init = reinterpret_cast<const RemoteInitPayload*>(payload.constData());
      emit initInfoReceived(init->screenWidth, init->screenHeight);
    }
    break;

  case REMOTE_PKT_FRAME:
    if(payload.size() >= (int)sizeof(RemoteFrameHeader)) {
      const RemoteFrameHeader* fHeader = reinterpret_cast<const RemoteFrameHeader*>(payload.constData());
      const char* imgBytes = payload.constData() + sizeof(RemoteFrameHeader);
      int imgSize = fHeader->imageSize;

      if(payload.size() >= (int)(sizeof(RemoteFrameHeader) + imgSize)) {
        QPixmap pixmap;
        if(pixmap.loadFromData(reinterpret_cast<const uchar*>(imgBytes), imgSize, "JPG")) {
          emit frameReceived(pixmap);
        }
      }
    }
    break;

  case REMOTE_PKT_CLOSE:
    disconnectFromHost();
    break;

  default:
    break;
  }
}
