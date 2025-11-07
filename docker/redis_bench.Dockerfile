# Dockerfile for Redis Benchmark
# Runs Redis benchmarks for performance comparison with CTE
#
# Usage:
#   docker run iowarp/cte-redis-bench:latest Set 4 1m 10000
#   docker run iowarp/cte-redis-bench:latest Get 8 4k 100000
#   docker run iowarp/cte-redis-bench:latest SetGet 4 1m 10000
#   docker run iowarp/cte-redis-bench:latest All 4 1m 10000

FROM iowarp/context-transfer-engine-build:latest

# Install Redis server and tools
# Using redis from Ubuntu repositories for simplicity and stability
RUN sudo apt-get update && \
    sudo apt-get install -y redis-server redis-tools && \
    sudo apt-get clean && \
    sudo rm -rf /var/lib/apt/lists/*

# Create benchmark directory
RUN sudo mkdir -p /benchmarks && \
    sudo chown -R iowarp:ubuntu /benchmarks

# Copy the Redis benchmark script
COPY benchmark/redis_bench.sh /benchmarks/redis_bench.sh

# Make the script executable
RUN sudo chmod +x /benchmarks/redis_bench.sh

# Set working directory
WORKDIR /benchmarks

# Set the entrypoint to run the benchmark script
# Arguments: test_case, num_clients, io_size, io_count
ENTRYPOINT ["/benchmarks/redis_bench.sh"]

# Default arguments for demonstration
CMD ["Set", "4", "1m", "10000"]
