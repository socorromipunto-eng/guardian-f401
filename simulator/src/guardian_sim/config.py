"""Configuration model for the Guardian F401 software device simulator."""

# Import dataclass for a compact immutable simulator configuration.
from dataclasses import dataclass

# Import the shared M10 authorization role registry.
from guardian_protocol import SecurityRole


# Define the default TCP host used for local-only simulator access.
DEFAULT_HOST = "127.0.0.1"

# Define the default TCP port reserved by this project for local development.
DEFAULT_PORT = 9401

# Define the default simulated hardware model name.
DEFAULT_MODEL = "Guardian-F401-SIM"

# Define the simulated firmware major version.
DEFAULT_FIRMWARE_MAJOR = 0

# Define the simulated firmware minor version aligned with the original simulator contract.
DEFAULT_FIRMWARE_MINOR = 2

# Define the simulated firmware patch version.
DEFAULT_FIRMWARE_PATCH = 0

# Define a recognizable 32-bit identifier reserved for the default simulator instance.
DEFAULT_DEVICE_ID = 0xF4010001


# Define one intentionally public simulator-only M10 demonstration PSK.
DEFAULT_SECURITY_PSK_HEX = "00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f"

# Keep security disabled by default so M2-M9 compatibility demos remain unchanged.
DEFAULT_SECURITY_ENABLED = False

# Grant the demo key operator authority but not future administrative authority.
DEFAULT_SECURITY_MAX_ROLE = SecurityRole.OPERATOR

# Store immutable simulator identity and network configuration.
@dataclass(frozen=True, slots=True)
class SimulatorConfig:
    """Configuration used to create one Guardian simulator instance."""

    # Bind only to loopback by default so the simulator is not exposed to the LAN.
    host: str = DEFAULT_HOST

    # Listen on the project development port unless the caller selects another port.
    port: int = DEFAULT_PORT

    # Publish a model name that clearly distinguishes software simulation from hardware.
    model: str = DEFAULT_MODEL

    # Publish the simulated firmware major version.
    firmware_major: int = DEFAULT_FIRMWARE_MAJOR

    # Publish the simulated firmware minor version.
    firmware_minor: int = DEFAULT_FIRMWARE_MINOR

    # Publish the simulated firmware patch version.
    firmware_patch: int = DEFAULT_FIRMWARE_PATCH

    # Publish one stable unsigned device identifier.
    device_id: int = DEFAULT_DEVICE_ID

    # Require M10 authenticated wrapping for privileged commands when enabled.
    security_enabled: bool = DEFAULT_SECURITY_ENABLED

    # Store the simulator-only PSK as exactly 32 immutable bytes.
    security_psk: bytes = bytes.fromhex(DEFAULT_SECURITY_PSK_HEX)

    # Store the maximum role accepted for this simulator PSK.
    security_max_role: SecurityRole = DEFAULT_SECURITY_MAX_ROLE

    # Validate network, identity and security values immediately after construction.
    def __post_init__(self) -> None:

        # Reject empty bind addresses because their exposure semantics are ambiguous.
        if not self.host:

            # Raise a precise configuration error.
            raise ValueError("host cannot be empty")

        # Reject invalid TCP port values before socket creation.
        if not 0 <= self.port <= 65535:

            # Raise a precise configuration error.
            raise ValueError("port must be between 0 and 65535")

        # Reject empty model identities before protocol responses are generated.
        if not self.model:

            # Raise a precise configuration error.
            raise ValueError("model cannot be empty")

        # Reject device identifiers that cannot fit in the published payload field.
        if not 0 <= self.device_id <= 0xFFFFFFFF:

            # Raise a precise configuration error.
            raise ValueError("device_id must fit in an unsigned 32-bit field")

        # Require exactly one 256-bit demo/provisioned PSK.
        if len(self.security_psk) != 32:

            # Reject ambiguous security configuration.
            raise ValueError("security_psk must contain exactly 32 bytes")

        # Require a published non-zero authorization ceiling.
        if self.security_max_role not in (
            SecurityRole.OBSERVER,
            SecurityRole.OPERATOR,
            SecurityRole.ADMIN,
        ):

            # Reject undefined authorization policy.
            raise ValueError("security_max_role must be OBSERVER, OPERATOR or ADMIN")
