"""User-facing exception hierarchy for guardianctl."""

# Import the published Guardian error registry for remote error decoding.
from guardian_protocol import ErrorCode


# Define the common base exception handled by the CLI boundary.
class GuardianCtlError(Exception):
    """Base class for expected guardianctl failures."""


# Represent failures while creating, sending or receiving a transport connection.
class TransportError(GuardianCtlError):
    """Raised when the Guardian transport cannot complete a request."""


# Represent missing or invalid M10 host security configuration.
class SecurityConfigurationError(GuardianCtlError):
    """Raised when a protected operation has no usable M10 credentials."""


# Represent protocol responses that violate host-side correlation expectations.
class ProtocolClientError(GuardianCtlError):
    """Raised when a decoded response violates the host protocol contract."""


# Represent an ERROR frame returned intentionally by the remote Guardian device.
class RemoteDeviceError(GuardianCtlError):
    """Raised when the device returns a Guardian Protocol ERROR frame."""

    # Preserve the command and wire error value for structured callers.
    def __init__(self, command: int, error_code: int) -> None:

        # Store the command associated with the remote failure.
        self.command = command

        # Store the raw one-byte error value returned by the device.
        self.error_code = error_code

        # Attempt to map the raw value into the published Guardian error registry.
        try:

            # Convert the raw error byte into a known protocol error.
            error_name = ErrorCode(error_code).name
        except ValueError:

            # Preserve unknown future error values without crashing diagnostics.
            error_name = f"UNKNOWN_ERROR_0x{error_code:02X}"

        # Initialize the base exception with a concise operator-facing message.
        super().__init__(
            f"device rejected command 0x{command:02X}: {error_name}"
        )
