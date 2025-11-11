# WRP CTE Jarvis Packages - Quick Start Guide

This directory contains Jarvis-CD packages for the IoWarp Content Transfer Engine (CTE).

## Available Packages

### 1. wrp_cte (Service)
Configures the CTE runtime environment including storage devices, worker threads, and data placement policies.

**Package Type**: `wrp_cte`
**Class**: `WrpCte`
**Location**: `jarvis_wrp_cte/wrp_cte/`

### 2. wrp_adapters (Interceptor)
Enables I/O interception for various APIs (POSIX, MPI-IO, STDIO, HDF5 VFD, NVIDIA GDS) to route I/O through CTE.

**Package Type**: `wrp_adapters`
**Class**: `WrpAdapters`
**Location**: `jarvis_wrp_cte/wrp_adapters/`

## Quick Start

### Register the Package Repository

```bash
# Add this repository to Jarvis
jarvis repo add /path/to/content-transfer-engine/test/jarvis_wrp_cte

# Verify packages are available
jarvis pkg list | grep wrp
```

### Option 1: Using YAML Pipeline

Create a pipeline file (e.g., `my_pipeline.yaml`):

```yaml
name: cte_test_pipeline
interceptors:
  # Configure CTE runtime
  - pkg_type: wrp_cte
    pkg_name: cte_runtime
    devices:
      - ["/tmp/cte_cache", "2GB", 1.0]
      - ["/mnt/nvme", "500GB", 0.8]
    worker_count: 4
    dpe_type: "round_robin"

  # Enable I/O adapters
  - pkg_type: wrp_adapters
    pkg_name: cte_adapters
    posix: true      # Enable POSIX interception
    mpiio: true      # Enable MPI-IO interception

pkgs:
  # Your application
  - pkg_type: builtin.ior
    pkg_name: benchmark
    interceptors: ["cte_adapters"]
    nprocs: 4
    api: "MPIIO"
    block: "1G"
```

Load and run:

```bash
jarvis ppl load yaml my_pipeline.yaml
jarvis ppl start
jarvis ppl status
jarvis ppl stop
```

### Option 2: Using Command Line

```bash
# Create pipeline
jarvis ppl create my_pipeline

# Add CTE runtime interceptor
jarvis interceptor append wrp_cte cte_runtime
jarvis interceptor conf cte_runtime \
  devices='[["/tmp/cache","2GB",1.0],["/mnt/nvme","500GB",0.8]]' \
  worker_count=4 \
  dpe_type="round_robin"

# Add adapters interceptor
jarvis interceptor append wrp_adapters cte_adapters
jarvis interceptor conf cte_adapters posix=true mpiio=true

# Add your application
jarvis pkg append builtin.ior benchmark
jarvis pkg conf benchmark \
  interceptors='["cte_adapters"]' \
  nprocs=4 \
  api="MPIIO" \
  block="1G"

# Start pipeline
jarvis ppl start

# Check status
jarvis ppl status

# Stop pipeline
jarvis ppl stop
```

## Example Pipelines

Pre-configured example pipelines are available in the `pipelines/` directory:

1. **example_posix_adapter.yaml** - Basic POSIX adapter configuration
2. **example_mpiio_adapter.yaml** - MPI-IO with IOR benchmark
3. **example_multi_adapter.yaml** - Multiple adapters simultaneously

```bash
# Run an example
jarvis ppl load yaml pipelines/example_mpiio_adapter.yaml
jarvis ppl start
```

## Configuration Options

### wrp_cte (Runtime)

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `devices` | list | [] | Storage devices as `(path, capacity, score)` tuples |
| `worker_count` | int | 4 | Number of worker threads |
| `dpe_type` | str | "round_robin" | Data placement engine type |
| `enable_prefetching` | bool | false | Enable data prefetching |
| `prefetch_distance` | str | "10MB" | Prefetch distance |

### wrp_adapters (Interceptor)

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `posix` | bool | false | Enable POSIX adapter |
| `mpiio` | bool | false | Enable MPI-IO adapter |
| `stdio` | bool | false | Enable STDIO adapter |
| `vfd` | bool | false | Enable HDF5 VFD adapter |
| `nvidia_gds` | bool | false | Enable NVIDIA GDS adapter |

## Verification

### Check Package Registration

```bash
# List all packages
jarvis pkg list

# Should show:
# wrp_cte
# wrp_adapters
```

### Check Interceptor Configuration

```bash
# View pipeline configuration
jarvis ppl print

# Should show interceptors section with:
# - wrp_cte runtime configuration
# - wrp_adapters with enabled adapters
```

### Verify Library Loading

```bash
# Check if adapter libraries are found
jarvis interceptor conf cte_adapters posix=true

# Should log:
# Found libwrp_cte_posix.so at /path/to/lib/libwrp_cte_posix.so
```

## Troubleshooting

### Package Not Found

```bash
# Check repository registration
jarvis repo list

# If not registered, add it
jarvis repo add /path/to/content-transfer-engine/test/jarvis_wrp_cte
```

### Adapter Library Not Found

```bash
# Check if CTE is built with adapters
ls /usr/local/lib/libwrp_cte_*.so

# If missing, rebuild with adapters enabled
cd /path/to/content-transfer-engine
cmake --preset=debug \
  -DWRP_CTE_ENABLE_MPIIO_ADAPTER=ON \
  -DWRP_CTE_ENABLE_STDIO_ADAPTER=ON
cmake --build build
cmake --install build
```

### Class Name Error

Error: "Package class name must follow UpperCamelCase"

This is a jarvis-cd requirement. The packages are correctly named:
- `wrp_cte` → class `WrpCte`
- `wrp_adapters` → class `WrpAdapters`

## Documentation

- **WRP CTE Package**: `jarvis_wrp_cte/wrp_cte/README.md`
- **WRP Adapters Package**: `jarvis_wrp_cte/wrp_adapters/README.md`
- **CTE Core Documentation**: `docs/cte/cte.md`
- **Jarvis Package Development**: `docs/jarvis/package_dev_guide.md`

## Support

For issues or questions:
1. Check the package README files
2. Review example pipelines
3. See the main CTE documentation
4. File an issue on the IoWarp GitHub repository

## Next Steps

1. **Explore Examples**: Run the example pipelines to see CTE in action
2. **Customize Configuration**: Adjust device configuration for your system
3. **Test with Applications**: Use CTE with your own I/O workloads
4. **Monitor Performance**: Use Jarvis status commands to track performance
5. **Experiment with Policies**: Try different data placement engines

Happy testing with IoWarp CTE!
