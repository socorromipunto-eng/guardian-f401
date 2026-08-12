"""Guardian F401 host command-line client."""

# Re-export the typed high-level Guardian client.
from .client import GuardianClient, PingResult

# Re-export immutable TCP and serial configuration.
from .config import ClientConfig, SerialConfig

# Re-export expected host-side error types.
from .errors import (
    GuardianCtlError,
    ProtocolClientError,
    RemoteDeviceError,
    TransportError,
)

# Re-export physical UART transport.
from .serial_transport import GuardianSerialTransport

# Re-export M5 persistent telemetry streaming.
from .telemetry_client import TelemetryMonitor, TelemetryRecord

# Re-export request sequence allocator.
from .sequence import SequenceManager

# Re-export transport contract and TCP transport.
from .transport import ExchangeTransport, GuardianTcpTransport

# Define the stable public import surface.
__all__ = [
    "ClientConfig",
    "ExchangeTransport",
    "GuardianClient",
    "GuardianCtlError",
    "GuardianSerialTransport",
    "GuardianTcpTransport",
    "PingResult",
    "ProtocolClientError",
    "RemoteDeviceError",
    "SequenceManager",
    "TelemetryMonitor",
    "TelemetryRecord",
    "SerialConfig",
    "TransportError",
]
