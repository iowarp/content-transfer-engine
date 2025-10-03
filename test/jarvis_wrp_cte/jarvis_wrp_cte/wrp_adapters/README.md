# WRP CTE Adapters Interceptor

The `wrp_adapters` package provides I/O interception capabilities for the IoWarp Content Transfer Engine (CTE). It enables transparent redirection of various I/O APIs to CTE for intelligent data placement and transfer management.

## Overview

The WRP Adapters interceptor works by injecting shared libraries via `LD_PRELOAD` (or HDF5 plugin mechanisms for VFD) that intercept I/O calls at the system level. This allows existing applications to benefit from CTE's features without requiring source code modifications.

## Supported Adapters

### POSIX Adapter (`posix`)
- **Library**: `libwrp_cte_posix.so`
- **Intercepts**: `read`, `write`, `open`, `close`, `lseek`, `pread`, `pwrite`, `stat`, and other POSIX I/O operations
- **Use Case**: Traditional file I/O operations, works with most applications

### MPI-IO Adapter (`mpiio`)
- **Library**: `libwrp_cte_mpiio.so`
- **Intercepts**: `MPI_File_*` operations (open, read, write, seek, etc.)
- **Use Case**: Parallel I/O in MPI applications
- **Note**: Requires MPI to be enabled during CTE build

### STDIO Adapter (`stdio`)
- **Library**: `libwrp_cte_stdio.so`
- **Intercepts**: `fread`, `fwrite`, `fopen`, `fclose`, `fseek`, `fprintf`, and other buffered I/O operations
- **Use Case**: Applications using C standard library I/O functions

### HDF5 VFD Adapter (`vfd`)
- **Library**: `libwrp_cte_vfd.so`
- **Mechanism**: HDF5 Virtual File Driver (not LD_PRELOAD)
- **Intercepts**: HDF5 file I/O operations
- **Use Case**: Scientific applications using HDF5 format
- **Configuration**: Sets `HDF5_PLUGIN_PATH` and `HDF5_DRIVER` environment variables

### NVIDIA GDS Adapter (`nvidia_gds`)
- **Library**: `libwrp_cte_nvidia_gds.so`
- **Intercepts**: NVIDIA GPUDirect Storage operations
- **Use Case**: GPU-accelerated I/O with direct GPU-storage communication
- **Requirements**: NVIDIA GPUs with GDS support, CUDA runtime

## Usage

### Basic Configuration

Add the interceptor to your Jarvis pipeline YAML file:

```yaml
name: my_pipeline
interceptors:
  - pkg_type: wrp_adapters
    pkg_name: cte_adapters
    posix: true          # Enable POSIX interception
    mpiio: false         # Disable MPI-IO
    stdio: false         # Disable STDIO
    vfd: false          # Disable HDF5 VFD
    nvidia_gds: false   # Disable NVIDIA GDS

pkgs:
  - pkg_type: my_application
    pkg_name: my_app
    interceptors: ["cte_adapters"]  # Apply the interceptor
    # ... other application configuration
```

### Multiple Adapters

You can enable multiple adapters simultaneously:

```yaml
interceptors:
  - pkg_type: wrp_adapters
    pkg_name: cte_adapters
    posix: true      # For general file I/O
    mpiio: true      # For MPI-IO operations
    stdio: true      # For buffered I/O
```

### Complete Example Pipeline

```yaml
name: ior_with_cte
interceptors:
  - pkg_type: wrp_cte
    pkg_name: cte_runtime
    devices:
      - ["/mnt/nvme", "1TB", 0.9]
      - ["/tmp/ram_cache", "8GB", 1.0]
    worker_count: 4

  - pkg_type: wrp_adapters
    pkg_name: cte_adapters
    mpiio: true      # IOR uses MPI-IO

pkgs:
  - pkg_type: builtin.ior
    pkg_name: benchmark
    interceptors: ["cte_adapters"]
    nprocs: 4
    block: "1G"
    transfer: "1M"
    api: "MPIIO"
```

## Command Line Usage

### Configure Interceptor

```bash
# Create pipeline
jarvis ppl create my_pipeline

# Add and configure WRP adapters interceptor
jarvis interceptor append wrp_adapters cte_adapters
jarvis interceptor conf cte_adapters posix=true mpiio=true

# Add application and link interceptor
jarvis pkg append my_application app
jarvis pkg conf app interceptors='["cte_adapters"]'

# Start pipeline (interceptor is applied automatically)
jarvis ppl start
```

