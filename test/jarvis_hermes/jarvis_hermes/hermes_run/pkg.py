"""
This module provides classes and methods to launch the HermesRun service.
hrun is ....
"""

from jarvis_cd.basic.pkg import Service, Color
from jarvis_util import *
import threading
import time
import tempfile


class HermesRun(Service):
    """
    This class provides methods to launch the HermesRun service.
    """
    def _init(self):
        """
        Initialize paths
        """
        self.daemon_pkg = None
        self.hostfile_path = f'{self.shared_dir}/hostfile'
        self.device_scores = {}
        pass

    def _benchmark_device(self, mount_point, device_name, duration=10, block_size=1024*1024):
        """
        Benchmark a storage device with 1MB transfers for specified duration.

        :param mount_point: The mount point of the device
        :param device_name: The name of the device
        :param duration: Duration of benchmark in seconds
        :param block_size: Size of each transfer (default 1MB)
        :return: Throughput score (normalized 0.0-1.0)
        """
        try:
            self.log(f'Benchmarking device {device_name} at {mount_point}', Color.YELLOW)

            # Create test file path
            if mount_point == '' or mount_point.startswith('ram://'):
                # Skip RAM devices as they can't be benchmarked this way
                self.log(f'Skipping RAM device {device_name}', Color.YELLOW)
                self.device_scores[device_name] = 1.0  # Assume RAM is fastest
                return 1.0

            # Ensure mount point exists and is accessible
            test_dir = mount_point.replace('fs://', '')
            os.makedirs(test_dir, exist_ok=True)
            test_file = os.path.join(test_dir, f'benchmark_{device_name}.tmp')

            # Perform write benchmark
            data = b'0' * block_size
            write_start = time.time()
            bytes_written = 0

            with open(test_file, 'wb') as f:
                while time.time() - write_start < duration:
                    f.write(data)
                    f.flush()
                    os.fsync(f.fileno())
                    bytes_written += block_size

            write_duration = time.time() - write_start
            write_throughput = bytes_written / write_duration / (1024 * 1024)  # MB/s

            # Perform read benchmark
            read_start = time.time()
            bytes_read = 0

            with open(test_file, 'rb') as f:
                while time.time() - read_start < duration:
                    chunk = f.read(block_size)
                    if not chunk:
                        f.seek(0)
                        continue
                    bytes_read += len(chunk)

            read_duration = time.time() - read_start
            read_throughput = bytes_read / read_duration / (1024 * 1024)  # MB/s

            # Clean up
            try:
                os.remove(test_file)
            except:
                pass

            # Calculate average throughput
            avg_throughput = (write_throughput + read_throughput) / 2

            self.log(f'Device {device_name}: Write={write_throughput:.2f} MB/s, '
                    f'Read={read_throughput:.2f} MB/s, Avg={avg_throughput:.2f} MB/s',
                    Color.GREEN)

            # Store raw throughput for normalization later
            self.device_scores[device_name] = avg_throughput
            return avg_throughput

        except Exception as e:
            self.log(f'Benchmark failed for {device_name}: {str(e)}', Color.RED)
            self.device_scores[device_name] = 0.1  # Low default score
            return 0.1

    def _configure_menu(self):
        """
        Create a CLI menu for the configurator method.
        For thorough documentation of these parameters, view:
        https://github.com/scs-lab/jarvis-util/wiki/3.-Argument-Parsing

        :return: List(dict)
        """
        return [

            {
                'name': 'recency_max',
                'msg': 'time before blob is considered stale (sec)',
                'type': float,
                'default': 1,
                'class': 'buffer organizer',
                'rank': 1,
            },
            {
                'name': 'borg_min_cap',
                'msg': 'Capacity percentage before reorganizing can begin',
                'type': float,
                'default': 0,
                'class': 'buffer organizer',
                'rank': 1,
            },
            {
                'name': 'flush_period',
                'msg': 'Period of time to check for flushing (seconds)',
                'type': int,
                'default': 5,
                'class': 'buffer organizer',
                'rank': 1,
            },
            {
                'name': 'include',
                'msg': 'Specify paths to include',
                'type': list,
                'default': [],
                'class': 'adapter',
                'rank': 1,
                'args': [
                    {
                        'name': 'path',
                        'msg': 'The path to be included',
                        'type': str
                    },
                ],
                'aliases': ['i']
            },
            {
                'name': 'exclude',
                'msg': 'Specify paths to exclude',
                'type': list,
                'default': [],
                'class': 'adapter',
                'rank': 1,
                'args': [
                    {
                        'name': 'path',
                        'msg': 'The path to be excluded',
                        'type': str
                    },
                ],
                'aliases': ['e']
            },
            {
                'name': 'adapter_mode',
                'msg': 'The adapter mode to use for Hermes',
                'type': str,
                'default': 'default',
                'choices': ['default', 'scratch', 'bypass'],
                'class': 'adapter',
                'rank': 1,
            },
            {
                'name': 'flush_mode',
                'msg': 'The flushing mode to use for adapters',
                'type': str,
                'default': 'async',
                'choices': ['sync', 'async'],
                'class': 'adapter',
                'rank': 1,
            },
            {
                'name': 'log_verbosity',
                'msg': 'Verbosity of the output, 0 for fatal, 1 for info',
                'type': int,
                'default': '1',
            },
            {
                'name': 'page_size',
                'msg': 'The page size to use for adapters',
                'type': str,
                'default': '1m',
                'class': 'adapter',
                'rank': 1,
            },
            {
                'name': 'ram',
                'msg': 'Amount of RAM to use for buffering',
                'type': str,
                'default': '0',
                'class': 'dpe',
                'rank': 1,
            },
            {
                'name': 'dpe',
                'msg': 'The DPE to use by default',
                'type': str,
                'default': 'MinimizeIoTime',
                'class': 'dpe',
                'rank': 1,
            },
            {
                'name': 'devices',
                'msg': 'Search for a number of devices to include',
                'type': list,
                'default': [],
                'args': [
                    {
                        'name': 'mount',
                        'msg': 'The path to the device',
                        'type': str
                    },
                    {
                        'name': 'size',
                        'msg': 'The amount of data to use',
                        'type': str
                    }
                ],
                'class': 'dpe',
                'rank': 1,
            },
            {
                'name': 'do_benchmark',
                'msg': 'Run local benchmark on storage devices',
                'type': bool,
                'default': True,
                'class': 'benchmark',
                'rank': 1,
            }
        ]
    
    def _configure(self, **kwargs):
        """
        Converts the Jarvis configuration to application-specific configuration.
        E.g., OrangeFS produces an orangefs.xml file.

        :param config: The human-readable jarvis YAML configuration for the
        application.
        :return: None
        """
        rg = self.jarvis.resource_graph
        hermes_server = {
            'devices': {},
        }

        # Create hostfile
        self.hostfile = self.jarvis.hostfile
        self.env['HERMES_LOG_VERBOSITY'] = str(self.config['log_verbosity'])

        # Begin making Hermes client config
        hermes_client = {
            'path_inclusions': [''],
            'path_exclusions': ['/'],
            'file_page_size': self.config['page_size']
        }
        if self.config['flush_mode'] == 'async':
            hermes_client['flushing_mode'] = 'kAsync'
        elif self.config['flush_mode'] == 'sync':
            hermes_client['flushing_mode'] = 'kSync'
        if self.config['include'] is not None:
            hermes_client['path_inclusions'] += self.config['include']
        if self.config['exclude'] is not None:
            hermes_client['path_exclusions'] += self.config['exclude']

        # Get storage info
        devs = []
        if len(self.config['devices']) == 0:
            # Get all the fastest storage device mount points on machine
            dev_df = rg.find_storage(needs_root=False)
            devs = dev_df.rows
        else:
            # Get the storage devices for the user
            for dev in self.config['devices']:
                devs.append({
                    'mount': dev['mount'],
                    'avail': int(SizeConv.to_int(dev['size'])  * 1 / .9),
                    'shared': False,
                    'dev_type': 'custom'
                })
        if len(devs) == 0:
            raise Exception('Hermes needs at least one storage device')

        # Make a copy of devs before adding RAM
        devs_copy = list(devs)

        # Add RAM to the devices list if configured
        if 'ram' in self.config and self.config['ram'] != '0':
            devs_copy.append({
                'mount': '',
                'avail': SizeConv.to_int(self.config['ram']),
                'shared': False,
                'dev_type': 'ram'
            })

        # Build device_info list with all devices including RAM
        device_info = []
        for i, dev in enumerate(devs_copy):
            dev_type = dev['dev_type']
            custom_name = f'{dev_type}_{i}' if dev_type != 'ram' else 'ram'
            mount = os.path.expandvars(dev['mount'])
            if len(mount) == 0:
                device_info.append((custom_name, '', dev))
                continue
            mount = f'{mount}/hermes_data'
            device_info.append((custom_name, mount, dev))

        # Run benchmarks in parallel if enabled
        if self.config.get('do_benchmark', True):
            self.log('Starting parallel device benchmarks...', Color.YELLOW)
            benchmark_threads = []

            for custom_name, mount, dev in device_info:
                if len(mount) == 0:
                    # RAM device - assign highest score
                    self.device_scores[custom_name] = 1.0
                    continue

                # Create and start benchmark thread
                thread = threading.Thread(
                    target=self._benchmark_device,
                    args=(f'fs://{mount}', custom_name)
                )
                benchmark_threads.append(thread)
                thread.start()

            # Wait for all benchmarks to complete
            for thread in benchmark_threads:
                thread.join()

            # Normalize scores (0.0 to 0.9 based on max throughput, reserve 1.0 for RAM)
            if self.device_scores:
                max_throughput = max(self.device_scores.values())
                if max_throughput > 0:
                    for dev_name in self.device_scores:
                        if dev_name != 'ram':  # Don't normalize RAM
                            self.device_scores[dev_name] = 0.9 * (self.device_scores[dev_name] / max_throughput)

            self.log('Benchmarks completed', Color.GREEN)

            # Disable benchmark for future runs
            self.config['do_benchmark'] = False

        # Configure devices with scores
        self.config['borg_paths'] = []
        for custom_name, mount, dev in device_info:
            # Handle RAM device
            if len(mount) == 0:
                ram_config = {
                    'mount_point': 'ram://',
                    'capacity': self.config['ram'],
                    'block_size': '4kb',
                    'is_shared_device': False,
                    'borg_capacity_thresh': [self.config['borg_min_cap'], 1.0],
                    'slab_sizes': ['256', '512', '1KB',
                                   '4KB', '16KB', '64KB', '1MB']
                }
                # Add RAM score if benchmarked
                if custom_name in self.device_scores:
                    ram_config['score'] = self.device_scores[custom_name]
                hermes_server['devices'][custom_name] = ram_config
                continue

            # Handle filesystem devices
            device_config = {
                'mount_point': f'fs://{mount}',
                'capacity': int(.9 * float(dev['avail'])),
                'block_size': '4kb',
                'is_shared_device': dev['shared'],
                'borg_capacity_thresh': [0.0, 1.0],
                'slab_sizes': ['4KB', '16KB', '64KB', '1MB']
            }

            # Add score if available
            if custom_name in self.device_scores:
                device_config['score'] = self.device_scores[custom_name]

            hermes_server['devices'][custom_name] = device_config
            self.config['borg_paths'].append(mount)
            Mkdir(mount, PsshExecInfo(hostfile=self.hostfile,
                                      env=self.env))

        # Get network Info
        hermes_server['buffer_organizer'] = {
            'recency_max': self.config['recency_max'],
            'flush_period': self.config['flush_period']
        }
        hermes_server['default_placement_policy'] = self.config['dpe']
        if self.config['adapter_mode'] == 'default':
            adapter_mode = 'kDefault'
        elif self.config['adapter_mode'] == 'scratch':
            adapter_mode = 'kScratch'
        elif self.config['adapter_mode'] == 'bypass':
            adapter_mode = 'kBypass'
        self.env['HERMES_ADAPTER_MODE'] = adapter_mode
        hermes_server['default_placement_policy'] = self.config['dpe']

        # Save hermes configurations
        hermes_server_yaml = f'{self.shared_dir}/hermes_server.yaml'
        YamlFile(hermes_server_yaml).save(hermes_server)
        self.env['HERMES_CONF'] = hermes_server_yaml

        # Save Hermes client configurations
        hermes_client_yaml = f'{self.shared_dir}/hermes_client.yaml'
        YamlFile(hermes_client_yaml).save(hermes_client)
        self.env['HERMES_CLIENT_CONF'] = hermes_client_yaml

    def start(self):
        """
        Launch an application. E.g., OrangeFS will launch the servers, clients,
        and metadata services on all necessary pkgs.

        :return: None
        """
        self.log(self.env['HERMES_CONF'])
        self.log(self.env['HERMES_CLIENT_CONF'])
        pass

    def stop(self):
        """
        Stop a running application. E.g., OrangeFS will terminate the servers,
        clients, and metadata services.

        :return: None
        """
        pass

    def kill(self):
       pass

    def clean(self):
        """
        Destroy all data for an application. E.g., OrangeFS will delete all
        metadata and data directories in addition to the orangefs.xml file.

        :return: None
        """
        self.hostfile = self.jarvis.hostfile
        for path in self.config['borg_paths']:
            self.log(f'Removing {path}', Color.YELLOW)
            Rm(path, PsshExecInfo(hostfile=self.hostfile))

    def status(self):
        """
        Check whether or not an application is running. E.g., are OrangeFS
        servers running?

        :return: True or false
        """
        return True
