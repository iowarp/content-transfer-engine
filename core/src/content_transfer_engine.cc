#include <chimaera/chimaera.h>
#include <wrp_cte/core/content_transfer_engine.h>
#include <wrp_cte/core/core_client.h>
#include <wrp_cte/core/core_config.h>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

// Define global pointer variable in source file (outside namespace)
HSHM_DEFINE_GLOBAL_PTR_VAR_CC(wrp_cte::core::ContentTransferEngine,
                              g_cte_manager);

namespace wrp_cte::core {

bool ContentTransferEngine::ClientInit(const std::string &config_path,
                                       const chi::PoolQuery &pool_query) {
  // Check for race conditions - if already initialized or initializing
  if (is_initialized_) {
    return true;
  }
  if (is_initializing_) {
    return true;
  }
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

  // Determine config file path - prioritize parameter, then environment variables
  CreateParams params;
  auto *ipc_manager = CHI_IPC;
  auto *main_allocator = ipc_manager->GetMainAllocator();

  std::string effective_config_path = config_path;

  // If config_path is empty, check environment variables
  if (effective_config_path.empty()) {
    // Prioritize WRP_RUNTIME_CONF
    const char *wrp_conf = std::getenv("WRP_CTE_CONF");
    effective_config_path = wrp_conf;
  }

  // Create CreateParams with config file contents if path is available
  if (!effective_config_path.empty()) {
    // Read the YAML file content into a string
    std::ifstream config_file(effective_config_path);
    if (config_file.is_open()) {
      std::stringstream buffer;
      buffer << config_file.rdbuf();
      std::string config_content = buffer.str();
      config_file.close();

      // Create params with the YAML content string
      params = CreateParams(main_allocator);
      params.config_yaml_string_ = config_content;
    } else {
      // If file cannot be opened, create params without config
      params = CreateParams(main_allocator);
    }
  } else {
    params = CreateParams(main_allocator);
  }

  HILOG(kInfo, "Creating CTE Core container from {}: {}",
        effective_config_path, params.config_yaml_string_.str());

  // Create CTE Core container using constants from core_tasks.h and specified pool_query
  cte_client->Create(hipc::MemContext(), pool_query,
                     wrp_cte::core::kCtePoolName, wrp_cte::core::kCtePoolId, params);

  // Suppress unused variable warnings
  (void)cte_client;
  (void)cte_config;

  // Mark as successfully initialized
  is_initialized_ = true;
  is_initializing_ = false;

  return true;
}

} // namespace wrp_cte::core