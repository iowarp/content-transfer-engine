# Quick Start: CTE Benchmark Distributed Tests

## Run Tests in 3 Steps

### 1. Prerequisites
Ensure you have:
- Docker and Docker Compose installed
- CTE Docker image built as `iowarp/context-transfer-engine-build:latest`

### 2. Run Tests
```bash
cd test/unit/benchmark
./run_tests.sh all
```

### 3. View Results
Results are saved in `results/` directory:
```bash
cat results/put_results.txt
cat results/get_results.txt
cat results/putget_results.txt
```

## Test Options

```bash
# Run all benchmarks
./run_tests.sh all

# Run specific benchmark
./run_tests.sh put      # Put (write) benchmark only
./run_tests.sh get      # Get (read) benchmark only
./run_tests.sh putget   # Combined Put+Get benchmark
```

## What Gets Tested

### Put Benchmark
- **Operation**: Write data to CTE using MPI
- **Config**: 4 MPI processes, 8 async depth, 4MB I/O size, 200 operations
- **Output**: `results/put_results.txt`

### Get Benchmark
- **Operation**: Read data from CTE using MPI
- **Config**: 4 MPI processes, 8 async depth, 4MB I/O size, 200 operations
- **Output**: `results/get_results.txt`

### PutGet Benchmark
- **Operation**: Combined write and read using MPI
- **Config**: 4 MPI processes, 8 async depth, 4MB I/O size, 200 operations
- **Output**: `results/putget_results.txt`

## Cluster Architecture

The test creates a 4-node Docker Compose cluster:
- **cte-bench-node1**: Coordinator (runs runtime, CTE, and mpirun)
- **cte-bench-node2-4**: Worker nodes (run runtime)
- **Total**: 8 MPI slots (2 per node)

### Startup Sequence
1. All nodes start Chimaera runtime
2. Node 1 waits 5 seconds for runtime initialization
3. Node 1 waits additional 10 seconds for other nodes
4. Node 1 launches CTE
5. Script runs benchmarks via mpirun

## Customization

Use environment variables to customize benchmark parameters:

```bash
# More concurrent operations
DEPTH=16 ./run_tests.sh all

# Larger I/O size
IO_SIZE=16m ./run_tests.sh put

# More operations
IO_COUNT=500 ./run_tests.sh all

# More MPI processes
NPROCS=8 PPN=2 ./run_tests.sh all
```

## Configuration Files

### chimaera_config.yaml
- Hostfile for 4 nodes
- TCP protocol on port 8080
- 16GB shared memory
- 8 workers per node

### cte_config.yaml
- 2x 1GB storage devices
- DPE: max_bw (maximum bandwidth)
- Neighborhood: 4 nodes
- Timeouts and polling configuration

### hostfile
Lists the 4 cluster nodes:
```
cte-bench-node1
cte-bench-node2
cte-bench-node3
cte-bench-node4
```

## Troubleshooting

### Script not executable
```bash
chmod +x run_tests.sh
```

### Docker permission denied
```bash
sudo usermod -aG docker $USER
# Log out and back in
```

### Container issues
```bash
# Check Docker status
docker ps

# View logs
docker logs cte-bench-node1

# Manual cleanup
docker compose down -v
```

### MPI issues
```bash
# Test MPI connectivity
docker exec cte-bench-node1 mpirun -np 4 \
    --hostfile /workspace/results/mpi_hostfile hostname
```

## Manual Execution

You can also run benchmarks manually:

```bash
# Start cluster
docker compose up -d
sleep 20

# Create MPI hostfile
cat > results/mpi_hostfile << 'EOF'
cte-bench-node1 slots=2
cte-bench-node2 slots=2
cte-bench-node3 slots=2
cte-bench-node4 slots=2
EOF

# Copy to container
docker cp results/mpi_hostfile cte-bench-node1:/workspace/results/

# Run benchmark
docker exec cte-bench-node1 mpirun -np 4 \
    --hostfile /workspace/results/mpi_hostfile \
    wrp_cte_bench Put 8 4m 200

# Cleanup
docker compose down -v
```

## For More Information

See [README.md](README.md) for complete documentation.
