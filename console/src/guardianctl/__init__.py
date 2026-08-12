"""Guardian F401 host command-line client."""

# Re-export the typed high-level Guardian client.
from .client import GuardianClient, PingResult

# Re-export immutable connection configuration.
from .config import ClientConfig

# Re-export expected host-side error types for future integrations.
from .errors import (
    GuardianCtlError,
    ProtocolClientError,
    RemoteDeviceError,
    TransportError,
)

# Re-export the thread-safe request sequence allocator.
from .sequence import SequenceManager

# Re-export the synchronous TCP development transport.
from .transport import GuardianTcpTransport

# Define the stable public host-client import surface explicitly.
__all__ = [
    "ClientConfig",
    "GuardianClient",
    "GuardianCtlError",
    "GuardianTcpTransport",
    "PingResult",
    "ProtocolClientError",
    "RemoteDeviceError",
    "SequenceManager",
    "TransportError",
]
