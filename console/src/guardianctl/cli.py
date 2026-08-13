"""Command-line interface for Guardian F401 diagnostics and live telemetry."""

# Import argparse for the dependency-free CLI.
import argparse

# Import os for environment-based M10 credential configuration.
import os

# Import shared M9 action and M10 authorization identifiers.
from guardian_protocol import ControlAction, SecurityRole

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
    SecurityClientConfig,
    SerialConfig,
)

# Import the expected host-side exception boundary.
from .errors import GuardianCtlError

# Import human and machine-readable presentation helpers.
from .presentation import (
    render_baseline_json,
    render_baseline_text,
    render_control_result_json,
    render_control_result_text,
    render_control_status_json,
    render_control_status_text,
    render_dsp_json,
    render_dsp_text,
    render_health_json,
    render_health_text,
    render_info_json,
    render_info_text,
    render_ping_json,
    render_ping_text,
    render_authenticated_session_json,
    render_authenticated_session_text,
    render_security_status_json,
    render_security_status_text,
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


    # Configure the optional M10 PSK from command line or environment.
    parser.add_argument(
        "--psk-hex",
        default=os.environ.get("GUARDIAN_PSK_HEX"),
        help=(
            "64 hexadecimal characters for the 256-bit M10 PSK; "
            "defaults to GUARDIAN_PSK_HEX"
        ),
    )

    # Configure the requested authenticated authorization role.
    parser.add_argument(
        "--role",
        choices=("observer", "operator", "admin"),
        default=os.environ.get("GUARDIAN_ROLE", "operator").lower(),
        help=(
            "M10 authenticated role "
            "(observer, operator, admin; default: operator)"
        ),
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

    # Register M8 GET_HEALTH_STATUS.
    subcommands.add_parser(
        "health",
        help="read baseline state, anomaly score and machine-health status",
    )

    # Register M8 baseline lifecycle control.
    baseline_parser = subcommands.add_parser(
        "baseline",
        help="start or reset the runtime machine-health baseline",
    )

    # Create required baseline action subcommands.
    baseline_actions = baseline_parser.add_subparsers(
        dest="baseline_action",
        required=True,
    )

    # Register explicit baseline learning.
    baseline_start = baseline_actions.add_parser(
        "start",
        help="start a fresh bounded baseline learning session",
    )

    # Configure the explicit baseline target.
    baseline_start.add_argument(
        "--samples",
        type=int,
        default=64,
        help="accepted DSP samples required for baseline completion (16-1024, default: 64)",
    )

    # Register runtime baseline reset.
    baseline_actions.add_parser(
        "reset",
        help="erase the runtime baseline and anomaly state",
    )

    # Register M9 supervisory-control status and safety-gated actions.
    control_parser = subcommands.add_parser(
        "control",
        help="inspect or manage M9 supervisory-control state",
    )

    # Create required M9 control action subcommands.
    control_actions = control_parser.add_subparsers(
        dest="control_action",
        required=True,
    )

    # Register GET_CONTROL_STATUS.
    control_actions.add_parser(
        "status",
        help="read M9 run-permit, interlock and fault-latch state",
    )

    # Register ARM without exposing a host machine-run command.
    control_actions.add_parser(
        "arm",
        help="arm supervision after baseline, interlock and safe-output checks",
    )

    # Register unconditional safe DISARM.
    control_actions.add_parser(
        "disarm",
        help="disable supervision and force logical run permit safe-off",
    )

    # Register explicit safe fault reset.
    control_actions.add_parser(
        "clear-fault",
        help="clear latched faults only after safe recovery conditions pass",
    )

    # Register M10 authenticated-session diagnostics and handshake testing.
    security_parser = subcommands.add_parser(
        "security",
        help="inspect or test M10 authenticated-session security",
    )

    # Create required M10 security actions.
    security_actions = security_parser.add_subparsers(
        dest="security_action",
        required=True,
    )

    # Register public GET_SECURITY_STATUS.
    security_actions.add_parser(
        "status",
        help="read public provisioning/session/replay diagnostics",
    )

    # Register an explicit challenge-response authentication test.
    security_actions.add_parser(
        "authenticate",
        help="perform AUTH_BEGIN/AUTH_FINISH using the configured PSK",
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


# Build optional M10 host security configuration.
def build_security_config(
    args: argparse.Namespace,
) -> SecurityClientConfig | None:
    """Return validated M10 credentials or None when no PSK was supplied."""

    # Preserve legacy/insecure compatibility when no PSK was configured.
    if not args.psk_hex:

        # Report absence of M10 client credentials.
        return None

    # Require exact 64-character hexadecimal encoding.
    if len(args.psk_hex) != 64:

        # Reject wrong key width before transport side effects.
        raise ValueError(
            "M10 PSK must contain exactly 64 hexadecimal characters"
        )

    # Decode the hexadecimal PSK.
    try:

        # Convert human configuration into exact 32-byte key material.
        psk = bytes.fromhex(
            args.psk_hex
        )
    except ValueError as exc:

        # Reject non-hexadecimal input.
        raise ValueError(
            "M10 PSK contains non-hexadecimal characters"
        ) from exc

    # Map CLI role text into the shared wire enum.
    role = {
        "observer": SecurityRole.OBSERVER,
        "operator": SecurityRole.OPERATOR,
        "admin": SecurityRole.ADMIN,
    }[args.role]

    # Return immutable validated security configuration.
    return SecurityClientConfig(
        psk=psk,
        role=role,
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

        # Build optional M10 authentication configuration.
        security_config = build_security_config(args)
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

    # Create the high-level typed Guardian client with optional M10 credentials.
    client = GuardianClient(
        transport=transport,
        security_config=security_config,
    )

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

        # Dispatch M8 GET_HEALTH_STATUS.
        if args.command == "health":

            # Execute the current machine-health query.
            health = client.health_status()

            # Select JSON output.
            if args.json:

                # Print machine-readable health output.
                print(render_health_json(health))
            else:

                # Print human-readable health output.
                print(render_health_text(health))

            # Report success.
            return 0

        # Dispatch M10 security diagnostics or explicit authentication.
        if args.command == "security":

            # Read public security status without requiring a PSK.
            if args.security_action == "status":

                # Execute GET_SECURITY_STATUS.
                status = client.security_status()

                # Select JSON output.
                if args.json:

                    # Print machine-readable public diagnostics.
                    print(render_security_status_json(status))
                else:

                    # Print human-readable public diagnostics.
                    print(render_security_status_text(status))

                # Report success.
                return 0

            # Require configured credentials for explicit authentication.
            if security_config is None:

                # Print a concise local configuration error.
                print(
                    (
                        "guardianctl: M10 authentication requires "
                        "--psk-hex or GUARDIAN_PSK_HEX"
                    ),
                    file=sys.stderr,
                )

                # Return configuration failure status.
                return 2

            # Execute a complete challenge-response authentication.
            session = client.authenticate_security()

            # Select JSON output.
            if args.json:

                # Print machine-readable session metadata.
                print(render_authenticated_session_json(session))
            else:

                # Print human-readable session metadata.
                print(render_authenticated_session_text(session))

            # Report success.
            return 0

        # Dispatch M8 baseline lifecycle control.
        if args.command == "baseline":

            # Start a fresh explicit baseline.
            if args.baseline_action == "start":

                # Execute the bounded baseline-start command.
                control = client.start_baseline(
                    args.samples
                )
            else:

                # Execute runtime baseline reset.
                control = client.reset_baseline()

            # Select JSON output.
            if args.json:

                # Print machine-readable acknowledgement.
                print(render_baseline_json(control))
            else:

                # Print human-readable acknowledgement.
                print(render_baseline_text(control))

            # Report success.
            return 0

        # Dispatch M9 supervisory-control operations.
        if args.command == "control":

            # Read current control status without changing policy state.
            if args.control_action == "status":

                # Execute GET_CONTROL_STATUS.
                status = client.control_status()

                # Select JSON output.
                if args.json:

                    # Print machine-readable control status.
                    print(render_control_status_json(status))
                else:

                    # Print human-readable control status.
                    print(render_control_status_text(status))

                # Report successful status query.
                return 0

            # Map the CLI action to the shared M9 wire enum.
            action = {
                "arm": ControlAction.ARM,
                "disarm": ControlAction.DISARM,
                "clear-fault": ControlAction.CLEAR_FAULT,
            }[args.control_action]

            # Execute the safety-gated device action.
            result = client.control_action(
                action
            )

            # Select JSON output.
            if args.json:

                # Print machine-readable normalized result.
                print(render_control_result_json(result))
            else:

                # Print human-readable normalized result.
                print(render_control_result_text(result))

            # Report successful action.
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
