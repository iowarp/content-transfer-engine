"""
This module provides classes and methods to launch the HermesViz service.
hrun is ....
"""

from jarvis_cd.basic.pkg import Service, Color
from jarvis_util import *
import time


class HermesViz(Service):
    """
    This class provides methods to launch the HermesViz service.
    """
    def _init(self):
        """
        Initialize paths
        """
        self.daemon_pkg = None
        self.hostfile_path = f'{self.shared_dir}/hostfile'
        pass

    def _configure_menu(self):
        """
        Create a CLI menu for the configurator method.
        For thorough documentation of these parameters, view:
        https://github.com/scs-lab/jarvis-util/wiki/3.-Argument-Parsing

        :return: List(dict)
        """
        return [
            {
                'name': 'port',
                'msg': 'Port of the server',
                'type': int,
                'default': 4280,
            },
            {
                'name': 'refresh',
                'msg': 'How frequently to poll (seconds)',
                'type': float,
                'default': 5,
            },
        ]
    
    def _configure(self, **kwargs):
        """
        Converts the Jarvis configuration to application-specific configuration.
        E.g., OrangeFS produces an orangefs.xml file.

        :param config: The human-readable jarvis YAML configuration for the
        application.
        :return: None
        """
        pass

    def start(self):
        """
        Launch an application. E.g., OrangeFS will launch the servers, clients,
        and metadata services on all necessary pkgs.

        :return: None
        """
        self.log(f'Starting HermesViz on port {self.config["port"]}', Color.YELLOW)
        out = Exec(f'which python', 
             LocalExecInfo(env=self.env, collect_output=True))
        self.log(f'Python path: {out.stdout}', Color.YELLOW)
        Exec(f'hermes_viz_server {self.config["port"]} {self.config["refresh"]}', 
             LocalExecInfo(env=self.env,
                           do_dbg=self.config['do_dbg'],
                           dbg_port=self.config['dbg_port'],
                           exec_async=True))
        time.sleep(self.config['sleep'])

    def stop(self):
        """
        Stop a running application. E.g., OrangeFS will terminate the servers,
        clients, and metadata services.

        :return: None
        """
        self.kill()

    def kill(self):
       Kill('.*hermes_viz_server.*',
            LocalExecInfo(env=self.env))

    def clean(self):
        """
        Destroy all data for an application. E.g., OrangeFS will delete all
        metadata and data directories in addition to the orangefs.xml file.

        :return: None
        """
        pass

    def status(self):
        """
        Check whether or not an application is running. E.g., are OrangeFS
        servers running?

        :return: True or false
        """
        return True
