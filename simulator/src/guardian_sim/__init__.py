"""Guardian F401 software device simulator."""

# Re-export simulator configuration for tests and future host integration.
from .config import SimulatorConfig

# Re-export the transport-independent device application model.
from .device import GuardianDevice

# Re-export the reusable TCP simulator server.
from .server import GuardianTcpServer, run_server

# Re-export the M8 simulator health model for focused tests.
from .health import SimulatorHealthModel

# Re-export the M9 simulator control model for focused tests.
from .control import SimulatorControlModel

# Define the stable public simulator import surface explicitly.
__all__ = [
    "GuardianDevice",
    "GuardianTcpServer",
    "SimulatorConfig",
    "SimulatorHealthModel",
    "SimulatorControlModel",
    "run_server",
]
