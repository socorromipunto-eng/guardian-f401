"""Unit tests for guardianctl request sequence allocation."""

# Import unittest from the Python standard library.
import unittest

# Import the sequence allocator under test.
from guardianctl import SequenceManager


# Verify deterministic sequence allocation and unsigned wrap behavior.
class SequenceManagerTests(unittest.TestCase):
    """Exercise guardianctl request correlation allocation."""

    # Verify monotonic values starting from the default request sequence.
    def test_default_sequence_is_monotonic(self) -> None:

        # Create a fresh default allocator.
        manager = SequenceManager()

        # Require the published initial non-zero sequence.
        self.assertEqual(manager.next(), 1)

        # Require deterministic monotonic advancement.
        self.assertEqual(manager.next(), 2)

        # Require another monotonic advancement.
        self.assertEqual(manager.next(), 3)

    # Verify unsigned wrap keeps zero reserved.
    def test_sequence_wraps_from_maximum_to_one(self) -> None:

        # Start immediately at the unsigned 32-bit maximum.
        manager = SequenceManager(start=0xFFFFFFFF)

        # Require the maximum value to be emitted once.
        self.assertEqual(manager.next(), 0xFFFFFFFF)

        # Require wrap to one instead of zero.
        self.assertEqual(manager.next(), 1)

    # Verify invalid initial values are rejected.
    def test_zero_start_is_rejected(self) -> None:

        # Require the allocator to reject the reserved zero value.
        with self.assertRaises(ValueError):

            # Attempt to create an allocator with invalid zero start.
            SequenceManager(start=0)


# Execute the tests when this file is run directly.
if __name__ == "__main__":

    # Start the standard-library unit test runner with verbose output.
    unittest.main(verbosity=2)
