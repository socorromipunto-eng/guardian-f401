"""Run the repository-local Guardian F401 simulator without package installation."""

# Import sys so repository-local source directories can be added to module search paths.
import sys

# Import Path for platform-independent repository path construction.
from pathlib import Path


# Resolve the repository root from this tools script location.
_REPOSITORY_ROOT = Path(__file__).resolve().parents[1]

# Resolve the shared Guardian Protocol Python package directory.
_PROTOCOL_SOURCE = _REPOSITORY_ROOT / "protocol" / "python"

# Resolve the Guardian simulator Python package directory.
_SIMULATOR_SOURCE = _REPOSITORY_ROOT / "simulator" / "src"

# Add the protocol package directory before importing repository modules.
sys.path.insert(0, str(_PROTOCOL_SOURCE))

# Add the simulator package directory before importing its command-line entry point.
sys.path.insert(0, str(_SIMULATOR_SOURCE))

# Import the simulator CLI only after repository-local module paths are available.
from guardian_sim.__main__ import main


# Execute the simulator CLI when this helper is run directly.
if __name__ == "__main__":

    # Start the Guardian F401 software device simulator.
    main()
