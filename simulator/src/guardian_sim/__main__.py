"""Command-line entry point for the Guardian F401 M2 device simulator."""

# Import argparse for a dependency-free command-line interface.
import argparse

# Import published simulator defaults and immutable configuration.
from .config import (
    DEFAULT_HOST,
    DEFAULT_PORT,
    DEFAULT_SECURITY_PSK_HEX,
    SimulatorConfig,
)

# Import the foreground TCP simulator runner.
from .server import run_server


# Parse command-line arguments into validated simulator configuration.
def parse_args() -> SimulatorConfig:
    """Return simulator configuration parsed from command-line options."""

    # Create the command-line argument parser.
    parser = argparse.ArgumentParser(
        description="Run the Guardian F401 protocol device simulator.",
    )

    # Allow explicit bind-address selection while preserving loopback safety by default.
    parser.add_argument(
        "--host",
        default=DEFAULT_HOST,
        help=f"TCP bind address (default: {DEFAULT_HOST})",
    )

    # Allow explicit TCP port selection for parallel development environments.
    parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help=f"TCP port (default: {DEFAULT_PORT})",
    )

    # Enable the M10 privileged-command security gate explicitly.
    parser.add_argument(
        "--secure",
        action="store_true",
        help="require M10 authenticated wrapping for baseline/control commands",
    )

    # Allow an explicit simulator-only PSK override.
    parser.add_argument(
        "--psk-hex",
        default=DEFAULT_SECURITY_PSK_HEX,
        help="64 hexadecimal characters for the simulator M10 PSK",
    )

    # Parse arguments supplied by the current process.
    args = parser.parse_args()

    # Require exact 256-bit hexadecimal key material.
    if len(args.psk_hex) != 64:
        raise SystemExit("--psk-hex must contain exactly 64 hexadecimal characters")

    # Decode the simulator PSK.
    try:
        security_psk = bytes.fromhex(args.psk_hex)
    except ValueError as exc:
        raise SystemExit("--psk-hex contains non-hexadecimal characters") from exc

    # Convert parsed values into immutable validated simulator configuration.
    return SimulatorConfig(
        host=args.host,
        port=args.port,
        security_enabled=args.secure,
        security_psk=security_psk,
    )


# Start the simulator when this module is executed as a program.
def main() -> None:
    """Run the configured Guardian F401 simulator."""

    # Parse and validate the operator's network configuration.
    config = parse_args()

    # Run the simulator until the operator interrupts it.
    run_server(config)


# Execute the CLI only when Python invokes this module as the process entry point.
if __name__ == "__main__":

    # Start the Guardian F401 simulator.
    main()
