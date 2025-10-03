"""
This module provides classes and methods to inject WRP (IoWarp) CTE adapters.
The WrpAdapters interceptor enables interception of various I/O APIs (POSIX,
MPI-IO, STDIO, HDF5 VFD, NVIDIA GDS) and routes them to the Content Transfer
Engine for intelligent data placement and transfer.
"""
from jarvis_cd.core.pkg import Interceptor
import pathlib
import os


class WrpAdapters(Interceptor):
    """
    WRP CTE Adapters Interceptor.

    This interceptor enables I/O interception for various APIs by setting up
    LD_PRELOAD with the appropriate WRP CTE adapter libraries. It supports:
    - POSIX I/O (read, write, open, close, etc.)
    - MPI-IO (MPI_File_* operations)
    - STDIO (fread, fwrite, fopen, etc.)
    - HDF5 VFD (Virtual File Driver for HDF5)
    - NVIDIA GDS (GPUDirect Storage)
    """

    def _init(self):
        """
        Initialize the interceptor.

        This method is called during package instantiation.
        No configuration is available yet at this stage.
        """
        pass

    def _configure_menu(self):
        """
        Define configuration options for the WRP adapters interceptor.

        Returns:
            List[Dict]: Configuration parameters for adapter selection.
        """
        return [
            {
                'name': 'posix',
                'msg': 'Intercept POSIX I/O operations',
                'type': bool,
                'default': False,
                'help': 'Intercepts read, write, open, close, lseek, etc.'
            },
            {
                'name': 'mpiio',
                'msg': 'Intercept MPI-IO operations',
                'type': bool,
                'default': False,
                'help': 'Intercepts MPI_File_* operations'
            },
            {
                'name': 'stdio',
                'msg': 'Intercept STDIO operations',
                'type': bool,
                'default': False,
                'help': 'Intercepts fread, fwrite, fopen, fclose, etc.'
            },
            {
                'name': 'vfd',
                'msg': 'Intercept HDF5 I/O via VFD',
                'type': bool,
                'default': False,
                'help': 'Enables HDF5 Virtual File Driver for CTE'
            },
            {
                'name': 'nvidia_gds',
                'msg': 'Intercept NVIDIA GDS I/O',
                'type': bool,
                'default': False,
                'help': 'Intercepts NVIDIA GPUDirect Storage operations'
            },
        ]

    def _configure(self, **kwargs):
        """
        Configure the WRP adapters interceptor.

        This method finds the adapter libraries and stores their paths
        in the environment for use during modify_env().

        Args:
            **kwargs: Configuration parameters (automatically updated to self.config)

        Raises:
            Exception: If no adapter is selected or if a selected adapter library
                      cannot be found.
        """
        has_one = False

        if self.config['posix']:
            posix_lib = self.find_library('wrp_cte_posix')
            if posix_lib is None:
                raise Exception('Could not find wrp_cte_posix library')
            self.env['WRP_CTE_POSIX'] = posix_lib
            self.env['WRP_CTE_ROOT'] = str(pathlib.Path(posix_lib).parent.parent)
            self.log(f'Found libwrp_cte_posix.so at {posix_lib}')
            has_one = True

        if self.config['mpiio']:
            mpiio_lib = self.find_library('wrp_cte_mpiio')
            if mpiio_lib is None:
                raise Exception('Could not find wrp_cte_mpiio library')
            self.env['WRP_CTE_MPIIO'] = mpiio_lib
            self.env['WRP_CTE_ROOT'] = str(pathlib.Path(mpiio_lib).parent.parent)
            self.log(f'Found libwrp_cte_mpiio.so at {mpiio_lib}')
            has_one = True

        if self.config['stdio']:
            stdio_lib = self.find_library('wrp_cte_stdio')
            if stdio_lib is None:
                raise Exception('Could not find wrp_cte_stdio library')
            self.env['WRP_CTE_STDIO'] = stdio_lib
            self.env['WRP_CTE_ROOT'] = str(pathlib.Path(stdio_lib).parent.parent)
            self.log(f'Found libwrp_cte_stdio.so at {stdio_lib}')
            has_one = True

        if self.config['vfd']:
            vfd_lib = self.find_library('wrp_cte_vfd')
            if vfd_lib is None:
                raise Exception('Could not find wrp_cte_vfd library')
            self.env['WRP_CTE_VFD'] = vfd_lib
            self.env['WRP_CTE_ROOT'] = str(pathlib.Path(vfd_lib).parent.parent)
            self.log(f'Found libwrp_cte_vfd.so at {vfd_lib}')
            has_one = True

        if self.config['nvidia_gds']:
            nvidia_gds_lib = self.find_library('wrp_cte_nvidia_gds')
            if nvidia_gds_lib is None:
                raise Exception('Could not find wrp_cte_nvidia_gds library')
            self.env['WRP_CTE_NVIDIA_GDS'] = nvidia_gds_lib
            self.env['WRP_CTE_ROOT'] = str(pathlib.Path(nvidia_gds_lib).parent.parent)
            self.log(f'Found libwrp_cte_nvidia_gds.so at {nvidia_gds_lib}')
            has_one = True

        if not has_one:
            raise Exception('No WRP CTE adapter selected. Please enable at least one adapter (posix, mpiio, stdio, vfd, or nvidia_gds).')

    def modify_env(self):
        """
        Modify the environment to enable WRP CTE adapter interception.

        This method is called automatically by Jarvis during pipeline start,
        just before the target package's start() method. It modifies the shared
        mod_env to add adapter libraries to LD_PRELOAD.

        The mod_env is shared between the interceptor and the target package,
        so changes made here directly affect the package's execution environment.
        """
        # Add POSIX adapter to LD_PRELOAD
        if self.config['posix']:
            self._add_to_preload(self.env['WRP_CTE_POSIX'])
            self.log(f"Added POSIX adapter to LD_PRELOAD")

        # Add MPI-IO adapter to LD_PRELOAD
        if self.config['mpiio']:
            self._add_to_preload(self.env['WRP_CTE_MPIIO'])
            self.log(f"Added MPI-IO adapter to LD_PRELOAD")

        # Add STDIO adapter to LD_PRELOAD
        if self.config['stdio']:
            self._add_to_preload(self.env['WRP_CTE_STDIO'])
            self.log(f"Added STDIO adapter to LD_PRELOAD")

        # Configure HDF5 VFD (uses plugin path instead of LD_PRELOAD)
        if self.config['vfd']:
            plugin_path_parent = str(pathlib.Path(self.env['WRP_CTE_VFD']).parent)
            self.setenv('HDF5_PLUGIN_PATH', plugin_path_parent)
            self.setenv('HDF5_DRIVER', 'wrp_cte_vfd')
            self.log(f"Configured HDF5 VFD with plugin path: {plugin_path_parent}")

        # Add NVIDIA GDS adapter to LD_PRELOAD
        if self.config['nvidia_gds']:
            self._add_to_preload(self.env['WRP_CTE_NVIDIA_GDS'])
            self.log(f"Added NVIDIA GDS adapter to LD_PRELOAD")

    def _add_to_preload(self, library_path):
        """
        Helper method to add a library to LD_PRELOAD safely.

        This method checks if the library is already in LD_PRELOAD before adding
        it, and properly handles the case where LD_PRELOAD is empty.

        Args:
            library_path (str): Path to the library to add to LD_PRELOAD
        """
        current_preload = self.mod_env.get('LD_PRELOAD', '')

        # Check if library is already in preload to avoid duplicates
        if library_path in current_preload.split(':'):
            return

        # Add to existing LD_PRELOAD or create new one
        if current_preload:
            new_preload = f"{library_path}:{current_preload}"
        else:
            new_preload = library_path

        self.setenv('LD_PRELOAD', new_preload)

    def clean(self):
        """
        Clean up interceptor data.

        WRP adapters typically don't create persistent data, but this method
        can be extended if needed for cleanup operations.
        """
        pass
