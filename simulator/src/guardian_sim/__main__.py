"""Command-line entry point for the Guardian F401 M2 device simulator."""

# Import argparse for a dependency-free command-line interface.
import argparse

# Import published simulator defaults and immutable configuration.
from .config import DEFAULT_HOST, DEFAULT_PORT, SimulatorConfig

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

    # Parse arguments supplied by the current process.
    args = parser.parse_args()

    # Convert parsed values into the immutable validated simulator configuration.
    return SimulatorConfig(
        host=args.host,
        port=args.port,
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
