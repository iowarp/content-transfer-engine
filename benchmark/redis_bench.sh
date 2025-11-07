#!/bin/bash
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
# Distributed under BSD 3-Clause license.                                   *
# Copyright by The HDF Group.                                               *
# Copyright by the Illinois Institute of Technology.                        *
# All rights reserved.                                                      *
#                                                                           *
# This file is part of Hermes. The full Hermes copyright notice, including  *
# terms governing use, modification, and redistribution, is contained in    *
# the COPYING file, which can be found at the top directory. If you do not  *
# have access to the file, you may request a copy from help@hdfgroup.org.   *
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

#
# Redis Benchmark Script
#
# This benchmark measures the performance of SET, GET, and combined operations
# in Redis, comparable to the CTE Core benchmark (wrp_cte_bench).
#
# Usage:
#   ./redis_bench.sh <test_case> <num_clients> <io_size> <io_count>
#
# Parameters:
#   test_case: Benchmark to conduct (Set, Get, SetGet, All)
#   num_clients: Number of parallel connections (similar to depth in CTE)
#   io_size: Size of values in bytes (supports k/K, m/M suffixes)
#   io_count: Number of operations to perform
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Redis server configuration
REDIS_PORT=6379
REDIS_HOST=127.0.0.1
REDIS_PID_FILE="/tmp/redis_bench_server.pid"
REDIS_LOG_FILE="/tmp/redis_bench_server.log"
REDIS_SERVER_CMD="redis-server"
REDIS_CLI_CMD="redis-cli"
REDIS_BENCHMARK_CMD="redis-benchmark"

# Parse size string with k/K, m/M suffixes
parse_size() {
    local size_str="$1"
    local num=""
    local suffix=""

    # Extract number and suffix
    if [[ $size_str =~ ^([0-9]+)([kKmM]?)$ ]]; then
        num="${BASH_REMATCH[1]}"
        suffix="${BASH_REMATCH[2]}"
    else
        echo "Error: Invalid size format: $size_str" >&2
        return 1
    fi

    local multiplier=1
    case "${suffix,,}" in
        k) multiplier=1024 ;;
        m) multiplier=$((1024 * 1024)) ;;
        *) multiplier=1 ;;
    esac

    echo $((num * multiplier))
}

# Format size to human-readable string
format_size() {
    local bytes=$1
    if [ $bytes -ge $((1024 * 1024 * 1024)) ]; then
        echo "$((bytes / (1024 * 1024 * 1024))) GB"
    elif [ $bytes -ge $((1024 * 1024)) ]; then
        echo "$((bytes / (1024 * 1024))) MB"
    elif [ $bytes -ge 1024 ]; then
        echo "$((bytes / 1024)) KB"
    else
        echo "$bytes B"
    fi
}

# Check if Redis server is running
is_redis_running() {
    $REDIS_CLI_CMD -h $REDIS_HOST -p $REDIS_PORT ping >/dev/null 2>&1
}

# Start Redis server
start_redis_server() {
    echo -e "${GREEN}Starting Redis server on port $REDIS_PORT...${NC}"

    if is_redis_running; then
        echo -e "${YELLOW}Redis server is already running on port $REDIS_PORT${NC}"
        echo -e "${YELLOW}Using existing server${NC}"
        return 0
    fi

    # Start Redis server with default configuration
    $REDIS_SERVER_CMD --port $REDIS_PORT \
                      --daemonize yes \
                      --pidfile $REDIS_PID_FILE \
                      --logfile $REDIS_LOG_FILE \
                      --save "" \
                      --appendonly no

    # Wait for server to start
    local retries=10
    local count=0
    while [ $count -lt $retries ]; do
        if is_redis_running; then
            echo -e "${GREEN}Redis server started successfully${NC}"
            return 0
        fi
        sleep 0.5
        count=$((count + 1))
    done

    echo -e "${RED}Error: Failed to start Redis server${NC}" >&2
    return 1
}

