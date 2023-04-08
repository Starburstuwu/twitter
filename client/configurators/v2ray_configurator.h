#ifndef V2RAYCONFIGURATOR_H
#define V2RAYCONFIGURATOR_H

#include <QObject>

#include "configurator_base.h"

using namespace amnezia;

class V2RayConfigurator : ConfiguratorBase
{
public:
    V2RayConfigurator(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    QString genV2RayConfig(const ServerCredentials &credentials, DockerContainer container,
                           const QJsonObject &containerConfig, ErrorCode *errorCode = nullptr);
};

#endif // V2RAYCONFIGURATOR_H

