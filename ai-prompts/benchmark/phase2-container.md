@CLAUDE.md Use dockerfile expert agent. 

Under docker, build two dockerfiles: redis_bench.Dockerfile and wrp_cte_bench.Dockerfile.

Add both to the github actions for this container.

## redis_bench.Dockerfile

FROM iowarp/context-transfer-engine:latest

Launches the benchmark similar to benchmark/redis_bench.sh

## wrp_cte_bench.Dockerfile

FROM iowarp/context-transfer-engine:latest

Launches the benchmark similar to benchmark/wrp_cte_bench.sh. Should take as input environment variables for each of the script parameters.
