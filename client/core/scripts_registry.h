#ifndef SCRIPTS_REGISTRY_H
#define SCRIPTS_REGISTRY_H

#include <QLatin1String>
#include "core/defs.h"
#include "containers/containers_defs.h"

namespace amnezia {

enum SharedScriptType {
    // General scripts
    prepare_host,
    install_docker,
    build_container,
    remove_container,
    remove_all_containers,
    setup_host_firewall,
    check_connection,
    check_server_is_busy
};
enum ProtocolScriptType {
    // Protocol scripts
    dockerfile,
    run_container,
    configure_container,
    container_startup,
    openvpn_template,
    wireguard_template,
    v2ray_client_template,
    shadowsocks_client_template
};


QString scriptFolder(DockerContainer container);

QString scriptName(SharedScriptType type);
QString scriptName(ProtocolScriptType type);

QString scriptData(SharedScriptType type);
QString scriptData(ProtocolScriptType type, DockerContainer container);
}

#endif // SCRIPTS_REGISTRY_H
