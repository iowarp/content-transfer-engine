# Deployment Dockerfile for Content Transfer Engine (CTE)
# Inherits from iowarp/iowarp-runtime-build:latest and uses artifacts from build.Dockerfile

#==============================================================================
# Use the runtime stage from build.Dockerfile
#==============================================================================
FROM iowarp/context-transfer-engine-build:latest AS deploy

# Create necessary directories
RUN mkdir -p /var/log/cte

# Create a wrapper script for easier container startup
RUN echo '#!/bin/bash\n\
set -e\n\
\n\
# Check if configuration file exists\n\
if [ ! -f "${WRP_CTE_CONF}" ]; then\n\
    echo "ERROR: Configuration file not found at ${WRP_CTE_CONF}"\n\
    echo "Please mount a configuration file to ${WRP_CTE_CONF}"\n\
    exit 1\n\
fi\n\
\n\
# Create storage directories from config if needed\n\
echo "Initializing CTE storage directories..."\n\
mkdir -p /mnt/cte_storage\n\
\n\
# Launch CTE with the specified pool query (default: local)\n\
POOL_QUERY=${CTE_POOL_QUERY:-local}\n\
echo "Starting Content Transfer Engine with pool query: ${POOL_QUERY}"\n\
exec /usr/local/bin/launch_cte ${POOL_QUERY}\n\
' > /usr/local/bin/start-cte.sh && chmod +x /usr/local/bin/start-cte.sh

# Set the entrypoint to the wrapper script
ENTRYPOINT ["/usr/local/bin/start-cte.sh"]
