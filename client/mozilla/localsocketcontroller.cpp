/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "protocols/protocols_defs.h"
#include "localsocketcontroller.h"

#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStandardPaths>

//#include "errorhandler.h"
#include "ipaddress.h"
#include "leakdetector.h"
#include "logger.h"
//#include "models/device.h"
//#include "models/keys.h"
#include "models/server.h"
//#include "settingsholder.h"

// How many times do we try to reconnect.
constexpr int MAX_CONNECTION_RETRY = 10;

// How long do we wait between one try and the next one.
constexpr int CONNECTION_RETRY_TIMER_MSEC = 500;

namespace {
Logger logger("LocalSocketController");
}

LocalSocketController::LocalSocketController() {
  MZ_COUNT_CTOR(LocalSocketController);

  m_socket = new QLocalSocket(this);
  connect(m_socket, &QLocalSocket::connected, this,
          &LocalSocketController::daemonConnected);
  connect(m_socket, &QLocalSocket::disconnected, this,
          &LocalSocketController::disconnected);
  connect(m_socket, &QLocalSocket::errorOccurred, this,
          &LocalSocketController::errorOccurred);
  connect(m_socket, &QLocalSocket::readyRead, this,
          &LocalSocketController::readData);

  m_initializingTimer.setSingleShot(true);
  connect(&m_initializingTimer, &QTimer::timeout, this,
          &LocalSocketController::initializeInternal);
}

LocalSocketController::~LocalSocketController() {
  MZ_COUNT_DTOR(LocalSocketController);
}

void LocalSocketController::errorOccurred(
    QLocalSocket::LocalSocketError error) {
  logger.error() << "Error occurred:" << error;

  if (m_daemonState == eInitializing) {
    if (m_initializingRetry++ < MAX_CONNECTION_RETRY) {
      m_initializingTimer.start(CONNECTION_RETRY_TIMER_MSEC);
      return;
    }

    emit initialized(false, false, QDateTime());
  }

  qCritical() << "ControllerError";
  disconnectInternal();
}

void LocalSocketController::disconnectInternal() {
  // We're still eReady as the Deamon is alive
  // and can make a new connection.
  m_daemonState = eReady;
  m_initializingRetry = 0;
  m_initializingTimer.stop();
  emit disconnected();
}

void LocalSocketController::initialize(const Device* device, const Keys* keys) {
  logger.debug() << "Initializing";

  Q_UNUSED(device);
  Q_UNUSED(keys);

  Q_ASSERT(m_daemonState == eUnknown);
  m_initializingRetry = 0;

  initializeInternal();
}

void LocalSocketController::initializeInternal() {
  m_daemonState = eInitializing;

#ifdef MZ_WINDOWS
  QString path = "\\\\.\\pipe\\amneziavpn";
#else
  QString path = "/var/run/amneziavpn/daemon.socket";
  if (!QFileInfo::exists(path)) {
    path = "/tmp/amneziavpn.socket";
  }
#endif

  logger.debug() << "Connecting to:" << path;
  m_socket->connectToServer(path);
}

void LocalSocketController::daemonConnected() {
  logger.debug() << "Daemon connected";
  Q_ASSERT(m_daemonState == eInitializing);
  checkStatus();
}

void LocalSocketController::activate(const QJsonObject &rawConfig) {
  QJsonObject wgConfig = rawConfig.value("wireguard_config_data").toObject();

  QJsonObject json;
  json.insert("type", "activate");
  //  json.insert("hopindex", QJsonValue((double)hop.m_hopindex));
  json.insert("privateKey", wgConfig.value(amnezia::config_key::client_priv_key));
  json.insert("deviceIpv4Address", wgConfig.value(amnezia::config_key::client_ip));
  json.insert("deviceIpv6Address", "dead::1");
  json.insert("serverPublicKey", wgConfig.value(amnezia::config_key::server_pub_key));
  json.insert("serverPskKey", wgConfig.value(amnezia::config_key::psk_key));
  json.insert("serverIpv4AddrIn", wgConfig.value(amnezia::config_key::hostName));
  //  json.insert("serverIpv6AddrIn", QJsonValue(hop.m_server.ipv6AddrIn()));
  json.insert("serverPort", wgConfig.value(amnezia::config_key::port).toInt());

  json.insert("serverIpv4Gateway", wgConfig.value(amnezia::config_key::hostName));
  //  json.insert("serverIpv6Gateway", QJsonValue(hop.m_server.ipv6Gateway()));
  json.insert("dnsServer", rawConfig.value(amnezia::config_key::dns1));

  QJsonArray jsAllowedIPAddesses;

  QJsonObject range_ipv4;
  range_ipv4.insert("address", "0.0.0.0");
  range_ipv4.insert("range", 0);
  range_ipv4.insert("isIpv6", false);
  jsAllowedIPAddesses.append(range_ipv4);

  QJsonObject range_ipv6;
  range_ipv6.insert("address", "::");
  range_ipv6.insert("range", 0);
  range_ipv6.insert("isIpv6", true);
  jsAllowedIPAddesses.append(range_ipv6);

  json.insert("allowedIPAddressRanges", jsAllowedIPAddesses);


  QJsonArray jsExcludedAddresses;
  jsExcludedAddresses.append(wgConfig.value(amnezia::config_key::hostName));
  json.insert("excludedAddresses", jsExcludedAddresses);


  //  QJsonArray splitTunnelApps;
  //  for (const auto& uri : hop.m_vpnDisabledApps) {
  //    splitTunnelApps.append(QJsonValue(uri));
  //  }
  //  json.insert("vpnDisabledApps", splitTunnelApps);
  write(json);
}

