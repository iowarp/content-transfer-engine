import datetime
import enum
from typing import overload


class CteOp(enum.Enum):
    kPutBlob = 0

    kGetBlob = 1

    kDelBlob = 2

    kGetOrCreateTag = 3

    kDelTag = 4

    kGetTagSize = 5

class MemContext:
    def __init__(self) -> None: ...

class CteTelemetry:
    @overload
    def __init__(self) -> None: ...

    @overload
    def __init__(self, op: CteOp, off: int, size: int, blob_name: str, tag_name: str, mod_time: datetime.timedelta | float, read_time: datetime.timedelta | float, logical_time: int = 0) -> None: ...

class Client:
    @overload
    def __init__(self) -> None: ...

    @overload
    def __init__(self, arg: "chi::UniqueId", /) -> None: ...

    def PollTelemetryLog(self, mctx: MemContext, minimum_logical_time: int) -> list[CteTelemetry]:
        """Poll telemetry log with minimum logical time filter"""

def get_cte_client() -> Client:
    """Get the global CTE client instance"""

def chimaera_runtime_init() -> bool:
    """Initialize the Chimaera runtime"""

def chimaera_client_init() -> bool:
    """Initialize the Chimaera client"""

def initialize_cte(config_path: str = '') -> bool:
    """Initialize the CTE subsystem"""
