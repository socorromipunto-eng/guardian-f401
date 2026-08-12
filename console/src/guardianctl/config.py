"""Configuration model for the Guardian host command-line client."""

# Import dataclass for an immutable client configuration object.
from dataclasses import dataclass


# Define the simulator development host used by default.
DEFAULT_HOST = "127.0.0.1"

# Define the simulator development port used by default.
DEFAULT_PORT = 9401

# Define a bounded default network timeout in seconds.
DEFAULT_TIMEOUT_SECONDS = 2.0


# Store immutable guardianctl connection settings.
@dataclass(frozen=True, slots=True)
class ClientConfig:
    """Connection settings used by Guardian host operations."""

    # Store the Guardian endpoint host name or IP address.
    host: str = DEFAULT_HOST

    # Store the Guardian endpoint TCP port.
    port: int = DEFAULT_PORT

    # Store the maximum connect/response wait in seconds.
    timeout_seconds: float = DEFAULT_TIMEOUT_SECONDS

    # Validate every externally supplied configuration field immediately.
    def __post_init__(self) -> None:

        # Reject empty host values because they have ambiguous connection semantics.
        if not self.host:

            # Raise a precise configuration diagnostic.
            raise ValueError("host cannot be empty")

        # Reject invalid TCP port values before socket creation.
        if not 1 <= self.port <= 65535:

            # Raise a precise configuration diagnostic.
            raise ValueError("port must be between 1 and 65535")

        # Reject non-positive timeouts because they cannot provide useful bounded I/O.
        if self.timeout_seconds <= 0.0:

            # Raise a precise configuration diagnostic.
            raise ValueError("timeout must be greater than zero")
