# CTE Benchmark Distributed Test Suite

This directory contains distributed benchmark tests for the Content Transfer Engine (CTE) using direct MPI execution and Docker Compose.

## Overview

The benchmark test suite runs the `wrp_cte_bench` benchmark application in a distributed environment with multiple nodes to test CTE performance under realistic distributed workloads.

## Directory Structure

```
test/unit/benchmark/
├── README.md                    # This file
├── QUICKSTART.md                # Quick start guide
├── .gitignore                   # Ignore temporary files
├── docker-compose.yaml          # Docker Compose configuration for 4-node cluster
├── chimaera_config.yaml         # Chimaera runtime configuration
├── cte_config.yaml              # CTE storage and DPE configuration
├── hostfile                     # Hostfile for Chimaera runtime
└── run_tests.sh                 # Main test runner script
```

## Prerequisites

1. **Docker and Docker Compose**: Ensure Docker is installed and running
2. **CTE Docker Image**: Build the CTE Docker image named `iowarp/context-transfer-engine-build:latest`
3. **Built CTE**: CTE should be built and installed in the Docker image with `wrp_cte_bench` executable

## Benchmark Test Cases

### Put Benchmark
Tests write performance by executing Put operations across multiple nodes using MPI.

**Default Configuration:**
- Test case: Put
- Async depth: 8 concurrent operations
- I/O size: 4 MB per operation
- I/O count: 200 operations per process
- MPI processes: 4 total (2 per node)

### Get Benchmark
Tests read performance by executing Get operations across multiple nodes using MPI.

**Default Configuration:**
- Test case: Get
- Async depth: 8 concurrent operations
- I/O size: 4 MB per operation
- I/O count: 200 operations per process
- MPI processes: 4 total (2 per node)

### PutGet Benchmark
Tests combined write and read performance across multiple nodes.

**Default Configuration:**
- Test case: PutGet
- Async depth: 8 concurrent operations
- I/O size: 4 MB per operation
- I/O count: 200 operations per process
- MPI processes: 4 total (2 per node)

## Usage

### Run All Benchmarks
```bash
cd test/unit/benchmark
./run_tests.sh all
```

### Run Specific Benchmark
```bash
# Put benchmark only
./run_tests.sh put

# Get benchmark only
./run_tests.sh get

# PutGet benchmark only
./run_tests.sh putget
```

### Customize Benchmark Parameters
```bash
# Run with custom depth and I/O size
DEPTH=16 IO_SIZE=16m ./run_tests.sh all

# Run with more operations
IO_COUNT=500 ./run_tests.sh put

# Run with more processes
NPROCS=8 PPN=2 ./run_tests.sh all
```

## Test Execution Flow

1. **Setup**: Prepares test environment
2. **Cluster Start**: Launches 4-node Docker Compose cluster
   - Node 1: Starts runtime (5s sleep), launches CTE (after 15s total)
   - Nodes 2-4: Start runtime and wait
3. **MPI Hostfile Creation**: Generates MPI hostfile in container at /tmp/mpi_hostfile
4. **Benchmark Execution**: Runs benchmarks using `mpirun` across all nodes
5. **Results Display**: Benchmark results are displayed directly to stdout
6. **Cleanup**: Stops and removes Docker containers

## Docker Compose Cluster

The test uses a 4-node cluster:
- **cte-bench-node1**: Main coordinator node (runs mpirun commands)
- **cte-bench-node2-4**: Worker nodes

Each node has:
- 2 MPI slots (configurable via PPN)
- Dedicated storage volume
- Access to Chimaera and CTE configuration files
- 16GB shared memory
- 16GB memory limit

### Node Startup Sequence

**Node 1:**
1. Creates storage directories
2. Starts Chimaera runtime in background
3. Waits 15 seconds (runtime initialization and for other nodes)
4. Keeps running for benchmark execution

**Nodes 2-4:**
1. Create storage directories
2. Start Chimaera runtime in foreground
3. Wait for commands from Node 1

## Configuration Files

### chimaera_config.yaml
Configures the Chimaera runtime:
- Hostfile: `/etc/iowarp/hostfile`
- Protocol: TCP on port 8080
- Shared memory: 16GB
- Queue depth: 1024
- Workers: 8 per node

### cte_config.yaml
Configures CTE storage and data placement:
- Storage devices: 2x 1GB file-based devices
- DPE type: `max_bw` (maximum bandwidth)
- Neighborhood: 4 nodes
- Target timeout: 30000ms
- Poll period: 5000ms

