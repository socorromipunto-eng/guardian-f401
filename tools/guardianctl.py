"""Run repository-local guardianctl without package installation."""

# Import sys so repository-local source directories can be added to module search paths.
import sys

# Import Path for platform-independent repository path construction.
from pathlib import Path


# Resolve the repository root from this tools script location.
_REPOSITORY_ROOT = Path(__file__).resolve().parents[1]

# Resolve the shared Guardian Protocol Python package directory.
_PROTOCOL_SOURCE = _REPOSITORY_ROOT / "protocol" / "python"

# Resolve the guardianctl Python package directory.
_CONSOLE_SOURCE = _REPOSITORY_ROOT / "console" / "src"

# Add the protocol package directory before importing repository modules.
sys.path.insert(0, str(_PROTOCOL_SOURCE))

# Add the console package directory before importing guardianctl.
sys.path.insert(0, str(_CONSOLE_SOURCE))

# Import the guardianctl CLI only after repository-local module paths are available.
from guardianctl.cli import main


# Execute guardianctl when this repository helper is run directly.
if __name__ == "__main__":

    # Exit using the process status returned by the CLI boundary.
    raise SystemExit(main())
