"""Command-line interface for Guardian F401 diagnostics and live telemetry."""

# Import argparse for the dependency-free CLI.
import argparse

# Import sys for explicit stdout and stderr routing.
import sys

# Import the high-level synchronous Guardian client.
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

# Import human and machine-readable presentation helpers.
from .presentation import (
    render_dsp_json,
    render_dsp_text,
    render_info_json,
    render_info_text,
    render_ping_json,
    render_ping_text,
    render_status_json,
    render_status_text,
    render_telemetry_json,
    render_telemetry_text,
)

# Import the physical synchronous serial transport.
from .serial_transport import GuardianSerialTransport

# Import persistent M5 telemetry streaming.
from .telemetry_client import TelemetryMonitor

# Import the synchronous exchange contract and TCP transport.
from .transport import ExchangeTransport, GuardianTcpTransport


# Build the complete guardianctl command-line grammar.
def build_parser() -> argparse.ArgumentParser:
    """Return the configured guardianctl argument parser."""

    # Create the top-level parser.
    parser = argparse.ArgumentParser(
        prog="guardianctl",
        description="Guardian F401 diagnostics, management and telemetry console.",
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

    # Select physical UART when supplied.
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

    # Configure bounded connect and response timeout.
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
        help="emit JSON or JSON Lines output",
    )

    # Create the required command registry.
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

    # Register M7 GET_DSP_FEATURES.
    subcommands.add_parser(
        "dsp",
        help="read the latest RMS, FFT and spectral feature snapshot",
    )

    # Register live M5 telemetry streaming.
    telemetry_parser = subcommands.add_parser(
        "telemetry",
        help="stream bounded live machine telemetry",
    )

    # Configure the device-side telemetry period.
    telemetry_parser.add_argument(
        "--period-ms",
        type=int,
        default=500,
        help="telemetry period in milliseconds (100-60000, default: 500)",
    )

    # Configure a bounded number of live samples.
    telemetry_parser.add_argument(
        "--count",
        type=int,
        default=10,
        help="number of samples to receive (default: 10)",
    )

    # Return the complete grammar.
    return parser


# Build immutable TCP or serial endpoint configuration.
def build_endpoint_config(
    args: argparse.Namespace,
) -> ClientConfig | SerialConfig:
    """Return the selected validated Guardian endpoint configuration."""

    # Select physical UART when a serial port is supplied.
    if args.serial_port:

        # Return validated physical serial configuration.
        return SerialConfig(
            port=args.serial_port,
            baud_rate=args.baud,
            timeout_seconds=args.timeout,
        )

    # Return validated TCP development configuration.
    return ClientConfig(
        host=args.host,
        port=args.port,
        timeout_seconds=args.timeout,
    )


# Build a synchronous request-response transport from endpoint configuration.
def build_transport(
    config: ClientConfig | SerialConfig,
) -> ExchangeTransport:
    """Return the synchronous transport for *config*."""

    # Select physical serial transport for SerialConfig.
    if isinstance(config, SerialConfig):

        # Return the physical synchronous transport.
        return GuardianSerialTransport(config)

    # Return the TCP simulator/development transport.
    return GuardianTcpTransport(config)


# Execute one guardianctl command.
def main(argv: list[str] | None = None) -> int:
    """Execute guardianctl and return a conventional process exit code."""

    # Build deterministic command grammar.
    parser = build_parser()

    # Parse explicit test arguments or the current process command line.
    args = parser.parse_args(argv)

    # Validate the selected endpoint before command side effects.
    try:

        # Build immutable TCP or physical serial configuration.
        endpoint_config = build_endpoint_config(args)
    except ValueError as exc:

        # Print configuration failure to stderr.
        print(
            f"guardianctl: configuration error: {exc}",
            file=sys.stderr,
        )

        # Return usage/configuration failure status.
        return 2

    # Handle live telemetry separately because it requires a persistent stream.
    if args.command == "telemetry":

        # Create the persistent M5 telemetry monitor.
        monitor = TelemetryMonitor(endpoint_config)

        # Select the live renderer once before receiving samples.
        renderer = (
            render_telemetry_json
            if args.json
            else render_telemetry_text
        )

        # Convert validation and communication failures into concise CLI diagnostics.
        try:

            # Stream each sample immediately to stdout.
            monitor.stream_samples(
                period_ms=args.period_ms,
                count=args.count,
                consumer=lambda record: print(
                    renderer(record),
                    flush=True,
                ),
            )

            # Report successful bounded telemetry completion.
            return 0
        except ValueError as exc:

            # Print telemetry policy validation failure to stderr.
            print(
                f"guardianctl: configuration error: {exc}",
                file=sys.stderr,
            )

            # Return usage/configuration failure status.
            return 2
        except GuardianCtlError as exc:

            # Print expected communication failure without traceback.
            print(
                f"guardianctl: {exc}",
                file=sys.stderr,
            )

            # Return operational failure status.
            return 1
        except KeyboardInterrupt:

            # Print a concise operator interruption message.
            print(
                "\nguardianctl: telemetry interrupted",
                file=sys.stderr,
            )

            # Return the conventional shell interruption status.
            return 130

    # Build the existing synchronous request-response transport.
    transport = build_transport(endpoint_config)

    # Create the high-level typed Guardian client.
    client = GuardianClient(transport=transport)

    # Normalize expected synchronous operational failures.
    try:

        # Dispatch PING.
        if args.command == "ping":

            # Execute PING.
            result = client.ping()

            # Select JSON output.
            if args.json:

                # Print machine-readable output.
                print(
                    render_ping_json(
                        result,
                        transport.endpoint,
                    )
                )
            else:

                # Print human-readable output.
                print(
                    render_ping_text(
                        result,
                        transport.endpoint,
                    )
                )

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

        # Dispatch GET_DSP_FEATURES.
        if args.command == "dsp":

            # Execute the latest DSP feature query.
            features = client.dsp_features()

            # Select JSON output.
            if args.json:

                # Print machine-readable feature output.
                print(render_dsp_json(features))
            else:

                # Print human-readable feature output.
                print(render_dsp_text(features))

            # Report success.
            return 0

        # Protect future edits from an unhandled parsed command.
        parser.error(f"unsupported command: {args.command}")

        # Satisfy static analysis.
        return 2

    except GuardianCtlError as exc:

        # Print expected operational failure without traceback.
        print(
            f"guardianctl: {exc}",
            file=sys.stderr,
        )

        # Return non-zero operational status.
        return 1


# Run the CLI when this module is executed directly.
if __name__ == "__main__":

    # Exit using the CLI status.
    raise SystemExit(main())
