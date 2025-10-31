# Distributed CTE Unit Tests

This directory contains configuration for running Content Transfer Engine (CTE) unit tests in a distributed 4-node containerized environment.

## Architecture

- **4 Docker containers** representing 4 distributed nodes
- **4 storage targets** - one HDD per node (node1_hdd1, node2_hdd2, node3_hdd3, node4_hdd4)
- **Shared network** - 172.26.0.0/16 subnet for inter-node communication
- **Node 1** - Primary node that builds CTE, launches the runtime, creates CTE, and runs tests
- **Nodes 2-4** - Secondary nodes that wait for build completion, then start their runtimes

## Files

- `docker-compose.yaml` - Docker Compose configuration for 4-node cluster
- `cte_config.yaml` - CTE configuration defining 4 storage targets
- `hostfile` - List of hostnames for distributed runtime
- `README.md` - This file

## Prerequisites

1. Docker and Docker Compose installed
2. IOWarp runtime dependencies image available: `iowarp/iowarp-deps:ai`
3. Spack environment with `iowarp-runtime` package
4. At least 64GB RAM available for containers (16GB per node)

## Usage

### Starting the Distributed Test Environment

```bash
# Navigate to the distributed test directory
cd test/unit/distributed

# Start all 4 nodes
docker-compose up

# Or run in detached mode
docker-compose up -d
```

### What Happens Automatically

1. **Node 1 (Primary)**:
   - Loads spack environment with `iowarp-runtime`
   - Starts IOWarp runtime
   - Builds Content Transfer Engine
   - Installs CTE binaries to shared volume
   - Creates storage directory `/mnt/hdd1`
   - Waits for all nodes to be ready
   - Launches CTE using `util/launch_cte.sh` with the distributed config
   - Runs unit tests (`test_core_functionality`)

2. **Nodes 2-4 (Secondary)**:
   - Wait for binaries to be available in shared volume
   - Load spack environment with `iowarp-runtime`
   - Create storage directories (`/mnt/hdd2`, `/mnt/hdd3`, `/mnt/hdd4`)
   - Start IOWarp runtime
   - Wait for tests to run

### Monitoring Test Execution

```bash
# View logs from all nodes
docker-compose logs -f

# View logs from specific node
docker-compose logs -f cte-node1

# Check test execution status
docker-compose logs cte-node1 | grep -A 20 "Running unit tests"
```

### Executing Commands in Containers

```bash
# Execute command in node 1
docker exec -it cte-distributed-node1 bash

# Execute command in node 2
docker exec -it cte-distributed-node2 bash
```

### Stopping the Environment

```bash
# Stop all containers
docker-compose down

# Stop and remove volumes (cleans up storage)
docker-compose down -v
```

## Network Configuration

- **Subnet**: 172.26.0.0/16
- **Node 1**: 172.26.0.10 (iowarp-node1)
- **Node 2**: 172.26.0.11 (iowarp-node2)
- **Node 3**: 172.26.0.12 (iowarp-node3)
- **Node 4**: 172.26.0.13 (iowarp-node4)

## Storage Configuration

Each node has its own storage target:

| Node | Target Name | Mount Point | Capacity | Manual Score |
|------|-------------|-------------|----------|--------------|
| 1    | node1_hdd1  | /mnt/hdd1   | 10GB     | 0.25         |
| 2    | node2_hdd2  | /mnt/hdd2   | 10GB     | 0.25         |
| 3    | node3_hdd3  | /mnt/hdd3   | 10GB     | 0.25         |
| 4    | node4_hdd4  | /mnt/hdd4   | 10GB     | 0.25         |

All targets have equal manual scores (0.25) to ensure balanced data distribution across nodes.

## Volumes

- `cte-install` - Shared install directory for CTE binaries
- `hdd1-storage` - Storage volume for node 1
- `hdd2-storage` - Storage volume for node 2
- `hdd3-storage` - Storage volume for node 3
- `hdd4-storage` - Storage volume for node 4

## Troubleshooting

### Build Failures

If the build fails on node 1:
```bash
# Check build logs
docker-compose logs cte-node1 | grep -i error

# Restart with clean volumes
docker-compose down -v
docker-compose up
```

### Runtime Connection Issues

If nodes cannot communicate:
```bash
# Check network connectivity
docker exec cte-distributed-node1 ping iowarp-node2
docker exec cte-distributed-node1 ping iowarp-node3
docker exec cte-distributed-node1 ping iowarp-node4

# Verify hostfile is correctly mounted
docker exec cte-distributed-node1 cat /etc/iowarp/hostfile
```

### Test Failures

If tests fail:
```bash
# View detailed test output
docker-compose logs cte-node1 | grep -A 100 "Running unit tests"

# Run tests manually
docker exec -it cte-distributed-node1 bash
cd /content-transfer-engine/build-docker
./test/unit/test_core_functionality
```

### Storage Issues

If storage directories are not accessible:
```bash
# Check mounted volumes
docker exec cte-distributed-node1 ls -la /mnt/hdd1
docker exec cte-distributed-node2 ls -la /mnt/hdd2

# Verify permissions
docker exec cte-distributed-node1 touch /mnt/hdd1/test_file
```

## Customization

### Changing Number of Nodes

To add or remove nodes:
1. Update `hostfile` with new node names
2. Add/remove node services in `docker-compose.yaml`
3. Update `cte_config.yaml` to add/remove storage targets
4. Adjust network IP addresses accordingly

### Changing Storage Capacity

Edit `cte_config.yaml` and modify the `capacity` field for each device.

### Using Different Storage Types

Edit `cte_config.yaml` and change `media_type` (e.g., "SSD", "NVMe") and adjust `manual_score` accordingly.

## Notes

- The configuration uses the default IOWarp runtime configuration (no custom chimaera config needed)
- All nodes share the same CTE configuration file mounted read-only
- Node 1 is responsible for creating the CTE instance using `launch_cte.sh`
- The test execution is sequential - Node 1 runs tests after ensuring all nodes are ready
- Containers remain running after tests complete to allow inspection (`tail -f /dev/null`)
