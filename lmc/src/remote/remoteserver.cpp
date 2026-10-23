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

#include "remoteserver.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QBuffer>
#include <QCursor>
#include <QDebug>

#ifdef Q_OS_LINUX
typedef void* (*pfn_XOpenDisplay)(const char*);
typedef int (*pfn_XCloseDisplay)(void*);
typedef int (*pfn_XFlush)(void*);
typedef int (*pfn_XTestFakeMotionEvent)(void*, int, int, int, unsigned long);
typedef int (*pfn_XTestFakeButtonEvent)(void*, unsigned int, int, unsigned long);
typedef int (*pfn_XTestFakeKeyEvent)(void*, unsigned int, int, unsigned long);
typedef unsigned int (*pfn_XKeysymToKeycode)(void*, unsigned long);

static pfn_XOpenDisplay s_XOpenDisplay = nullptr;
static pfn_XCloseDisplay s_XCloseDisplay = nullptr;
static pfn_XFlush s_XFlush = nullptr;
static pfn_XTestFakeMotionEvent s_XTestFakeMotionEvent = nullptr;
static pfn_XTestFakeButtonEvent s_XTestFakeButtonEvent = nullptr;
static pfn_XTestFakeKeyEvent s_XTestFakeKeyEvent = nullptr;
static pfn_XKeysymToKeycode s_XKeysymToKeycode = nullptr;
#endif

RemoteServer::RemoteServer(QObject* parent)
  : QObject(parent),
    m_server(new QTcpServer(this)),
    m_clientSocket(nullptr),
    m_frameTimer(new QTimer(this)),
    m_jpegQuality(65),
    m_frameIntervalMs(66) // ~15 FPS
#ifdef Q_OS_LINUX
    , m_display(nullptr),
    m_x11Available(false)
#endif
{
  connect(m_server, &QTcpServer::newConnection, this, &RemoteServer::onNewConnection);
  connect(m_frameTimer, &QTimer::timeout, this, &RemoteServer::captureAndSendFrame);

  initInputSimulator();
}

RemoteServer::~RemoteServer() {
  stopServer();
  cleanupInputSimulator();
}

void RemoteServer::initInputSimulator() {
#ifdef Q_OS_LINUX
  m_x11Lib.setFileName("X11");
  m_xtstLib.setFileName("Xtst");

  if(m_x11Lib.load() && m_xtstLib.load()) {
    s_XOpenDisplay = (pfn_XOpenDisplay)m_x11Lib.resolve("XOpenDisplay");
    s_XCloseDisplay = (pfn_XCloseDisplay)m_x11Lib.resolve("XCloseDisplay");
    s_XFlush = (pfn_XFlush)m_x11Lib.resolve("XFlush");
    s_XKeysymToKeycode = (pfn_XKeysymToKeycode)m_x11Lib.resolve("XKeysymToKeycode");

    s_XTestFakeMotionEvent = (pfn_XTestFakeMotionEvent)m_xtstLib.resolve("XTestFakeMotionEvent");
    s_XTestFakeButtonEvent = (pfn_XTestFakeButtonEvent)m_xtstLib.resolve("XTestFakeButtonEvent");
    s_XTestFakeKeyEvent = (pfn_XTestFakeKeyEvent)m_xtstLib.resolve("XTestFakeKeyEvent");

    if(s_XOpenDisplay && s_XCloseDisplay && s_XFlush && s_XTestFakeMotionEvent && s_XTestFakeButtonEvent) {
      m_display = s_XOpenDisplay(nullptr);
      if(m_display) {
        m_x11Available = true;
      }
    }
  }
#endif
}

void RemoteServer::cleanupInputSimulator() {
#ifdef Q_OS_LINUX
  if(m_display && s_XCloseDisplay) {
    s_XCloseDisplay(m_display);
    m_display = nullptr;
  }
  m_x11Available = false;
#endif
}

bool RemoteServer::startServer(quint16 port) {
  stopServer();
  if(!m_server->listen(QHostAddress::Any, port)) {
    qWarning() << "RemoteServer: Failed to listen on port" << port << m_server->errorString();
    return false;
  }
  return true;
}

void RemoteServer::stopServer() {
  m_frameTimer->stop();
  if(m_clientSocket) {
    // Send close packet
    sendPacket(REMOTE_PKT_CLOSE, QByteArray());
    m_clientSocket->disconnectFromHost();
    m_clientSocket = nullptr;
  }
  if(m_server->isListening()) {
    m_server->close();
    emit serverStopped();
  }
}