# Stop Redis server
stop_redis_server() {
    echo -e "${GREEN}Stopping Redis server...${NC}"

    if [ -f "$REDIS_PID_FILE" ]; then
        $REDIS_CLI_CMD -h $REDIS_HOST -p $REDIS_PORT shutdown >/dev/null 2>&1 || true
        sleep 1
        rm -f "$REDIS_PID_FILE"
    fi

    echo -e "${GREEN}Redis server stopped${NC}"
}

# Flush Redis database
flush_redis() {
    echo -e "${YELLOW}Flushing Redis database...${NC}"
    $REDIS_CLI_CMD -h $REDIS_HOST -p $REDIS_PORT flushall >/dev/null 2>&1
}

# Run SET benchmark
run_set_benchmark() {
    local num_clients=$1
    local io_size=$2
    local io_count=$3

    echo ""
    echo "=== Running SET Benchmark ==="

    flush_redis

    $REDIS_BENCHMARK_CMD -h $REDIS_HOST -p $REDIS_PORT \
                         -t set \
                         -c $num_clients \
                         -n $io_count \
                         -d $io_size \
                         --csv | tail -1 | while IFS=, read -r test requests_per_sec; do
        echo ""
        echo "SET Results:"
        echo "  Requests/sec: $requests_per_sec"

        # Calculate bandwidth in MB/s using pure bash
        # Remove quotes from requests_per_sec and extract first number (requests/sec)
        local rps=$(echo "$requests_per_sec" | cut -d',' -f1 | tr -d '"')
        local bandwidth_bytes=$(awk "BEGIN {printf \"%.2f\", $rps * $io_size}")
        local throughput=$(awk "BEGIN {printf \"%.2f\", $bandwidth_bytes / (1024 * 1024)}")
        echo "  Bandwidth: $throughput MB/s"
    done

    echo "============================"
}

# Run GET benchmark
run_get_benchmark() {
    local num_clients=$1
    local io_size=$2
    local io_count=$3

    echo ""
    echo "=== Running GET Benchmark ==="

    # First populate data
    echo "Populating data for GET benchmark..."
    flush_redis

    $REDIS_BENCHMARK_CMD -h $REDIS_HOST -p $REDIS_PORT \
                         -t set \
                         -c $num_clients \
                         -n $io_count \
                         -d $io_size \
                         -q >/dev/null 2>&1

    echo "Starting GET benchmark..."

    $REDIS_BENCHMARK_CMD -h $REDIS_HOST -p $REDIS_PORT \
                         -t get \
                         -c $num_clients \
                         -n $io_count \
                         -d $io_size \
                         --csv | tail -1 | while IFS=, read -r test requests_per_sec; do
        echo ""
        echo "GET Results:"
        echo "  Requests/sec: $requests_per_sec"

        # Calculate bandwidth in MB/s using pure bash
        # Remove quotes from requests_per_sec and extract first number (requests/sec)
        local rps=$(echo "$requests_per_sec" | cut -d',' -f1 | tr -d '"')
        local bandwidth_bytes=$(awk "BEGIN {printf \"%.2f\", $rps * $io_size}")
        local throughput=$(awk "BEGIN {printf \"%.2f\", $bandwidth_bytes / (1024 * 1024)}")
        echo "  Bandwidth: $throughput MB/s"
    done

    echo "============================"
}

# Run SET+GET benchmark
run_setget_benchmark() {
    local num_clients=$1
    local io_size=$2
    local io_count=$3

    echo ""
    echo "=== Running SET+GET Benchmark ==="

    flush_redis

    # Run both SET and GET operations
    local total_ops=$((io_count * 2))

    $REDIS_BENCHMARK_CMD -h $REDIS_HOST -p $REDIS_PORT \
                         -t set,get \
                         -c $num_clients \
                         -n $io_count \
                         -d $io_size \
                         --csv | tail -2 | while IFS=, read -r test requests_per_sec; do
        echo ""
        echo "$test Results:"
        echo "  Requests/sec: $requests_per_sec"

        # Calculate bandwidth in MB/s using pure bash
        # Remove quotes from requests_per_sec and extract first number (requests/sec)
        local rps=$(echo "$requests_per_sec" | cut -d',' -f1 | tr -d '"')
        local bandwidth_bytes=$(awk "BEGIN {printf \"%.2f\", $rps * $io_size}")
        local throughput=$(awk "BEGIN {printf \"%.2f\", $bandwidth_bytes / (1024 * 1024)}")
        echo "  Bandwidth: $throughput MB/s"
    done

    echo "============================"
}

