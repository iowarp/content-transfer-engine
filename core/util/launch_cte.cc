// Copyright 2024 IOWarp
// Utility to launch the Content Transfer Engine (CTE)

#include <wrp_cte/core/core_client.h>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  try {
    // Parse command-line arguments for pool query type
    // Default to "dynamic" if no argument provided
    std::string pool_query_type = "dynamic";

    if (argc > 1) {
      pool_query_type = argv[1];
      // Convert to lowercase for case-insensitive comparison
      for (char &c : pool_query_type) {
        c = std::tolower(c);
      }
    }

    // Determine appropriate pool query based on argument
    chi::PoolQuery pool_query;
    if (pool_query_type == "local") {
      pool_query = chi::PoolQuery::Local();
      std::cout << "Using Local pool query for CTE initialization" << std::endl;
    } else if (pool_query_type == "dynamic") {
      pool_query = chi::PoolQuery::Dynamic();
      std::cout << "Using Dynamic pool query for CTE initialization" << std::endl;
    } else {
      std::cerr << "Invalid pool query type: " << argv[1] << std::endl;
      std::cerr << "Usage: " << argv[0] << " [local|dynamic]" << std::endl;
      return 1;
    }

    // Initialize the CTE client with the specified pool query
    wrp_cte::core::WRP_CTE_CLIENT_INIT("", pool_query);

    std::cout << "Content Transfer Engine initialized successfully" << std::endl;
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error initializing Content Transfer Engine: " << e.what() << std::endl;
    return 1;
  }
}