quint16 RemoteServer::serverPort() const {
  return m_server->serverPort();
}

bool RemoteServer::isClientConnected() const {
  return (m_clientSocket != nullptr && m_clientSocket->state() == QAbstractSocket::ConnectedState);
}

void RemoteServer::onNewConnection() {
  QTcpSocket* newSocket = m_server->nextPendingConnection();
  if(!newSocket)
    return;

  if(m_clientSocket) {
    // Only allow one viewer at a time
    newSocket->disconnectFromHost();
    newSocket->deleteLater();
    return;
  }

  m_clientSocket = newSocket;
  m_receiveBuffer.clear();

  connect(m_clientSocket, &QTcpSocket::disconnected, this, &RemoteServer::onClientDisconnected);
  connect(m_clientSocket, &QTcpSocket::readyRead, this, &RemoteServer::onReadyRead);

  // Send screen init info
  QScreen* screen = QGuiApplication::primaryScreen();
  if(screen) {
    QSize screenSize = screen->size();
    RemoteInitPayload initPayload;
    initPayload.screenWidth = screenSize.width();
    initPayload.screenHeight = screenSize.height();

    QByteArray payloadData(reinterpret_cast<const char*>(&initPayload), sizeof(initPayload));
    sendPacket(REMOTE_PKT_INIT, payloadData);
  }

  // Start frame streaming timer
  m_frameTimer->start(m_frameIntervalMs);

  emit clientConnected(m_clientSocket->peerAddress().toString());
}

void RemoteServer::onClientDisconnected() {
  m_frameTimer->stop();
  m_receiveBuffer.clear();
  if(m_clientSocket) {
    m_clientSocket->deleteLater();
    m_clientSocket = nullptr;
  }
  emit clientDisconnected();
}

void RemoteServer::captureAndSendFrame() {
  if(!isClientConnected())
    return;

  // Avoid piling up write buffers if network is slow
  if(m_clientSocket->bytesToWrite() > 1024 * 512)
    return;

  QScreen* screen = QGuiApplication::primaryScreen();
  if(!screen)
    return;

  QPixmap pixmap = screen->grabWindow(0);
  if(pixmap.isNull())
    return;

  QByteArray jpegData;
  QBuffer buffer(&jpegData);
  buffer.open(QIODevice::WriteOnly);
  pixmap.save(&buffer, "JPG", m_jpegQuality);

  RemoteFrameHeader frameHeader;
  frameHeader.width = pixmap.width();
  frameHeader.height = pixmap.height();
  frameHeader.imageSize = jpegData.size();

  QByteArray payload;
  payload.resize(sizeof(RemoteFrameHeader) + jpegData.size());
  memcpy(payload.data(), &frameHeader, sizeof(RemoteFrameHeader));
  memcpy(payload.data() + sizeof(RemoteFrameHeader), jpegData.constData(), jpegData.size());

  sendPacket(REMOTE_PKT_FRAME, payload);
}

void RemoteServer::sendPacket(quint8 type, const QByteArray& payload) {
  if(!m_clientSocket || m_clientSocket->state() != QAbstractSocket::ConnectedState)
    return;

  RemotePacketHeader header;
  header.magic = REMOTE_MAGIC;
  header.type = type;
  header.payloadSize = payload.size();

  m_clientSocket->write(reinterpret_cast<const char*>(&header), sizeof(header));
  if(!payload.isEmpty()) {
    m_clientSocket->write(payload);
  }
}

void RemoteServer::onReadyRead() {
  if(!m_clientSocket)
    return;

  m_receiveBuffer.append(m_clientSocket->readAll());

  while(m_receiveBuffer.size() >= (int)sizeof(RemotePacketHeader)) {
    const RemotePacketHeader* pHeader = reinterpret_cast<const RemotePacketHeader*>(m_receiveBuffer.constData());
    if(pHeader->magic != REMOTE_MAGIC) {
      // Out of sync; drop one byte and continue
      m_receiveBuffer.remove(0, 1);
      continue;
    }

    int totalSize = sizeof(RemotePacketHeader) + pHeader->payloadSize;
    if(m_receiveBuffer.size() < totalSize) {
      // Need more data
      break;
    }

    quint8 type = pHeader->type;
    QByteArray payload = m_receiveBuffer.mid(sizeof(RemotePacketHeader), pHeader->payloadSize);
    m_receiveBuffer.remove(0, totalSize);

    processPacket(type, payload);
  }
}

