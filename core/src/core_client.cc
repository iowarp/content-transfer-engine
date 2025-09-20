#include <chimaera/chimaera.h>
#include <chimaera/core/core_client.h>
#include <chimaera/core/core_config.h>

namespace wrp_cte::core {

// Define global pointer variables in source file
HSHM_DEFINE_GLOBAL_PTR_VAR_CC(wrp_cte::core::Client, g_cte_client);
HSHM_DEFINE_GLOBAL_PTR_VAR_CC(wrp_cte::core::Config, g_cte_config);

bool WRP_CTE_INIT(const std::string &config_path) {
  (void)config_path;

  // Track initialization state to prevent duplicate initialization
  static bool cte_initialized = false;
  static bool cte_initializing = false;
  
  // Return true if already initialized
  if (cte_initialized) {
    return true;
  }
  
  // Return true if currently initializing (prevents recursive calls)
  if (cte_initializing) {
    return true;
  }
  
  // Mark as initializing
  cte_initializing = true;

  // Initialize Chimaera client
  if (!chi::CHIMAERA_CLIENT_INIT()) {
    cte_initializing = false;
    return false;
  }

  // Global pointer variables are automatically created on first access
  // Use the macros to access them
  auto &client = WRP_CTE_CLIENT;
  auto &config = WRP_CTE_CONFIG;
  client.Create(hipc::MemContext(), chi::PoolQuery::Local());

  // Both client and config are now initialized with default constructors
  (void)client;
  (void)config;

  // Mark as fully initialized
  cte_initialized = true;
  cte_initializing = false;

  return true;
}

} // namespace wrp_cte::core