#include <chimaera/chimaera.h>
#include <wrp_cte/core/content_transfer_engine.h>
#include <wrp_cte/core/core_client.h>
#include <wrp_cte/core/core_config.h>

// Define global pointer variable in source file (outside namespace)
HSHM_DEFINE_GLOBAL_PTR_VAR_CC(wrp_cte::core::ContentTransferEngine,
                              g_cte_manager);

namespace wrp_cte::core {

bool ContentTransferEngine::ClientInit() {
  // Check for race conditions - if already initialized or initializing
  if (is_initialized_) {
    return true;
  }
  if (is_initializing_) {
    return true;
  }
  HILOG(kInfo, "HERE?");
  auto *chimaera_manager = CHI_CHIMAERA_MANAGER;
  if (chimaera_manager->IsInitializing()) {
    return true;
  }

  // Set initializing flag
  is_initializing_ = true;

  // Initialize Chimaera client
  if (!chi::CHIMAERA_CLIENT_INIT()) {
    is_initializing_ = false;
    return false;
  }

  // Initialize CTE core client and config
  auto *cte_client = WRP_CTE_CLIENT;
  auto *cte_config = WRP_CTE_CONFIG;
  cte_client->Create(hipc::MemContext(), chi::PoolQuery::Local());

  // Suppress unused variable warnings
  (void)cte_client;
  (void)cte_config;

  // Mark as successfully initialized
  is_initialized_ = true;
  is_initializing_ = false;

  return true;
}

} // namespace wrp_cte::core