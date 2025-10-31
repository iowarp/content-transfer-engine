#!/bin/bash
# Distributed CTE Test Runner
# This script sets up and runs the distributed CTE tests in Docker containers

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored message
print_msg() {
    local color=$1
    shift
    echo -e "${color}$@${NC}"
}

print_header() {
    echo ""
    print_msg "$BLUE" "=================================="
    print_msg "$BLUE" "$@"
    print_msg "$BLUE" "=================================="
}

print_success() {
    print_msg "$GREEN" "✓ $@"
}

print_error() {
    print_msg "$RED" "✗ $@"
}

print_warning() {
    print_msg "$YELLOW" "⚠ $@"
}

# Function to check prerequisites
check_prerequisites() {
    print_header "Checking Prerequisites"

    if ! command -v docker &> /dev/null; then
        print_error "Docker is not installed"
        exit 1
    fi
    print_success "Docker found"

    if ! docker compose version &> /dev/null; then
        print_error "Docker Compose is not installed"
        exit 1
    fi
    print_success "Docker Compose found"

    # Check if docker daemon is running
    if ! docker ps &> /dev/null; then
        print_error "Docker daemon is not running"
        exit 1
    fi
    print_success "Docker daemon is running"

    # Check for required files
    if [ ! -f "docker-compose.yaml" ]; then
        print_error "docker-compose.yaml not found"
        exit 1
    fi
    print_success "docker-compose.yaml found"

    if [ ! -f "hostfile" ]; then
        print_error "hostfile not found"
        exit 1
    fi
    print_success "hostfile found"

    if [ ! -f "cte_config.yaml" ]; then
        print_error "cte_config.yaml not found"
        exit 1
    fi
    print_success "cte_config.yaml found"
}

# Function to clean up previous runs
cleanup() {
    print_header "Cleaning Up Previous Test Environment"

    if docker compose ps -q 2>/dev/null | grep -q .; then
        print_msg "$YELLOW" "Stopping running containers..."
        docker compose down -v 2>/dev/null || true
        print_success "Containers stopped"
    else
        print_success "No running containers to clean up"
    fi
}

# Function to start the test environment
start_environment() {
    print_header "Starting 4-Node Distributed Test Environment"

    print_msg "$BLUE" "Starting containers..."
    docker compose up -d

    # Wait for containers to be ready
    print_msg "$BLUE" "Waiting for containers to start..."
    sleep 5

    # Check if all containers are running
    local running=$(docker compose ps -q | wc -l)
    if [ "$running" -ne 4 ]; then
        print_error "Not all containers started (expected 4, got $running)"
        docker compose logs
        exit 1
    fi
    print_success "All 4 containers started successfully"
}

# Function to monitor test progress
monitor_tests() {
    print_header "Monitoring Test Execution"

    print_msg "$BLUE" "Following logs from node 1 (press Ctrl+C to stop monitoring)..."
    echo ""

    # Follow logs from node 1 until tests complete
    docker compose logs -f cte-node1 &
    LOG_PID=$!

    # Wait for test completion or timeout
    local timeout=600  # 10 minutes timeout
    local elapsed=0
    local interval=5

    while [ $elapsed -lt $timeout ]; do
        # Check if tests have completed
        if docker compose logs cte-node1 2>/dev/null | grep -q "Tests complete"; then
            sleep 2  # Give it a moment to finish logging
            kill $LOG_PID 2>/dev/null || true
            print_success "Tests completed"
            return 0
        fi

        # Check if tests failed
        if docker compose logs cte-node1 2>/dev/null | grep -q "Test.*FAILED"; then
            sleep 2
            kill $LOG_PID 2>/dev/null || true
            print_error "Tests failed"
            return 1
        fi

        sleep $interval
        elapsed=$((elapsed + interval))
    done

    kill $LOG_PID 2>/dev/null || true
    print_error "Test execution timed out after ${timeout}s"
    return 1
}

# Function to display test results
show_results() {
    print_header "Test Results"

    # Extract test results from node 1 logs
    docker compose logs cte-node1 | grep -A 20 "Running distributed unit tests" || true

    # Check for test summary
    if docker compose logs cte-node1 | grep -q "All tests passed"; then
        print_success "All distributed tests passed!"
        return 0
    elif docker compose logs cte-node1 | grep -q "tests passed"; then
        print_warning "Some tests passed (see details above)"
        return 0
    else
        print_error "Tests did not complete successfully"
        return 1
    fi
}

# Function to show usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Run CTE distributed unit tests in a 4-node Docker environment.

OPTIONS:
    -h, --help              Show this help message
    -c, --cleanup-only      Only cleanup previous runs, don't start new tests
    -k, --keep              Keep containers running after tests (for debugging)
    -l, --logs              Show full logs after test completion
    -n, --no-cleanup        Don't cleanup before starting tests

EXAMPLES:
    # Run tests with default settings
    $0

    # Run tests and keep containers for debugging
    $0 --keep

    # Only cleanup previous test runs
    $0 --cleanup-only

    # Run tests and show full logs afterward
    $0 --logs
EOF
}

# Main execution
main() {
    local cleanup_only=false
    local keep_containers=false
    local show_logs=false
    local do_cleanup=true

    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                usage
                exit 0
                ;;
            -c|--cleanup-only)
                cleanup_only=true
                shift
                ;;
            -k|--keep)
                keep_containers=true
                shift
                ;;
            -l|--logs)
                show_logs=true
                shift
                ;;
            -n|--no-cleanup)
                do_cleanup=false
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                usage
                exit 1
                ;;
        esac
    done

    print_header "CTE Distributed Test Runner"

    # Check prerequisites
    check_prerequisites

    # Cleanup if requested
    if [ "$do_cleanup" = true ] || [ "$cleanup_only" = true ]; then
        cleanup
    fi

    # Exit if cleanup-only mode
    if [ "$cleanup_only" = true ]; then
        print_success "Cleanup complete"
        exit 0
    fi

    # Start test environment
    start_environment

    # Monitor test execution
    if monitor_tests; then
        show_results
        TEST_EXIT_CODE=$?
    else
        print_error "Test monitoring failed"
        TEST_EXIT_CODE=1
    fi

    # Show full logs if requested
    if [ "$show_logs" = true ]; then
        print_header "Full Container Logs"
        docker compose logs
    fi

    # Cleanup unless keep flag is set
    if [ "$keep_containers" = true ]; then
        print_warning "Containers kept running for debugging"
        print_msg "$BLUE" "To view logs: docker compose logs -f"
        print_msg "$BLUE" "To cleanup: docker compose down -v"
    else
        print_header "Cleaning Up Test Environment"
        docker compose down -v
        print_success "Cleanup complete"
    fi

    # Exit with test result
    if [ $TEST_EXIT_CODE -eq 0 ]; then
        print_success "Distributed tests completed successfully"
        exit 0
    else
        print_error "Distributed tests failed"
        exit 1
    fi
}

# Run main function
main "$@"
