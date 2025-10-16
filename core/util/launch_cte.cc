// Copyright 2024 IOWarp
// Utility to launch the Content Transfer Engine (CTE)

#include <wrp_cte/core/core_client.h>
#include <iostream>

int main(int argc, char** argv) {
  try {
    // Initialize the CTE client - this triggers iowarp-runtime to create the CTE
    wrp_cte::core::WRP_CTE_CLIENT_INIT();

    std::cout << "Content Transfer Engine initialized successfully" << std::endl;
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Error initializing Content Transfer Engine: " << e.what() << std::endl;
    return 1;
  }
}
