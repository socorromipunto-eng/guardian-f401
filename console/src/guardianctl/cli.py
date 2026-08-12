"""Command-line interface for Guardian F401 host diagnostics."""

# Import argparse for a dependency-free professional CLI.
import argparse

# Import sys for explicit stdout/stderr routing.
import sys

# Import the high-level Guardian client.
from .client import GuardianClient

# Import immutable client connection configuration and defaults.
from .config import (
    DEFAULT_HOST,
    DEFAULT_PORT,
    DEFAULT_TIMEOUT_SECONDS,
    ClientConfig,
)

# Import the expected host-side exception boundary.
from .errors import GuardianCtlError

# Import text and JSON renderers.
from .presentation import (
    render_info_json,
    render_info_text,
    render_ping_json,
    render_ping_text,
    render_status_json,
    render_status_text,
)

# Import the default TCP development transport.
from .transport import GuardianTcpTransport


# Build the complete guardianctl command-line grammar.
def build_parser() -> argparse.ArgumentParser:
    """Return the configured guardianctl argument parser."""

    # Create the top-level parser with a concise project description.
    parser = argparse.ArgumentParser(
        prog="guardianctl",
        description="Guardian F401 host diagnostics and management console.",
    )

    # Allow the operator to select a different Guardian development endpoint.
    parser.add_argument(
        "--host",
        default=DEFAULT_HOST,
        help=f"Guardian host or IP address (default: {DEFAULT_HOST})",
    )

    # Allow the operator to select a different Guardian TCP development port.
    parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help=f"Guardian TCP port (default: {DEFAULT_PORT})",
    )

    # Allow the operator to bound connect and response latency explicitly.
    parser.add_argument(
        "--timeout",
        type=float,
        default=DEFAULT_TIMEOUT_SECONDS,
        help=(
            "connect/response timeout in seconds "
            f"(default: {DEFAULT_TIMEOUT_SECONDS})"
        ),
    )

    # Allow machine-readable output for scripts, CI and future tooling.
    parser.add_argument(
        "--json",
        action="store_true",
        help="emit machine-readable JSON output",
    )

    # Create the required command registry.
    subcommands = parser.add_subparsers(
        dest="command",
        required=True,
    )

    # Register the PING diagnostic command.
    subcommands.add_parser(
        "ping",
        help="verify Guardian connectivity and measure latency",
    )

    # Register the DEVICE_INFO diagnostic command.
    subcommands.add_parser(
        "info",
        help="read device model, firmware and identifier",
    )

    # Register the GET_STATUS diagnostic command.
    subcommands.add_parser(
        "status",
        help="read runtime state and protocol diagnostics",
    )

    # Return the complete command-line grammar.
    return parser


# Execute one parsed guardianctl command and return a process exit status.
def main(argv: list[str] | None = None) -> int:
    """Execute guardianctl and return a conventional process exit code."""

    # Build the deterministic command-line grammar.
    parser = build_parser()

    # Parse explicit test arguments or the current process command line.
    args = parser.parse_args(argv)

    # Validate connection parameters using the shared immutable configuration model.
    try:

        # Construct the host connection configuration.
        config = ClientConfig(
            host=args.host,
            port=args.port,
            timeout_seconds=args.timeout,
        )
    except ValueError as exc:

        # Print configuration errors to stderr for shell-friendly behavior.
        print(f"guardianctl: configuration error: {exc}", file=sys.stderr)

        # Return the conventional command-line usage/configuration failure status.
        return 2

    # Create the configured synchronous TCP transport.
    transport = GuardianTcpTransport(config)

    # Create the high-level typed Guardian client.
    client = GuardianClient(transport=transport)

    # Convert expected communication failures into concise CLI diagnostics.
    try:

        # Dispatch the PING command.
        if args.command == "ping":

            # Execute the typed high-level PING operation.
            result = client.ping()

            # Select JSON output when explicitly requested.
            if args.json:

                # Print the machine-readable PING representation.
                print(render_ping_json(result, config))
            else:

                # Print the human-readable PING representation.
                print(render_ping_text(result, config))

            # Report command success.
            return 0

        # Dispatch the device-information command.
        if args.command == "info":

            # Execute the typed high-level metadata operation.
            info = client.device_info()

            # Select JSON output when explicitly requested.
            if args.json:

                # Print the machine-readable metadata representation.
                print(render_info_json(info))
            else:

                # Print the human-readable metadata representation.
                print(render_info_text(info))

            # Report command success.
            return 0

        # Dispatch the runtime-status command.
        if args.command == "status":

            # Execute the typed high-level runtime-status operation.
            status = client.status()

            # Select JSON output when explicitly requested.
            if args.json:

                # Print the machine-readable status representation.
                print(render_status_json(status))
            else:

                # Print the human-readable status representation.
                print(render_status_text(status))

            # Report command success.
            return 0

        # Protect future edits from accidentally creating an unhandled parsed command.
        parser.error(f"unsupported command: {args.command}")

        # Satisfy static analysis even though parser.error always terminates execution.
        return 2

    except GuardianCtlError as exc:

        # Print expected communication/protocol failures to stderr only.
        print(f"guardianctl: {exc}", file=sys.stderr)

        # Return a non-zero operational failure status without a Python traceback.
        return 1


# Run the CLI when this module is executed directly.
if __name__ == "__main__":

    # Exit the process using the status returned by the CLI boundary.
    raise SystemExit(main())
