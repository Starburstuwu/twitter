#ifndef OPENVPNPROTOCOL_H
#define OPENVPNPROTOCOL_H

#include <QObject>
#include <QString>
#include <QTimer>

#include "vpnprotocol.h"

#include "core/ipcclient.h"

class OpenVpnProtocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit OpenVpnProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~OpenVpnProtocol() override;

    ErrorCode start() override;
    void stop() override;

    ErrorCode prepare() override;
    static QString defaultConfigFileName();
    static QString defaultConfigPath();


private:
    QString configPath() const;
    bool openVpnProcessIsRunning() const;
    bool sendTermSignal();
    void readOpenVpnConfiguration(const QJsonObject &configuration);
    void handle_cli_message(QString message);
    void killOpenVpnProcess();
    void sendByteCount();
    void sendInitialData();

    QHash<QRemoteObjectPendingCallWatcher*, QTimer*> m_watchers;
    QString m_configFileName;
    QTemporaryFile m_configFile;


private:
    void updateRouteGateway(QString line);
    void updateVpnGateway(const QString &line);

    QSharedPointer<PrivilegedProcess> m_openVpnProcess;
};

#endif // OPENVPNPROTOCOL_H
