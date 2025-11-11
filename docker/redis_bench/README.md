# Redis Benchmark Docker Compose

This directory contains a Docker Compose configuration for running Redis benchmarks with CTE (Context Transfer Engine) integration.

## Overview

The Redis benchmark runs entirely in a single container with Redis server started internally. No external Redis installation or separate Redis container is needed.

## Quick Start

```bash
# Run with default settings (All tests, 4 clients, 1MB I/O, 10000 ops)
docker-compose up

# Run in background
docker-compose up -d

# View logs
docker-compose logs -f redis-bench
```

## Configuration

### Environment Variables

All benchmark parameters can be configured via environment variables:

| Variable | Description | Default | Examples |
|----------|-------------|---------|----------|
| `TEST_CASE` | Test to run: Set, Get, SetGet, All | `All` | Set, Get, SetGet |
| `NUM_CLIENTS` | Number of parallel Redis clients | `4` | 1, 4, 8, 16 |
| `IO_SIZE` | Size of each I/O operation | `1m` | 4k, 1m, 16m |
| `IO_COUNT` | Number of operations to perform | `10000` | 1000, 10000, 100000 |

### I/O Size Format

The `IO_SIZE` parameter supports these suffixes:
- `b` - bytes (e.g., `1024b`)
- `k` - kilobytes (e.g., `4k`)
- `m` - megabytes (e.g., `1m`)
- `g` - gigabytes (e.g., `2g`)

## Usage Examples

### Basic Usage

```bash
# Run Set benchmark with 8 clients
TEST_CASE=Set NUM_CLIENTS=8 docker-compose up

# Run Get benchmark with small I/O (4KB)
TEST_CASE=Get NUM_CLIENTS=16 IO_SIZE=4k IO_COUNT=100000 docker-compose up

# Run mixed SetGet benchmark
TEST_CASE=SetGet NUM_CLIENTS=4 IO_SIZE=1m IO_COUNT=10000 docker-compose up
```

### Performance Testing Scenarios

```bash
# High IOPS test - many small operations
TEST_CASE=All NUM_CLIENTS=16 IO_SIZE=4k IO_COUNT=1000000 docker-compose up

# High throughput test - large operations
TEST_CASE=All NUM_CLIENTS=2 IO_SIZE=16m IO_COUNT=1000 docker-compose up

# Write-heavy workload
TEST_CASE=Set NUM_CLIENTS=8 IO_SIZE=1m IO_COUNT=50000 docker-compose up

# Read-heavy workload
TEST_CASE=Get NUM_CLIENTS=8 IO_SIZE=1m IO_COUNT=50000 docker-compose up
```

### Using Alternative Service Configurations

The docker-compose.yml file includes commented-out service definitions for common test scenarios. Uncomment the desired service and run:

```bash
# Edit docker-compose.yml to uncomment the desired service, then:
docker-compose up redis-bench-set
docker-compose up redis-bench-get
docker-compose up redis-bench-large
docker-compose up redis-bench-small
```

## Test Cases

### All Tests
Runs Set, Get, and SetGet benchmarks sequentially.

```bash
TEST_CASE=All docker-compose up
```

### Set Test
Write-only benchmark - measures Redis SET operation performance.

```bash
TEST_CASE=Set docker-compose up
```

### Get Test
Read-only benchmark - measures Redis GET operation performance.

```bash
TEST_CASE=Get docker-compose up
```

### SetGet Test
Mixed read/write benchmark - alternates between SET and GET operations.

```bash
TEST_CASE=SetGet docker-compose up
```

## Resource Configuration

The default configuration allocates:
- **Memory limit**: 4GB
- No shared memory configuration needed (Redis manages its own memory)

For large I/O tests, increase memory limit by editing docker-compose.yml:

```yaml
mem_limit: 8g
```

## Troubleshooting

### Container Exits Immediately
Check logs to see benchmark results:
```bash
docker-compose logs redis-bench
```

### Out of Memory Errors
Increase memory limit in docker-compose.yml or reduce IO_SIZE/IO_COUNT.

### Performance Issues
- Reduce NUM_CLIENTS for large I/O operations
- Reduce IO_COUNT for initial testing
- Monitor system resources with `docker stats`

## Cleanup

```bash
# Stop and remove containers
docker-compose down

# Remove all stopped containers and networks
docker-compose down --remove-orphans
```

## Integration with CI/CD

Example GitHub Actions workflow:

```yaml
- name: Run Redis Benchmark
  run: |
    cd docker/redis_bench
    TEST_CASE=All NUM_CLIENTS=4 IO_SIZE=1m IO_COUNT=10000 docker-compose up
    docker-compose down
```

## Notes

- Redis server starts automatically inside the container
- Benchmark results are printed to stdout
- Container removes itself after completion (restart: "no")
- All data is in-memory (no persistent storage)
