#include "hermes_shm/util/logging.h"
#include <chimaera/chimaera.h>
#include <chimaera/core/content_transfer_engine.h>
#include <chimaera/core/core_client.h>
#include <chimaera/core/core_config.h>

namespace wrp_cte::core {

// Define global pointer variables in source file
HSHM_DEFINE_GLOBAL_PTR_VAR_CC(wrp_cte::core::Client, g_cte_client);
HSHM_DEFINE_GLOBAL_PTR_VAR_CC(wrp_cte::core::Config, g_cte_config);

bool WRP_CTE_CLIENT_INIT(const std::string &config_path) {
  (void)config_path;
  auto *cte_manager = CTE_MANAGER;
  return cte_manager->ClientInit();
}

} // namespace wrp_cte::core