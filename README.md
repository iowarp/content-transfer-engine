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

### Dependencies

Our docker container has all dependencies installed for you.
```bash
docker pull iowarp/iowarp-build:latest
```

### Build with CMake

```bash
mkdir -p build
cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release 
make -j8
make install
```

Tip: run `ccmake ..` (or `cmake-gui`) to browse available CMake options.

## Testing

- CTest unit tests (after building):

```bash
cd build
ctest -VV
```

## Development

- Linting: we follow the Google C++ Style Guide.
    - Run `make lint` (wraps `ci/lint.sh` which uses `cpplint`).
    - Install `cpplint` via `pip install cpplint` if needed.

## Contributing

We follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). Submit PRs with clear descriptions and tests when possible. The CI will validate style and builds.

## License

This project is licensed under the BSD-3-Clause License - see the [LICENSE](LICENSE) file for details.
