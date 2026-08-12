"""Module entry point for python -m guardianctl."""

# Import the CLI boundary used by both module and repository helper entry points.
from .cli import main


# Execute guardianctl when Python runs this package as a module.
if __name__ == "__main__":

    # Exit using the CLI's conventional process status.
    raise SystemExit(main())