void LocalSocketController::deactivate() {
  logger.debug() << "Deactivating";

  if (m_daemonState != eReady) {
    logger.debug() << "No disconnect, controller is not ready";
    emit disconnected();
    return;
  }

  QJsonObject json;
  json.insert("type", "deactivate");
  write(json);
  emit disconnected();
}

void LocalSocketController::checkStatus() {
  logger.debug() << "Check status";

  if (m_daemonState == eReady || m_daemonState == eInitializing) {
    Q_ASSERT(m_socket);

    QJsonObject json;
    json.insert("type", "status");
    write(json);
  }
}

void LocalSocketController::getBackendLogs(
    std::function<void(const QString&)>&& a_callback) {
  logger.debug() << "Backend logs";

  if (m_logCallback) {
    m_logCallback("");
    m_logCallback = nullptr;
  }

  if (m_daemonState != eReady) {
    std::function<void(const QString&)> callback = a_callback;
    callback("");
    return;
  }

  m_logCallback = std::move(a_callback);

  QJsonObject json;
  json.insert("type", "logs");
  write(json);
}

void LocalSocketController::cleanupBackendLogs() {
  logger.debug() << "Cleanup logs";

  if (m_logCallback) {
    m_logCallback("");
    m_logCallback = nullptr;
  }

  if (m_daemonState != eReady) {
    return;
  }

  QJsonObject json;
  json.insert("type", "cleanlogs");
  write(json);
}

void LocalSocketController::readData() {
  logger.debug() << "Reading";

  Q_ASSERT(m_socket);
  Q_ASSERT(m_daemonState == eInitializing || m_daemonState == eReady);
  QByteArray input = m_socket->readAll();
  m_buffer.append(input);

  while (true) {
    int pos = m_buffer.indexOf("\n");
    if (pos == -1) {
      break;
    }

    QByteArray line = m_buffer.left(pos);
    m_buffer.remove(0, pos + 1);

    QByteArray command(line);
    command = command.trimmed();

    if (command.isEmpty()) {
      continue;
    }

    parseCommand(command);
  }
}

void LocalSocketController::parseCommand(const QByteArray& command) {
  QJsonDocument json = QJsonDocument::fromJson(command);
  if (!json.isObject()) {
    logger.error() << "Invalid JSON - object expected";
    return;
  }

  QJsonObject obj = json.object();
  QJsonValue typeValue = obj.value("type");
  if (!typeValue.isString()) {
    logger.error() << "Invalid JSON - no type";
    return;
  }
  QString type = typeValue.toString();

  logger.debug() << "Parse command:" << type;

  if (m_daemonState == eInitializing && type == "status") {
    m_daemonState = eReady;

    QJsonValue connected = obj.value("connected");
    if (!connected.isBool()) {
      logger.error() << "Invalid JSON for status - connected expected";
      return;
    }

    QDateTime datetime;
    if (connected.toBool()) {
      QJsonValue date = obj.value("date");
      if (!date.isString()) {
        logger.error() << "Invalid JSON for status - date expected";
        return;
      }

      datetime = QDateTime::fromString(date.toString());
      if (!datetime.isValid()) {
        logger.error() << "Invalid JSON for status - date is invalid";
        return;
      }
    }

    emit initialized(true, connected.toBool(), datetime);
    return;
  }

  if (m_daemonState != eReady) {
    logger.error() << "Unexpected command";
    return;
  }

  if (type == "status") {
    QJsonValue serverIpv4Gateway = obj.value("serverIpv4Gateway");
    if (!serverIpv4Gateway.isString()) {
      logger.error() << "Unexpected serverIpv4Gateway value";
      return;
    }

    QJsonValue deviceIpv4Address = obj.value("deviceIpv4Address");
    if (!deviceIpv4Address.isString()) {
      logger.error() << "Unexpected deviceIpv4Address value";
      return;
    }

    QJsonValue txBytes = obj.value("txBytes");
    if (!txBytes.isDouble()) {
      logger.error() << "Unexpected txBytes value";
      return;
    }

    QJsonValue rxBytes = obj.value("rxBytes");
    if (!rxBytes.isDouble()) {
      logger.error() << "Unexpected rxBytes value";
      return;
    }

    emit statusUpdated(serverIpv4Gateway.toString(),
                       deviceIpv4Address.toString(), txBytes.toDouble(),
                       rxBytes.toDouble());
    return;
  }

  if (type == "disconnected") {
    disconnectInternal();
    return;
  }

  if (type == "connected") {
    QJsonValue pubkey = obj.value("pubkey");
    if (!pubkey.isString()) {
      logger.error() << "Unexpected pubkey value";
      return;
    }

    logger.debug() << "Handshake completed with:"
                   << pubkey.toString();
    emit connected(pubkey.toString());
    return;
  }

  if (type == "backendFailure") {
    qCritical() << "backendFailure";
    return;
  }

  if (type == "logs") {
    // We don't care if we are not waiting for logs.
    if (!m_logCallback) {
      return;
    }

    QJsonValue logs = obj.value("logs");
    m_logCallback(logs.isString() ? logs.toString().replace("|", "\n")
                                  : QString());
    m_logCallback = nullptr;
    return;
  }

  logger.warning() << "Invalid command received:" << command;
}

void LocalSocketController::write(const QJsonObject& json) {
  Q_ASSERT(m_socket);
  m_socket->write(QJsonDocument(json).toJson(QJsonDocument::Compact));
  m_socket->write("\n");
  m_socket->flush();
}
