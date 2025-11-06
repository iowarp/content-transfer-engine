#!/bin/bash
# CTE Benchmark Distributed Test Runner
# Purpose: Execute CTE benchmarks in a distributed Docker Compose cluster
# Usage: ./run_tests.sh [test_case] [OPTIONS]
#   test_case: put, get, putget, all (default: all)

set -e

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Test parameters (can be overridden)
TEST_CASE="${1:-all}"
DEPTH="${DEPTH:-1}"
IO_SIZE="${IO_SIZE:-1m}"
IO_COUNT="${IO_COUNT:-1}"
NPROCS="${NPROCS:-4}"
PPN="${PPN:-2}"

echo -e "${GREEN}===================================================${NC}"
echo -e "${GREEN}CTE Benchmark Distributed Test Runner${NC}"
echo -e "${GREEN}===================================================${NC}"
echo ""

# Function to print colored messages
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if docker compose is available
check_docker_compose() {
    if command -v docker-compose &> /dev/null; then
        DOCKER_COMPOSE="docker-compose"
    elif docker compose version &> /dev/null; then
        DOCKER_COMPOSE="docker compose"
    else
        print_error "Docker Compose not found. Please install Docker Compose."
        exit 1
    fi
    print_info "Using: $DOCKER_COMPOSE"
}

# Function to cleanup resources
cleanup() {
    print_info "Cleaning up Docker containers..."
    cd "$SCRIPT_DIR"
    $DOCKER_COMPOSE down -v 2>/dev/null || true
}

# Function to setup environment
setup_environment() {
    print_info "Setting up test environment..."
    print_info "Benchmark results will be displayed to stdout only"
}

# Function to start Docker Compose cluster
start_cluster() {
    print_info "Starting Docker Compose cluster..."
    cd "$SCRIPT_DIR"
    $DOCKER_COMPOSE up -d

    # Wait for containers to be ready
    print_info "Waiting for containers to be ready and runtime to initialize..."
    sleep 20

    # Check container status
    print_info "Container status:"
    $DOCKER_COMPOSE ps
}

# Function to verify SSH connectivity between nodes
verify_ssh_connectivity() {
    print_info "Verifying SSH connectivity between nodes..."

    local ssh_failed=0
    local nodes=("cte-bench-node2" "cte-bench-node3" "cte-bench-node4")

    for node in "${nodes[@]}"; do
        print_info "Testing SSH from node1 to $node..."
        if docker exec cte-bench-node1 ssh -o StrictHostKeyChecking=no -o ConnectTimeout=5 "$node" echo "SSH to $node successful" 2>&1; then
            print_info "✓ SSH to $node: SUCCESS"
        else
            print_error "✗ SSH to $node: FAILED"
            ssh_failed=1
        fi
    done

    if [ $ssh_failed -eq 1 ]; then
        print_error "SSH connectivity verification failed"
        print_error "Please ensure SSH is running on all nodes and SSH keys are properly configured"
        exit 1
    fi

    print_info "SSH connectivity verified successfully"
}

# Function to create MPI hostfile
create_mpi_hostfile() {
    print_info "Creating MPI hostfile..."

    # Create hostfile directly in container
    docker exec cte-bench-node1 bash -c "cat > /tmp/mpi_hostfile << EOF
cte-bench-node1 slots=$PPN
cte-bench-node2 slots=$PPN
cte-bench-node3 slots=$PPN
cte-bench-node4 slots=$PPN
EOF"

    print_info "MPI hostfile created with 4 nodes ($PPN slots each)"
}

# Function to run benchmark via MPI
run_benchmark() {
    local test_name="$1"

    print_info "Running benchmark: $test_name"
    print_info "Configuration: depth=$DEPTH, io_size=$IO_SIZE, io_count=$IO_COUNT, nprocs=$NPROCS"
    echo ""
    echo -e "${GREEN}=== $test_name Benchmark Results ===${NC}"

    # Run benchmark using mpirun
    docker exec cte-bench-node1 bash -c "
        mpirun -np $NPROCS \
            --hostfile /tmp/mpi_hostfile \
            wrp_cte_bench $test_name $DEPTH $IO_SIZE $IO_COUNT
    "

    local exit_code=$?
    echo ""

    if [ $exit_code -eq 0 ]; then
        print_info "$test_name benchmark completed successfully"
    else
        print_error "$test_name benchmark failed"
        return 1
    fi
}

# Function to display results summary
display_results() {
    print_info "All benchmark tests completed"
    echo ""
}

# Function to show usage
usage() {
    cat << EOF
Usage: $0 [test_case] [OPTIONS]

Run CTE benchmarks in distributed environment.

TEST CASES:
    put          Run Put (write) benchmark only
    get          Run Get (read) benchmark only
    putget       Run PutGet (combined) benchmark only
    all          Run all benchmarks (default)

OPTIONS:
    Environment variables to customize benchmark parameters:
    DEPTH        Number of async operations (default: 8)
    IO_SIZE      Size of each I/O operation (default: 4m)
    IO_COUNT     Number of operations per process (default: 200)
    NPROCS       Total MPI processes (default: 4)
    PPN          Processes per node (default: 2)

EXAMPLES:
    # Run all benchmarks with defaults
    ./run_tests.sh all

    # Run Put benchmark only
    ./run_tests.sh put

    # Run with custom parameters
    DEPTH=16 IO_SIZE=16m IO_COUNT=500 ./run_tests.sh all

    # Run with more processes
    NPROCS=8 PPN=2 ./run_tests.sh putget

EOF
}

# Main execution
main() {
    # Check for help flag
    if [ "$TEST_CASE" = "-h" ] || [ "$TEST_CASE" = "--help" ]; then
        usage
        exit 0
    fi

    print_info "Test configuration: $TEST_CASE"

    # Check prerequisites
    check_docker_compose

    # Setup cleanup trap
    trap cleanup EXIT

    # Setup environment
    setup_environment

    # Start cluster
    start_cluster

    # Verify SSH connectivity first
    verify_ssh_connectivity

    # Create MPI hostfile
    create_mpi_hostfile

    # Run benchmarks based on test case
    case "$TEST_CASE" in
        put)
            run_benchmark "Put"
            ;;
        get)
            run_benchmark "Get"
            ;;
        putget)
            run_benchmark "PutGet"
            ;;
        all)
            print_info "Running all benchmark tests..."
            run_benchmark "Put"
            run_benchmark "Get"
            run_benchmark "PutGet"
            ;;
        *)
            print_error "Invalid test case: $TEST_CASE"
            print_info "Valid options: put, get, putget, all"
            usage
            exit 1
            ;;
    esac

    # Display results
    display_results

    print_info "Benchmark tests completed successfully!"
}

# Run main function
main