void RemoteServer::processPacket(quint8 type, const QByteArray& payload) {
  switch(type) {
  case REMOTE_PKT_MOUSE:
    if(payload.size() >= (int)sizeof(RemoteMousePayload)) {
      const RemoteMousePayload* mouse = reinterpret_cast<const RemoteMousePayload*>(payload.constData());
      handleMousePacket(*mouse);
    }
    break;

  case REMOTE_PKT_KEY:
    if(payload.size() >= (int)sizeof(RemoteKeyPayload)) {
      const RemoteKeyPayload* key = reinterpret_cast<const RemoteKeyPayload*>(payload.constData());
      handleKeyPacket(*key);
    }
    break;

  case REMOTE_PKT_CLOSE:
    if(m_clientSocket) {
      m_clientSocket->disconnectFromHost();
    }
    break;

  default:
    break;
  }
}

void RemoteServer::handleMousePacket(const RemoteMousePayload& mouse) {
#ifdef Q_OS_LINUX
  if(m_x11Available && m_display) {
    if(mouse.eventType == REMOTE_MOUSE_MOVE) {
      s_XTestFakeMotionEvent(m_display, -1, mouse.x, mouse.y, 0);
      s_XFlush(m_display);
    } else if(mouse.eventType == REMOTE_MOUSE_PRESS || mouse.eventType == REMOTE_MOUSE_RELEASE) {
      unsigned int button = 1;
      if(mouse.button == 2) // RightButton
        button = 3;
      else if(mouse.button == 4) // MiddleButton
        button = 2;

      s_XTestFakeButtonEvent(m_display, button, (mouse.eventType == REMOTE_MOUSE_PRESS), 0);
      s_XFlush(m_display);
    } else if(mouse.eventType == REMOTE_MOUSE_WHEEL) {
      unsigned int wheelButton = (mouse.delta > 0) ? 4 : 5;
      s_XTestFakeButtonEvent(m_display, wheelButton, 1, 0);
      s_XTestFakeButtonEvent(m_display, wheelButton, 0, 0);
      s_XFlush(m_display);
    }
    return;
  }
#endif

  // Fallback: cursor move only
  if(mouse.eventType == REMOTE_MOUSE_MOVE) {
    QCursor::setPos(mouse.x, mouse.y);
  }
}

void RemoteServer::handleKeyPacket(const RemoteKeyPayload& key) {
#ifdef Q_OS_LINUX
  if(m_x11Available && m_display && s_XTestFakeKeyEvent && s_XKeysymToKeycode) {
    unsigned long keysym = 0;
    // Map common Qt keys to X11 keysym
    if(key.key >= Qt::Key_A && key.key <= Qt::Key_Z) {
      keysym = (key.modifiers & Qt::ShiftModifier) ? (key.key) : (key.key + 0x20);
    } else if(key.key >= Qt::Key_0 && key.key <= Qt::Key_9) {
      keysym = key.key;
    } else if(key.key == Qt::Key_Return || key.key == Qt::Key_Enter) {
      keysym = 0xFF0D; // XK_Return
    } else if(key.key == Qt::Key_Backspace) {
      keysym = 0xFF08; // XK_BackSpace
    } else if(key.key == Qt::Key_Tab) {
      keysym = 0xFF09; // XK_Tab
    } else if(key.key == Qt::Key_Escape) {
      keysym = 0xFF1B; // XK_Escape
    } else if(key.key == Qt::Key_Space) {
      keysym = 0x0020; // XK_space
    } else if(key.key == Qt::Key_Left) {
      keysym = 0xFF51;
    } else if(key.key == Qt::Key_Up) {
      keysym = 0xFF52;
    } else if(key.key == Qt::Key_Right) {
      keysym = 0xFF53;
    } else if(key.key == Qt::Key_Down) {
      keysym = 0xFF54;
    }

    if(keysym != 0) {
      unsigned int keycode = s_XKeysymToKeycode(m_display, keysym);
      if(keycode != 0) {
        s_XTestFakeKeyEvent(m_display, keycode, (key.eventType == REMOTE_KEY_PRESS), 0);
        s_XFlush(m_display);
      }
    }
  }
#endif
}
