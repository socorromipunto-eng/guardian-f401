"""Configuration models for Guardian host transports."""

# Import dataclass for immutable connection configuration objects.
from dataclasses import dataclass

# Import shared M10 role and PSK validation.
from guardian_protocol import SecurityRole, validate_psk


# Define the simulator development host used by default.
DEFAULT_HOST = "127.0.0.1"

# Define the simulator development TCP port used by default.
DEFAULT_PORT = 9401

# Define a bounded default network or serial response timeout in seconds.
DEFAULT_TIMEOUT_SECONDS = 2.0

# Define the physical Guardian UART baud rate used by M4.
DEFAULT_SERIAL_BAUD = 115200


# Store immutable TCP guardianctl connection settings.
@dataclass(frozen=True, slots=True)
class ClientConfig:
    """TCP connection settings used by Guardian host operations."""

    # Store the Guardian TCP host name or IP address.
    host: str = DEFAULT_HOST

    # Store the Guardian TCP port.
    port: int = DEFAULT_PORT

    # Store the maximum connect and response wait in seconds.
    timeout_seconds: float = DEFAULT_TIMEOUT_SECONDS

    # Validate every externally supplied configuration field immediately.
    def __post_init__(self) -> None:

        # Reject empty host values because they have ambiguous semantics.
        if not self.host:

            # Raise a precise configuration diagnostic.
            raise ValueError("host cannot be empty")

        # Reject invalid TCP port values before socket creation.
        if not 1 <= self.port <= 65535:

            # Raise a precise configuration diagnostic.
            raise ValueError("port must be between 1 and 65535")

        # Reject non-positive timeouts because they cannot provide bounded I/O.
        if self.timeout_seconds <= 0.0:

            # Raise a precise configuration diagnostic.
            raise ValueError("timeout must be greater than zero")

    # Return the endpoint text used by operator-facing output.
    @property
    def endpoint(self) -> str:
        """Return the configured TCP endpoint."""

        # Render host and TCP port deterministically.
        return f"{self.host}:{self.port}"


# Store immutable physical UART guardianctl connection settings.
@dataclass(frozen=True, slots=True)
class SerialConfig:
    """Serial connection settings used by Guardian host operations."""

    # Store the operating-system serial port name such as COM5.
    port: str

    # Store the physical UART baud rate.
    baud_rate: int = DEFAULT_SERIAL_BAUD

    # Store the maximum serial response wait in seconds.
    timeout_seconds: float = DEFAULT_TIMEOUT_SECONDS

    # Validate every externally supplied serial configuration field immediately.
    def __post_init__(self) -> None:

        # Reject empty operating-system serial port names.
        if not self.port:

            # Raise a precise configuration diagnostic.
            raise ValueError("serial port cannot be empty")

        # Reject non-positive baud rates before serial configuration.
        if self.baud_rate <= 0:

            # Raise a precise configuration diagnostic.
            raise ValueError("baud rate must be greater than zero")

        # Reject non-positive timeouts because they cannot provide bounded I/O.
        if self.timeout_seconds <= 0.0:

            # Raise a precise configuration diagnostic.
            raise ValueError("timeout must be greater than zero")

    # Return the endpoint text used by operator-facing output.
    @property
    def endpoint(self) -> str:
        """Return the configured physical serial endpoint."""

        # Render port and baud rate deterministically.
        return f"{self.port}@{self.baud_rate}"


# Store immutable M10 host authentication configuration.
@dataclass(frozen=True, slots=True)
class SecurityClientConfig:
    """Guardian M10 PSK authentication settings."""

    # Store exactly one 256-bit pre-shared key.
    psk: bytes

    # Request OPERATOR authorization by default for M8/M9 state-changing commands.
    role: SecurityRole = SecurityRole.OPERATOR

    # Validate key width and role immediately.
    def __post_init__(self) -> None:

        # Normalize the PSK into immutable bytes.
        normalized_psk = validate_psk(
            self.psk
        )

        # Replace bytes-like input with validated immutable bytes.
        object.__setattr__(
            self,
            "psk",
            normalized_psk,
        )

        # Normalize the requested role.
        normalized_role = SecurityRole(
            self.role
        )

        # Reject the absence of an authentication role.
        if normalized_role == SecurityRole.NONE:

            # Require an actual authorization role.
            raise ValueError(
                "security role cannot be NONE"
            )

        # Preserve the normalized enum value.
        object.__setattr__(
            self,
            "role",
            normalized_role,
        )
