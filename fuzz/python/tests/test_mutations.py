"""Unit tests for Guardian M11 deterministic mutation primitives."""

# Import unittest from the Python standard library.
import unittest

# Import the public M11 mutation API.
from guardian_robustness import (
    MutationKind,
    chunk_stream,
    mutate,
)


# Verify reproducible defensive fault generation.
class MutationTests(unittest.TestCase):
    """Exercise every M11 mutation and fragmentation primitive."""

    # Define one stable non-empty test frame-like byte stream.
    SOURCE = bytes.fromhex(
        "47460101010000000001000034025e68"
    )

    # Verify every mutation changes source bytes and is reproducible.
    def test_every_mutation_is_reproducible_and_changes_input(self) -> None:

        # Exercise every published mutation class.
        for kind in MutationKind:

            # Create the first deterministic mutation.
            first = mutate(
                self.SOURCE,
                kind,
                0x12345678,
            )

            # Recreate the same mutation from the same seed.
            second = mutate(
                self.SOURCE,
                kind,
                0x12345678,
            )

            # Require exact deterministic reproduction.
            self.assertEqual(
                first,
                second,
            )

            # Require every mutation to actually change the source stream.
            self.assertNotEqual(
                first.data,
                self.SOURCE,
            )

    # Verify arbitrary fragmentation preserves exact byte ordering.
    def test_chunk_stream_round_trip(self) -> None:

        # Split the canonical source into deterministic fragments.
        chunks = chunk_stream(
            self.SOURCE,
            seed=0xA5A5A5A5,
            maximum_chunk=3,
        )

        # Require every chunk to be non-empty.
        self.assertTrue(
            all(chunks)
        )

        # Require exact stream reconstruction.
        self.assertEqual(
            b"".join(chunks),
            self.SOURCE,
        )

    # Verify invalid empty mutation input fails clearly.
    def test_empty_mutation_source_is_rejected(self) -> None:

        # Require a precise local test configuration failure.
        with self.assertRaises(ValueError):

            # Attempt an impossible mutation.
            mutate(
                b"",
                MutationKind.FLIP_BIT,
                1,
            )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