### View Configuration

```bash
# View pipeline configuration
jarvis ppl print

# Output shows:
# Interceptors:
#   cte_adapters:
#     Type: wrp_adapters
#     Configuration:
#       posix: true
#       mpiio: true
#       stdio: false
#       vfd: false
#       nvidia_gds: false
```

## Technical Details

### Environment Modification

The interceptor modifies the execution environment in the following ways:

1. **LD_PRELOAD**: Adds adapter libraries to intercept I/O calls
   ```
   LD_PRELOAD=/path/to/libwrp_cte_posix.so:/path/to/libwrp_cte_mpiio.so
   ```

2. **HDF5 VFD** (when enabled): Configures HDF5 plugin path
   ```
   HDF5_PLUGIN_PATH=/path/to/lib
   HDF5_DRIVER=wrp_cte_vfd
   ```

3. **WRP_CTE_ROOT**: Set to CTE installation directory for all adapters

### Library Search

The interceptor automatically searches for adapter libraries in:
- Package-specific `LD_LIBRARY_PATH`
- System `LD_LIBRARY_PATH`
- Standard system library paths (`/usr/lib`, `/usr/local/lib`, etc.)

### Adapter Stacking

Multiple adapters can be used simultaneously. They are added to `LD_PRELOAD` in the order specified, with later adapters potentially overriding earlier ones for the same symbols.

## Troubleshooting

### Library Not Found

If you get an error like "Could not find wrp_cte_posix library":

1. Ensure CTE is built with the adapter enabled:
   ```bash
   cmake --preset=debug -DWRP_CTE_ENABLE_POSIX_ADAPTER=ON
   ```

2. Check installation:
   ```bash
   find /usr/local -name "libwrp_cte_*.so"
   ```

3. Add to `LD_LIBRARY_PATH`:
   ```bash
   export LD_LIBRARY_PATH=/path/to/cte/lib:$LD_LIBRARY_PATH
   ```

### No Adapter Selected

Error: "No WRP CTE adapter selected"

**Solution**: Enable at least one adapter in the configuration:
```bash
jarvis interceptor conf cte_adapters posix=true
```

### MPI-IO Not Available

If MPI-IO adapter is not found:
- MPI-IO adapter requires MPI support during build
- Enable with: `-DWRP_CTE_ENABLE_MPIIO_ADAPTER=ON`
- Ensure MPI is installed and findable by CMake

### HDF5 VFD Issues

If HDF5 applications don't use the VFD:
1. Check `HDF5_PLUGIN_PATH` is set correctly
2. Verify `HDF5_DRIVER=wrp_cte_vfd`
3. Ensure HDF5 is built with plugin support

## Build Requirements

To enable specific adapters, use these CMake options:

```bash
# Always built (core adapters)
-DBUILD_WRP_CTE=ON

# Optional adapters
-DWRP_CTE_ENABLE_STDIO_ADAPTER=ON
-DWRP_CTE_ENABLE_MPIIO_ADAPTER=ON
-DWRP_CTE_ENABLE_VFD=ON
-DWRP_CTE_ENABLE_NVIDIA_GDS_ADAPTER=ON
```

## Integration with CTE Runtime

The `wrp_adapters` interceptor should be used together with the `wrp_cte` service package:

1. **wrp_cte**: Configures CTE runtime (devices, workers, policies)
2. **wrp_adapters**: Enables I/O interception for applications

```yaml
interceptors:
  # Configure CTE runtime first
  - pkg_type: wrp_cte
    pkg_name: cte_runtime
    devices: [...]

  # Then enable adapters
  - pkg_type: wrp_adapters
    pkg_name: cte_adapters
    posix: true

pkgs:
  - pkg_type: my_app
    pkg_name: app
    interceptors: ["cte_adapters"]  # Only reference the adapter interceptor
```

## See Also

- [CTE Core Documentation](../../../../docs/cte/cte.md)
- [WRP CTE Service Package](../wrp_cte/README.md)
- [Jarvis-CD Package Development Guide](../../../../docs/jarvis/package_dev_guide.md)
- [IoWarp Project Documentation](../../../../README.md)
