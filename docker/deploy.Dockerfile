# Deployment Dockerfile for Content Transfer Engine (CTE)
# Inherits from the build container and runs launch_cte
FROM iowarp/context-transfer-engine-build:latest

# Create configuration directory
RUN mkdir -p /etc/wrp_cte

# Copy default configuration
COPY config/cte_default.yaml /etc/wrp_cte/wrp_cte.conf

# Set WRP_CTE_CONF environment variable
ENV WRP_CTE_CONF=/etc/wrp_cte/wrp_cte.conf

# Expose default ZeroMQ port
EXPOSE 5555

# Run launch_cte directly with local pool query
CMD ["launch_cte", "local"]
