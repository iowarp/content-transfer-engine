@CLAUDE.md Use dockerfile expert agent. 

Under docker, build two dockerfiles: redis_bench.Dockerfile and wrp_cte_bench.Dockerfile.

Add both to the github actions for this container.

## redis_bench.Dockerfile

FROM iowarp/context-transfer-engine:latest

Launches the benchmark similar to benchmark/redis_bench.sh

## wrp_cte_bench.Dockerfile

FROM iowarp/context-transfer-engine:latest

Launches the benchmark similar to benchmark/wrp_cte_bench.sh. Should take as input environment variables for each of the script parameters.



## Compose files

Build example docker-compose files for both benchmarks.

### Redis

This one is easy. It should have every environment variable that the container uses.
Place under docker/redis_bench.

### WRP

This one is less easy. It has two parts: launching the runtime + CTE and then the benchmark.
Place this under docker/wrp_cte_bench. We should have one CTE configuration for both containers.

The first container to be aware of is iowarp/iowarp:latest. This one deploys iowarp with CTE. 
An example compose for this container is below:
```
services:
  iowarp:
    image: iowarp/iowarp:latest
    container_name: iowarp
    hostname: iowarp-node

    # Mount custom configuration
    volumes:
      - ./wrp_conf.yaml:/etc/iowarp/wrp_conf.yaml:ro

    # Expose ZeroMQ port
    ports:
      - "5555:5555"

    # Run as daemon with interactive terminal
    stdin_open: true
    tty: true

    shm_size: 8g
    mem_limit: 8g
```

The other container is the wrp_cte_bench container, which is defined in docker/wrp_cte_bench.Dockerfile.