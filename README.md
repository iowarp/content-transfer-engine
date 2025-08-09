# The Content Transfer Engine: Hermes

The CTE is a heterogeneous-aware, multi-tiered, dynamic, and distributed I/O buffering system designed to accelerate I/O for HPC and data-intensive workloads.


[![Project Site](https://img.shields.io/badge/Project-Site-blue)](https://grc.iit.edu/research/projects/iowarp)
[![Documentation](https://img.shields.io/badge/Docs-Hub-green)](https://grc.iit.edu/docs/category/iowarp)
[![License](https://img.shields.io/badge/License-BSD%203--Clause-yellow.svg)](LICENSE)
![Build](https://github.com/HDFGroup/iowarp/workflows/GitHub%20Actions/badge.svg)
[![Coverage Status](https://coveralls.io/repos/github/HDFGroup/iowarp/badge.svg?branch=master)](https://coveralls.io/github/HDFGroup/iowarp?branch=master)

## Overview

iowarp provides a programmable buffering layer across memory/storage tiers and supports multiple I/O pathways via adapters. It integrates with HPC runtimes and workflows to improve throughput, latency, and predictability.


## Build instructions

Below is a condensed, task-focused version of the official guide. For the full walkthrough, see: [Building iowarp - Complete Guide](https://grc.iit.edu/docs/iowarp/building-iowarp)

### Dependencies

- C++17-capable compiler (GCC 9.4+ tested)
- Thallium (Mochi), yaml-cpp
- iowarp_shm (shared-memory utilities and config)
- HDF5 1.14.0 if building the HDF5 VFD
- MPI-IO tested with MPICH and OpenMPI

### Bootstrap Spack (recommended path)

1) Install Spack
```bash
cd ${HOME}
git clone https://github.com/spack/spack.git
cd spack
git checkout tags/v0.22.2
echo ". ${PWD}/share/spack/setup-env.sh" >> ~/.bashrc
source ~/.bashrc
```

2) Add the GRC Spack repo
```bash
cd ${HOME}
git clone https://github.com/grc-iit/grc-repo
spack repo add grc-repo
```

3) Discover externals and compilers (load site modules first, e.g., MPI/libfabric/UCX)
```bash
spack external find
spack external find python
spack compiler add
spack compiler list
```

### Install iowarp with Spack

1) Verify dependency resolutions
```bash
spack spec -I iowarp
```

2) Install (default)
```bash
spack install iowarp
```

3) Install with interceptors (MPI-IO + HDF5 VFD)
```bash
spack install iowarp+vfd+mpiio
```

4) Example: use libfabric with verbs
```bash
spack install iowarp ^libfabric fabrics=rxm,sockets,tcp,udp,verbs
```

5) Troubleshooting (commonly helpful commands)
- Environment not detected → ensure modules loaded, then:
    ```bash
    spack external find
    cat ~/.spack/packages.yaml
    ```
- Dependency conflicts → inspect and constrain:
    ```bash
    spack spec -I iowarp
    spack install iowarp ^mpich@3.3.2
    ```
- Compiler too old → add and select a newer GCC:
    ```bash
    spack compiler add
    spack install iowarp%gcc@9.4
    ```
- Fetch/download issues → check network/proxy; sometimes package URLs are outdated.
- Spack corruption → remove `~/.spack` and the cloned spack directory, then reinstall.
- Debug installs:
    ```bash
    spack -d install iowarp
    ```

### Build with CMake (developer flow)

This path speeds up iterative development compared to reinstalling via Spack.

1) Ensure dependencies are available (Spack is still used to provide deps)
```bash
spack load iowarp_shm
```

2) Optional: manage installs with SCSPKG
```bash
git clone https://github.com/grc-iit/scspkg.git
cd scspkg
pip install -e . --user
echo "module use $(scspkg module dir)" >> ~/.bashrc
scspkg init False
```

3) Optional: create a iowarp package namespace for local installs
```bash
scspkg create iowarp_run
module show iowarp_run         # view env
scspkg pkg src iowarp_run      # source dir (optional use)
scspkg pkg root iowarp_run     # install prefix
```

4) Configure, build, and install
```bash
mkdir -p build
cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$(scspkg pkg root iowarp_run)
make -j
make install
```

5) Enable interceptors at configure time (example)
```bash
cmake .. \
    -Diowarp_ENABLE_MPIIO_ADAPTER=ON \
    -Diowarp_ENABLE_VFD=ON \
    -DCMAKE_INSTALL_PREFIX=$(scspkg pkg root iowarp_run)
```

Tip: run `ccmake ..` (or `cmake-gui`) to browse available CMake options.

## Testing

- CTest unit tests (after building):

```bash
cd build
ctest -VV
```

- Jarvis pipelines for NVIDIA GDS (examples):
    - Pipelines live under `test/pipelines/nvidia_gds/`
    - Run with Jarvis:

```bash
jarvis ppl load yaml test/pipelines/nvidia_gds/<pipeline_yaml>
jarvis ppl run
```

## Development

- Linting: we follow the Google C++ Style Guide.
    - Run `make lint` (wraps `ci/lint.sh` which uses `cpplint`).
    - Install `cpplint` via `pip install cpplint` if needed.

## Contributing

We follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). Submit PRs with clear descriptions and tests when possible. The CI will validate style and builds.

## License

This project is licensed under the BSD-3-Clause License - see the [LICENSE](LICENSE) file for details.