# Run all benchmarks
run_all_benchmarks() {
    local num_clients=$1
    local io_size=$2
    local io_count=$3

    run_set_benchmark $num_clients $io_size $io_count
    run_get_benchmark $num_clients $io_size $io_count
    run_setget_benchmark $num_clients $io_size $io_count
}

# Print usage
print_usage() {
    echo "Usage: $0 <test_case> <num_clients> <io_size> <io_count>"
    echo ""
    echo "Parameters:"
    echo "  test_case: Benchmark to conduct (Set, Get, SetGet, All)"
    echo "  num_clients: Number of parallel connections (e.g., 4)"
    echo "  io_size: Size of values in bytes (e.g., 1m, 4k, 1024)"
    echo "  io_count: Number of operations to perform (e.g., 10000)"
    echo ""
    echo "Examples:"
    echo "  $0 Set 4 1m 10000"
    echo "  $0 Get 8 4k 100000"
    echo "  $0 SetGet 4 1m 10000"
    echo "  $0 All 4 1m 10000"
}

# Main script
main() {
    # Check arguments
    if [ $# -ne 4 ]; then
        print_usage
        exit 1
    fi

    local test_case="$1"
    local num_clients="$2"
    local io_size_str="$3"
    local io_count="$4"

    # Parse I/O size
    local io_size=$(parse_size "$io_size_str")
    if [ $? -ne 0 ]; then
        exit 1
    fi

    # Validate parameters
    if [ $num_clients -le 0 ] || [ $io_size -le 0 ] || [ $io_count -le 0 ]; then
        echo -e "${RED}Error: Invalid parameters${NC}" >&2
        echo "  num_clients must be > 0" >&2
        echo "  io_size must be > 0" >&2
        echo "  io_count must be > 0" >&2
        exit 1
    fi

    # Check if redis commands are available
    if ! command -v $REDIS_SERVER_CMD &> /dev/null; then
        echo -e "${RED}Error: redis-server not found${NC}" >&2
        echo "Please install Redis first" >&2
        exit 1
    fi

    if ! command -v $REDIS_BENCHMARK_CMD &> /dev/null; then
        echo -e "${RED}Error: redis-benchmark not found${NC}" >&2
        echo "Please install Redis first" >&2
        exit 1
    fi

    # Print benchmark configuration
    echo "=== Redis Benchmark ==="
    echo "Test case: $test_case"
    echo "Parallel clients: $num_clients"
    echo "Value size: $(format_size $io_size)"
    echo "Operations: $io_count"
    echo "Total I/O: $(format_size $((io_size * io_count)))"
    echo "============================"
    echo ""

    # Setup cleanup trap
    trap stop_redis_server EXIT

    # Start Redis server
    start_redis_server
    if [ $? -ne 0 ]; then
        exit 1
    fi

    # Run benchmark based on test case
    case "${test_case,,}" in
        set)
            run_set_benchmark $num_clients $io_size $io_count
            ;;
        get)
            run_get_benchmark $num_clients $io_size $io_count
            ;;
        setget)
            run_setget_benchmark $num_clients $io_size $io_count
            ;;
        all)
            run_all_benchmarks $num_clients $io_size $io_count
            ;;
        *)
            echo -e "${RED}Error: Unknown test case: $test_case${NC}" >&2
            echo "Valid options: Set, Get, SetGet, All" >&2
            exit 1
            ;;
    esac

    echo ""
    echo -e "${GREEN}Benchmark completed successfully${NC}"
}

# Run main function
main "$@"
