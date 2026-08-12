"""Command-line interface for Guardian F401 host diagnostics."""

# Import argparse for the dependency-free CLI.
import argparse

# Import sys for explicit stdout and stderr routing.
import sys

# Import the high-level Guardian client.
from .client import GuardianClient

# Import TCP and serial configuration models and defaults.
from .config import (
    DEFAULT_HOST,
    DEFAULT_PORT,
    DEFAULT_SERIAL_BAUD,
    DEFAULT_TIMEOUT_SECONDS,
    ClientConfig,
    SerialConfig,
)

# Import the expected host-side exception boundary.
from .errors import GuardianCtlError

# Import presentation helpers.
from .presentation import (
    render_info_json,
    render_info_text,
    render_ping_json,
    render_ping_text,
    render_status_json,
    render_status_text,
)

# Import the physical serial transport.
from .serial_transport import GuardianSerialTransport

# Import the exchange contract and TCP transport.
from .transport import ExchangeTransport, GuardianTcpTransport


# Build the complete guardianctl command-line grammar.
def build_parser() -> argparse.ArgumentParser:
    """Return the configured guardianctl argument parser."""

    # Create the top-level parser.
    parser = argparse.ArgumentParser(
        prog="guardianctl",
        description="Guardian F401 host diagnostics and management console.",
    )

    # Configure TCP host selection.
    parser.add_argument(
        "--host",
        default=DEFAULT_HOST,
        help=f"Guardian TCP host or IP address (default: {DEFAULT_HOST})",
    )

    # Configure TCP port selection.
    parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help=f"Guardian TCP port (default: {DEFAULT_PORT})",
    )

    # Select physical UART when provided.
    parser.add_argument(
        "--serial-port",
        help="physical UART port, for example COM5 or /dev/ttyUSB0",
    )

    # Configure physical UART baud rate.
    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_SERIAL_BAUD,
        help=f"physical UART baud rate (default: {DEFAULT_SERIAL_BAUD})",
    )

    # Configure bounded response timeout.
    parser.add_argument(
        "--timeout",
        type=float,
        default=DEFAULT_TIMEOUT_SECONDS,
        help=(
            "connect/response timeout in seconds "
            f"(default: {DEFAULT_TIMEOUT_SECONDS})"
        ),
    )

    # Configure machine-readable output.
    parser.add_argument(
        "--json",
        action="store_true",
        help="emit machine-readable JSON output",
    )

    # Create required command registry.
    subcommands = parser.add_subparsers(
        dest="command",
        required=True,
    )

    # Register PING.
    subcommands.add_parser(
        "ping",
        help="verify Guardian connectivity and measure latency",
    )

    # Register DEVICE_INFO.
    subcommands.add_parser(
        "info",
        help="read device model, firmware and identifier",
    )

    # Register GET_STATUS.
    subcommands.add_parser(
        "status",
        help="read runtime state and protocol diagnostics",
    )

    # Return complete grammar.
    return parser


# Build either TCP or physical serial transport.
def build_transport(args: argparse.Namespace) -> ExchangeTransport:
    """Return the selected Guardian exchange transport."""

    # Select physical UART when a serial port is supplied.
    if args.serial_port:

        # Validate serial configuration.
        config = SerialConfig(
            port=args.serial_port,
            baud_rate=args.baud,
            timeout_seconds=args.timeout,
        )

        # Return physical serial transport.
        return GuardianSerialTransport(config)

    # Validate TCP configuration.
    config = ClientConfig(
        host=args.host,
        port=args.port,
        timeout_seconds=args.timeout,
    )

    # Return TCP transport.
    return GuardianTcpTransport(config)


# Execute one guardianctl command.
def main(argv: list[str] | None = None) -> int:
    """Execute guardianctl and return a conventional process exit code."""

    # Build command grammar.
    parser = build_parser()

    # Parse command line.
    args = parser.parse_args(argv)

    # Build selected transport.
    try:

        # Create TCP or physical serial transport.
        transport = build_transport(args)
    except ValueError as exc:

        # Print configuration failure to stderr.
        print(f"guardianctl: configuration error: {exc}", file=sys.stderr)

        # Return usage/configuration failure status.
        return 2

    # Create high-level client.
    client = GuardianClient(transport=transport)

    # Normalize expected operational failures.
    try:

        # Dispatch PING.
        if args.command == "ping":

            # Execute PING.
            result = client.ping()

            # Select JSON output.
            if args.json:

                # Print machine-readable output.
                print(render_ping_json(result, transport.endpoint))
            else:

                # Print human-readable output.
                print(render_ping_text(result, transport.endpoint))

            # Report success.
            return 0

        # Dispatch DEVICE_INFO.
        if args.command == "info":

            # Execute metadata query.
            info = client.device_info()

            # Select JSON output.
            if args.json:

                # Print machine-readable output.
                print(render_info_json(info))
            else:

                # Print human-readable output.
                print(render_info_text(info))

            # Report success.
            return 0

        # Dispatch GET_STATUS.
        if args.command == "status":

            # Execute status query.
            status = client.status()

            # Select JSON output.
            if args.json:

                # Print machine-readable output.
                print(render_status_json(status))
            else:

                # Print human-readable output.
                print(render_status_text(status))

            # Report success.
            return 0

        # Protect future edits from an unhandled parsed command.
        parser.error(f"unsupported command: {args.command}")

        # Satisfy static analysis.
        return 2

    except GuardianCtlError as exc:

        # Print expected operational failure without traceback.
        print(f"guardianctl: {exc}", file=sys.stderr)

        # Return non-zero operational status.
        return 1


# Run the CLI when this module is executed directly.
if __name__ == "__main__":

    # Exit using the CLI status.
    raise SystemExit(main())
