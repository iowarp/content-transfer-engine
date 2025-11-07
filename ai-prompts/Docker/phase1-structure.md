@CLAUDE.md Add a dockerfile called build.Dockerfile and deploy.Dockerfile.

build.Dockerfile will build the CTE using the cmake preset release and install it.

deploy.Dockerfile will inherit from the build dockerfile and call launch_cte using the local query.

Add a github action that will build build.Dockerfile as iowarp/context-transfer-engine-build:latest and deploy.Dockerfile as iowarp/context-transfer-engine-build:latest.

Implement an example docker compose for launching the CTE on a single node. This compose file should
take as input a configuration file and copy to the container or mount as a volume. Either way.