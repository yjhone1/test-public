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

#ifndef REMOTEDEFINITIONS_H
#define REMOTEDEFINITIONS_H

#include <QtGlobal>

#define REMOTE_MAGIC 0x52454D54 // "REMT" in ASCII
#define DEFAULT_REMOTE_PORT 50005

enum RemotePacketType : quint8 {
  REMOTE_PKT_INIT  = 1,
  REMOTE_PKT_FRAME = 2,
  REMOTE_PKT_MOUSE = 3,
  REMOTE_PKT_KEY   = 4,
  REMOTE_PKT_CLOSE = 5
};

enum RemoteMouseEventType : quint8 {
  REMOTE_MOUSE_MOVE    = 1,
  REMOTE_MOUSE_PRESS   = 2,
  REMOTE_MOUSE_RELEASE = 3,
  REMOTE_MOUSE_WHEEL   = 4
};

enum RemoteKeyEventType : quint8 {
  REMOTE_KEY_PRESS   = 1,
  REMOTE_KEY_RELEASE = 2
};

#pragma pack(push, 1)
struct RemotePacketHeader {
  quint32 magic;
  quint8 type;
  quint32 payloadSize;
};

struct RemoteInitPayload {
  quint32 screenWidth;
  quint32 screenHeight;
};

struct RemoteFrameHeader {
  quint32 width;
  quint32 height;
  quint32 imageSize;
};

struct RemoteMousePayload {
  quint8 eventType; // RemoteMouseEventType
  quint8 button;    // Qt::MouseButton (1=Left, 2=Right, 4=Middle)
  quint32 x;        // Coordinate on host screen
  quint32 y;        // Coordinate on host screen
  qint32 delta;     // Wheel delta
};

struct RemoteKeyPayload {
  quint8 eventType; // RemoteKeyEventType
  quint32 key;      // Qt::Key
  quint32 modifiers;// Qt::KeyboardModifiers
};
#pragma pack(pop)

#endif // REMOTEDEFINITIONS_H
