/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Distributed under BSD 3-Clause license.                                   *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Illinois Institute of Technology.                        *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of Hermes. The full Hermes copyright notice, including  *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the top directory. If you do not  *
 * have access to the file, you may request a copy from help@hdfgroup.org.   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef HRUN_TASKS_HERMES_INCLUDE_hermes_H_
#define HRUN_TASKS_HERMES_INCLUDE_hermes_H_

#include "chimaera_admin/chimaera_admin_client.h"
#include "hermes/config_client.h"
#include "hermes/config_server.h"
#include "hermes/hermes_types.h"
#include "hermes_core/hermes_core_client.h"

namespace hermes {

class ConfigurationManager {
public:
  hermes::Client mdm_;
  ServerConfig server_config_;
  ClientConfig client_config_;
  bool is_initialized_ = false;

public:
  ConfigurationManager() = default;

  void ClientInit() {
    if (is_initialized_) {
      return;
    }
    // Create connection to MDM
    std::string config_path = "";
    LoadClientConfig(config_path);
    LoadServerConfig(config_path);
    mdm_.Create(
        HSHM_MCTX,
        chi::DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
        chi::DomainQuery::GetGlobalBcast(), "hermes_core");
    is_initialized_ = true;
  }

  void ServerInit() { ClientInit(); }

  void LoadClientConfig(std::string config_path) {
    // Load hermes config
    if (config_path.empty()) {
      config_path = GetEnvSafe(Constant::kHermesClientConf);
    }
    HILOG(kInfo, "Loading client configuration: {}", config_path);
    client_config_.LoadFromFile(config_path);
  }

  void LoadServerConfig(std::string config_path) {
    // Load hermes config
    if (config_path.empty()) {
      config_path = GetEnvSafe(Constant::kHermesServerConf);
    }
    HILOG(kInfo, "Loading server configuration: {}", config_path);
    server_config_.LoadFromFile(config_path);
  }
};

HSHM_DEFINE_GLOBAL_PTR_VAR_H((hermes::ConfigurationManager), hermes_conf)
#define HERMES_CONF                                                            \
  HSHM_GET_GLOBAL_PTR_VAR((hermes::ConfigurationManager), hermes::hermes_conf)
#define HERMES_CLIENT_CONF HERMES_CONF->client_config_
#define HERMES_SERVER_CONF HERMES_CONF->server_config_

/** Initialize client-side Hermes transparently */
static inline bool TRANSPARENT_HERMES() {
  if (CHIMAERA_CLIENT_INIT()) {
    HERMES_CONF->ClientInit();
    return true;
  }
  return false;
}

/** Wrapper for client-side hermes init */
static inline bool HERMES_INIT() { return TRANSPARENT_HERMES(); }

} // namespace hermes

using hermes::HERMES_INIT;
using hermes::TRANSPARENT_HERMES;

#endif // HRUN_TASKS_HERMES_INCLUDE_hermes_H_