### hostfile
Lists the 4 nodes for Chimaera runtime:
```
cte-bench-node1
cte-bench-node2
cte-bench-node3
cte-bench-node4
```

## Customizing Benchmarks

### Modify Benchmark Parameters

Use environment variables:

```bash
# Increase concurrency
DEPTH=16 ./run_tests.sh all

# Larger I/O operations
IO_SIZE=16m ./run_tests.sh put

# More operations per process
IO_COUNT=1000 ./run_tests.sh all

# More MPI processes
NPROCS=8 PPN=2 ./run_tests.sh all
```

### Modify CTE Configuration

Edit `cte_config.yaml` to adjust:
- Storage device paths and capacities
- DPE algorithm (`random`, `round_robin`, `max_bw`)
- Neighborhood size
- Timeout and poll periods

Example:
```yaml
storage:
  - path: "/mnt/cte_storage/storage1.bin"
    capacity_limit: "10GB"  # Larger storage
    score: 0.9

dpe:
  dpe_type: "round_robin"   # Different DPE
  neighborhood: 8            # More neighbors
```

### Modify Cluster Size

Edit `docker-compose.yaml` to add/remove nodes and update:
1. `hostfile` with new node names
2. `chimaera_config.yaml` hostfile reference
3. MPI hostfile creation in `run_tests.sh`

## Results Interpretation

Benchmark results are displayed directly to stdout and include:
- **Total Time**: Total execution time in milliseconds (ms)
- **Throughput**: Data transfer rate in MB/s
- **IOPS**: I/O operations per second
- **Latency**: Average operation latency in milliseconds (ms)
- **Per-Process Statistics**: Individual MPI rank performance

All timing values are reported in **milliseconds (ms)** as per CLAUDE.md requirements.

Note: Benchmark results are not stored to disk. If you need to save results, redirect the script output:
```bash
./run_tests.sh all > benchmark_output.txt 2>&1
```

## Troubleshooting

### Docker Compose Not Found
```bash
# Install Docker Compose
sudo apt-get install docker-compose
# or use docker compose plugin
docker compose version
```

### Permission Denied
```bash
# Make script executable
chmod +x run_tests.sh
```

### Container Startup Issues
```bash
# Check Docker status
docker ps

# View container logs
docker logs cte-bench-node1
docker logs cte-bench-node2
```

### CTE Launch Failures
```bash
# Check if runtime started correctly
docker exec cte-bench-node1 ps aux | grep chimaera

# Check CTE configuration
docker exec cte-bench-node1 cat /etc/iowarp/cte_config.yaml

# View full logs
docker compose logs
```

### MPI Errors
```bash
# Check MPI hostfile
docker exec cte-bench-node1 cat /tmp/mpi_hostfile

# Test MPI connectivity
docker exec cte-bench-node1 mpirun -np 4 --hostfile /tmp/mpi_hostfile hostname
```

## Integration with CI/CD

The benchmark tests can be integrated into CI/CD pipelines:

```bash
# Run tests and capture output
cd test/unit/benchmark
./run_tests.sh all > benchmark_output.txt 2>&1
exit_code=$?

# Parse results for performance regression
throughput=$(grep -i "throughput" benchmark_output.txt | awk '{print $2}')
# Add regression check logic

exit $exit_code
```

## Manual Benchmark Execution

You can also run benchmarks manually after starting the cluster:

```bash
# Start the cluster
cd test/unit/benchmark
docker compose up -d
sleep 20  # Wait for initialization

# Create MPI hostfile manually
docker exec cte-bench-node1 bash -c "cat > /tmp/mpi_hostfile << EOF
cte-bench-node1 slots=2
cte-bench-node2 slots=2
cte-bench-node3 slots=2
cte-bench-node4 slots=2
EOF"

# Run benchmark manually
docker exec cte-bench-node1 bash -c "
    mpirun -np 4 \
        --hostfile /tmp/mpi_hostfile \
        wrp_cte_bench Put 8 4m 200
"

# Cleanup
docker compose down -v
```

## See Also

- [Quick Start Guide](QUICKSTART.md)
- [CTE Core Documentation](../../../docs/cte/cte.md)
- [Benchmark Source Code](../../../benchmark/wrp_cte_bench.cc)
- [Distributed Unit Tests](../distributed/README.md)
