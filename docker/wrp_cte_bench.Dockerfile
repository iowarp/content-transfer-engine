# Dockerfile for WRP CTE Benchmark
# Runs CTE Core benchmarks for Put, Get, and PutGet operations
#
# Usage with environment variables:
#   docker run -e TEST_CASE=Put -e NUM_PROCS=1 -e DEPTH=4 -e IO_SIZE=1m -e IO_COUNT=100 iowarp/cte-wrp-bench:latest
#   docker run -e TEST_CASE=Get -e NUM_PROCS=4 -e DEPTH=8 -e IO_SIZE=4k -e IO_COUNT=1000 iowarp/cte-wrp-bench:latest
#
# Usage with direct arguments:
#   docker run iowarp/cte-wrp-bench:latest Put 1 4 1m 100
#   docker run iowarp/cte-wrp-bench:latest Get 4 8 4k 1000

FROM iowarp/context-transfer-engine-build:latest

# Create benchmark directory
RUN sudo mkdir -p /benchmarks && \
    sudo chown -R iowarp:ubuntu /benchmarks

# Copy the benchmark script and configuration file
COPY benchmark/wrp_cte_bench.sh /benchmarks/wrp_cte_bench.sh
COPY benchmark/cte_config_ram.yaml /benchmarks/cte_config.yaml

# Make the script executable
RUN sudo chmod +x /benchmarks/wrp_cte_bench.sh

# Set working directory
WORKDIR /benchmarks

# Set default environment variables for CTE runtime
ENV CTE_INIT_RUNTIME=0
ENV WRP_CTE_CONF=/benchmarks/cte_config.yaml

# Default benchmark parameters (can be overridden with environment variables)
ENV TEST_CASE=Put
ENV NUM_PROCS=1
ENV DEPTH=4
ENV IO_SIZE=1m
ENV IO_COUNT=100

# Create a wrapper script that handles both direct arguments and environment variables
RUN echo '#!/bin/bash' > /benchmarks/run_bench.sh && \
    echo 'set -e' >> /benchmarks/run_bench.sh && \
    echo '# If arguments are provided, use them directly' >> /benchmarks/run_bench.sh && \
    echo 'if [ $# -eq 5 ]; then' >> /benchmarks/run_bench.sh && \
    echo '    exec /benchmarks/wrp_cte_bench.sh "$@"' >> /benchmarks/run_bench.sh && \
    echo '# Otherwise, use environment variables' >> /benchmarks/run_bench.sh && \
    echo 'else' >> /benchmarks/run_bench.sh && \
    echo '    exec /benchmarks/wrp_cte_bench.sh "$TEST_CASE" "$NUM_PROCS" "$DEPTH" "$IO_SIZE" "$IO_COUNT"' >> /benchmarks/run_bench.sh && \
    echo 'fi' >> /benchmarks/run_bench.sh && \
    sudo chmod +x /benchmarks/run_bench.sh

# Set the entrypoint to run the wrapper script
# Arguments: test_case, num_procs, depth, io_size, io_count
# Or use environment variables: TEST_CASE, NUM_PROCS, DEPTH, IO_SIZE, IO_COUNT
ENTRYPOINT ["/benchmarks/run_bench.sh"]

# Default arguments (Put 1 4 1m 100)
CMD []
