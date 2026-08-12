"""Thread-safe request sequence allocation for guardianctl."""

# Import Lock so future concurrent host operations cannot duplicate a sequence.
from threading import Lock


# Allocate non-zero unsigned 32-bit request correlation identifiers.
class SequenceManager:
    """Generate monotonically increasing Guardian request sequence values."""

    # Initialize the sequence generator from an explicit deterministic starting point.
    def __init__(self, start: int = 1) -> None:

        # Reject sequence values outside the unsigned 32-bit protocol field.
        if not 1 <= start <= 0xFFFFFFFF:

            # Raise a precise caller configuration error.
            raise ValueError("start must be between 1 and 0xFFFFFFFF")

        # Store the next sequence value that will be returned.
        self._next_value = start

        # Protect the read/update pair from future concurrent callers.
        self._lock = Lock()

    # Return one unique sequence and advance the internal allocator.
    def next(self) -> int:
        """Return the next non-zero unsigned 32-bit sequence."""

        # Serialize sequence allocation across concurrent caller threads.
        with self._lock:

            # Preserve the value selected for this request.
            value = self._next_value

            # Check whether the unsigned 32-bit maximum has been reached.
            if self._next_value == 0xFFFFFFFF:

                # Wrap to one so zero remains reserved for future protocol use.
                self._next_value = 1
            else:

                # Advance monotonically for the next request.
                self._next_value += 1

            # Return the request correlation identifier selected above.
            return value
