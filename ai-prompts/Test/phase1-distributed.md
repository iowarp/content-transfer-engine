@CLAUDE.md Make a distributed, containerized unit test for the content transfer engine. The test should have 4 nodes and should be defined under test/unit/distributed.
1. Create a cte configuration file. Let's have 4 directories: ${HOME}/hdd1:/mnt/hdd1, ${HOME}/hdd2:/mnt/hdd2/, etc. These will be the targets for the CTE. We will have to mount these as volumes. The configuration should be stored in test/unit/distributed and should be fixed. It should never have to change. We can just use the default iowarp runtime configuration, so no need for a chimaera config as well.
2. Launch the iowarp-runtime on each container
3. In the first container, create the cte using the utility script launch_cte.
4. Then, also first container, launch the unit tests for core functionality.

Below is an example docker compose from the iowarp runtime for its unit tests. We should augment to do ``spack load iowarp-runtime`` and to build content-transfer-engine.
```
services:
  # Node 1
  iowarp-node1:
    image: iowarp/iowarp:latest
    container_name: iowarp-distributed-node1
    hostname: iowarp-node1
    networks:
      iowarp-cluster:
        ipv4_address: 172.25.0.10
    volumes:
      - ~/.ppi-jarvis:/root/.ppi-jarvis
      - ../../../:/iowarp-runtime
      - ./hostfile:/etc/iowarp/hostfile:ro
      - ./chimaera_distributed.yaml:/etc/iowarp/chimaera_distributed.yaml:ro
      - iowarp-install:/usr/local
    environment:
      - NODE_ID=1
      - NODE_IP=172.25.0.10
      - CONTAINER_HOSTFILE=/etc/iowarp/hostfile
    shm_size: '16gb'
    mem_limit: 16g
    working_dir: /iowarp-runtime
    entrypoint: [ "/bin/bash", "-c" ]
    command: >
      "
        echo 'Node 1: Cleaning old build directory...' &&
        cd /iowarp-runtime &&
        rm -rf build-docker &&
        echo 'Node 1: Loading spack environment...' &&
        export SPACK_ROOT=/root/spack &&
        source /root/spack/share/spack/setup-env.sh &&
        spack load cte-hermes-shm &&
        echo 'Node 1: Spack environment loaded' &&
        echo 'Node 1: Building IOWarp runtime...' &&
        mkdir -p build-docker && cd build-docker &&
        echo 'Node 1: Running cmake...' &&
        cmake --preset docker .. &&
        echo 'Node 1: CMake complete. Building runtime and tests...' &&
        cmake --build . -j8 &&
        echo 'Node 1: Build complete. Installing...' &&
        cmake --install . &&
        echo 'Node 1: Install complete. Starting runtime...' &&
        export PATH=/usr/local/bin:$PATH &&
        WRP_RUNTIME_CONF=/etc/iowarp/chimaera_distributed.yaml chimaera_start_runtime &
        RUNTIME_PID=\$! &&
        echo \"Node 1: Runtime started (PID \$RUNTIME_PID). Ready for test execution.\" &&
        tail -f /dev/null
      "

  # Node 2
  iowarp-node2:
    image: iowarp/iowarp:latest
    container_name: iowarp-distributed-node2
    hostname: iowarp-node2
    networks:
      iowarp-cluster:
        ipv4_address: 172.25.0.11
    volumes:
      - ~/.ppi-jarvis:/root/.ppi-jarvis
      - ../../../:/iowarp-runtime
      - ./hostfile:/etc/iowarp/hostfile:ro
      - ./chimaera_distributed.yaml:/etc/iowarp/chimaera_distributed.yaml:ro
      - iowarp-install:/usr/local
    environment:
      - NODE_ID=2
      - NODE_IP=172.25.0.11
      - CONTAINER_HOSTFILE=/etc/iowarp/hostfile
    shm_size: '16gb'
    mem_limit: 16g
    working_dir: /iowarp-runtime
    entrypoint: [ "/bin/bash", "-c" ]
    command: >
      "
        echo 'Node 2: Waiting for build to complete...' &&
        while [ ! -f /usr/local/bin/chimaera_start_runtime ]; do
          sleep 2
          echo 'Node 2: Still waiting for binaries...'
        done &&
        echo 'Node 2: Binaries found. Loading spack environment...' &&
        export SPACK_ROOT=/root/spack &&
        source /root/spack/share/spack/setup-env.sh &&
        spack load cte-hermes-shm &&
        echo 'Node 2: Spack environment loaded' &&
        echo 'Node 2: Starting runtime...' &&
        export PATH=/usr/local/bin:$PATH &&
        WRP_RUNTIME_CONF=/etc/iowarp/chimaera_distributed.yaml chimaera_start_runtime &
        RUNTIME_PID=\$! &&
        echo \"Node 2: Runtime started (PID \$RUNTIME_PID). Waiting for tests...\" &&
        tail -f /dev/null
      "

  # Node 3
  iowarp-node3:
    image: iowarp/iowarp:latest
    container_name: iowarp-distributed-node3
    hostname: iowarp-node3
    networks:
      iowarp-cluster:
        ipv4_address: 172.25.0.12
    volumes:
      - ~/.ppi-jarvis:/root/.ppi-jarvis
      - ../../../:/iowarp-runtime
      - ./hostfile:/etc/iowarp/hostfile:ro
      - ./chimaera_distributed.yaml:/etc/iowarp/chimaera_distributed.yaml:ro
      - iowarp-install:/usr/local
    environment:
      - NODE_ID=3
      - NODE_IP=172.25.0.12
      - CONTAINER_HOSTFILE=/etc/iowarp/hostfile
    shm_size: '16gb'
    mem_limit: 16g
    working_dir: /iowarp-runtime
    entrypoint: [ "/bin/bash", "-c" ]
    command: >
      "
        echo 'Node 3: Waiting for build to complete...' &&
        while [ ! -f /usr/local/bin/chimaera_start_runtime ]; do
          sleep 2
          echo 'Node 3: Still waiting for binaries...'
        done &&
        echo 'Node 3: Binaries found. Loading spack environment...' &&
        export SPACK_ROOT=/root/spack &&
        source /root/spack/share/spack/setup-env.sh &&
        spack load cte-hermes-shm &&
        echo 'Node 3: Spack environment loaded' &&
        echo 'Node 3: Starting runtime...' &&
        export PATH=/usr/local/bin:$PATH &&
        WRP_RUNTIME_CONF=/etc/iowarp/chimaera_distributed.yaml chimaera_start_runtime &
        RUNTIME_PID=\$! &&
        echo \"Node 3: Runtime started (PID \$RUNTIME_PID). Waiting for tests...\" &&
        tail -f /dev/null
      "

  # Node 4
  iowarp-node4:
    image: iowarp/iowarp:latest
    container_name: iowarp-distributed-node4
    hostname: iowarp-node4
    networks:
      iowarp-cluster:
        ipv4_address: 172.25.0.13
    volumes:
      - ~/.ppi-jarvis:/root/.ppi-jarvis
      - ../../../:/iowarp-runtime
      - ./hostfile:/etc/iowarp/hostfile:ro
      - ./chimaera_distributed.yaml:/etc/iowarp/chimaera_distributed.yaml:ro
      - iowarp-install:/usr/local
    environment:
      - NODE_ID=4
      - NODE_IP=172.25.0.13
      - CONTAINER_HOSTFILE=/etc/iowarp/hostfile
    shm_size: '16gb'
    mem_limit: 16g
    working_dir: /iowarp-runtime
    entrypoint: [ "/bin/bash", "-c" ]
    command: >
      "
        echo 'Node 4: Waiting for build to complete...' &&
        while [ ! -f /usr/local/bin/chimaera_start_runtime ]; do
          sleep 2
          echo 'Node 4: Still waiting for binaries...'
        done &&
        echo 'Node 4: Binaries found. Loading spack environment...' &&
        export SPACK_ROOT=/root/spack &&
        source /root/spack/share/spack/setup-env.sh &&
        spack load cte-hermes-shm &&
        echo 'Node 4: Spack environment loaded' &&
        echo 'Node 4: Starting runtime...' &&
        export PATH=/usr/local/bin:$PATH &&
        WRP_RUNTIME_CONF=/etc/iowarp/chimaera_distributed.yaml chimaera_start_runtime &
        RUNTIME_PID=\$! &&
        echo \"Node 4: Runtime started (PID \$RUNTIME_PID). Waiting for tests...\" &&
        tail -f /dev/null
      "

volumes:
  iowarp-install:
    driver: local

networks:
  iowarp-cluster:
    driver: bridge
    ipam:
      config:
        - subnet: 172.25.0.0/16
```