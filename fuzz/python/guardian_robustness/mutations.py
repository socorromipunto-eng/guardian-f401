"""Deterministic Guardian byte-stream mutations for defensive robustness testing."""

# Import random for reproducible pseudo-random fault plans.
import random

# Import dataclass for immutable mutation metadata.
from dataclasses import dataclass

# Import Enum for stable mutation identifiers.
from enum import Enum, auto


# Define the available byte-stream fault classes.
class MutationKind(Enum):
    """Defensive byte-stream mutations used by M11 campaigns."""

    # Flip exactly one bit.
    FLIP_BIT = auto()

    # Replace one byte with a random value.
    REPLACE_BYTE = auto()

    # Delete one byte.
    DELETE_BYTE = auto()

    # Duplicate one bounded slice.
    DUPLICATE_SLICE = auto()

    # Truncate the stream at one position.
    TRUNCATE = auto()

    # Insert bounded random noise.
    INSERT_NOISE = auto()

    # Corrupt the two-byte Guardian payload-length field.
    POISON_LENGTH = auto()

    # Corrupt one byte in the CRC trailer.
    CORRUPT_CRC = auto()

    # Corrupt one Guardian magic byte.
    CORRUPT_MAGIC = auto()


# Store one reproducible mutation result.
@dataclass(frozen=True, slots=True)
class MutationCase:
    """One mutated byte stream plus enough metadata to reproduce it."""

    # Store the mutation class.
    kind: MutationKind

    # Store the seed used by the local mutation RNG.
    seed: int

    # Store the mutated byte stream.
    data: bytes

    # Store a compact operator-facing description.
    description: str


# Return a deterministic non-empty byte range.
def _bounded_index(
    rng: random.Random,
    length: int,
) -> int:
    """Return one valid byte index for a non-empty buffer."""

    # Reject empty input because no byte index exists.
    if length <= 0:

        # Raise a precise local test-configuration failure.
        raise ValueError(
            "mutation requires at least one input byte"
        )

    # Return one reproducible valid index.
    return rng.randrange(
        length
    )


# Flip exactly one deterministic bit.
def _flip_bit(
    data: bytes,
    rng: random.Random,
) -> tuple[bytes, str]:
    """Flip one bit while preserving stream length."""

    # Convert immutable input into mutable test storage.
    mutated = bytearray(
        data
    )

    # Select one byte.
    index = _bounded_index(
        rng,
        len(mutated),
    )

    # Select one bit.
    bit = rng.randrange(
        8
    )

    # Flip the selected bit.
    mutated[index] ^= 1 << bit

    # Return immutable output and reproduction metadata.
    return (
        bytes(mutated),
        f"flip bit {bit} at byte {index}",
    )


# Replace exactly one deterministic byte.
def _replace_byte(
    data: bytes,
    rng: random.Random,
) -> tuple[bytes, str]:
    """Replace one byte with a different value."""

    # Convert immutable input into mutable test storage.
    mutated = bytearray(
        data
    )

    # Select one byte.
    index = _bounded_index(
        rng,
        len(mutated),
    )

    # Preserve the original value.
    original = mutated[
        index
    ]

    # Generate a guaranteed-different replacement.
    replacement = (
        original
        + 1
        + rng.randrange(255)
    ) & 0xFF

    # Replace the selected byte.
    mutated[index] = replacement

    # Return immutable output and reproduction metadata.
    return (
        bytes(mutated),
        (
            f"replace byte {index}: "
            f"0x{original:02X}->0x{replacement:02X}"
        ),
    )


# Delete exactly one deterministic byte.
def _delete_byte(
    data: bytes,
    rng: random.Random,
) -> tuple[bytes, str]:
    """Delete one byte from a non-empty stream."""

    # Select one byte.
    index = _bounded_index(
        rng,
        len(data),
    )

    # Remove only that byte.
    mutated = (
        data[:index]
        + data[index + 1:]
    )

    # Return the shortened stream.
    return (
        mutated,
        f"delete byte {index}",
    )


# Duplicate one bounded deterministic slice.
def _duplicate_slice(
    data: bytes,
    rng: random.Random,
) -> tuple[bytes, str]:
    """Duplicate a short slice at a deterministic insertion point."""

    # Require one byte of source material.
    if not data:

        # Reject impossible mutation.
        raise ValueError(
            "slice duplication requires non-empty input"
        )

    # Select the source slice start.
    start = rng.randrange(
        len(data)
    )

    # Limit duplication to sixteen bytes to keep campaigns bounded.
    maximum = min(
        16,
        len(data) - start,
    )

    # Select a non-zero slice width.
    width = rng.randint(
        1,
        maximum,
    )

    # Select an insertion point including EOF.
    insertion = rng.randrange(
        len(data) + 1
    )

    # Copy the selected slice.
    duplicate = data[
        start:
        start + width
    ]

    # Insert the duplicate without removing original bytes.
    mutated = (
        data[:insertion]
        + duplicate
        + data[insertion:]
    )

    # Return the expanded stream.
    return (
        mutated,
        (
            f"duplicate bytes {start}:{start + width} "
            f"at {insertion}"
        ),
    )


# Truncate one deterministic suffix.
def _truncate(
    data: bytes,
    rng: random.Random,
) -> tuple[bytes, str]:
    """Truncate a stream while always changing non-empty input."""

    # Require one source byte.
    if not data:

        # Reject impossible truncation.
        raise ValueError(
            "truncation requires non-empty input"
        )

    # Select a cut before EOF so output differs.
    cut = rng.randrange(
        len(data)
    )

    # Keep only the prefix.
    mutated = data[
        :cut
    ]

    # Return the truncated stream.
    return (
        mutated,
        f"truncate at byte {cut}",
    )


# Insert bounded deterministic random noise.
def _insert_noise(
    data: bytes,
    rng: random.Random,
) -> tuple[bytes, str]:
    """Insert one to sixteen pseudo-random bytes."""

    # Select bounded noise width.
    width = rng.randint(
        1,
        16,
    )

    # Generate deterministic bytes from the local RNG.
    noise = bytes(
        rng.randrange(256)
        for _ in range(width)
    )

    # Select an insertion point including EOF.
    insertion = rng.randrange(
        len(data) + 1
    )

    # Insert noise without removing source bytes.
    mutated = (
        data[:insertion]
        + noise
        + data[insertion:]
    )

    # Return the expanded stream.
    return (
        mutated,
        f"insert {width} noise bytes at {insertion}",
    )


# Corrupt the Guardian payload-length field when a fixed header exists.
def _poison_length(
    data: bytes,
    rng: random.Random,
) -> tuple[bytes, str]:
    """Replace bytes 10..11 with one deliberately different length."""

    # Require the complete fixed Guardian header.
    if len(data) < 12:

        # Fall back to one deterministic bit flip for short input.
        return _flip_bit(
            data,
            rng,
        )

    # Convert input into mutable storage.
    mutated = bytearray(
        data
    )

    # Read the existing big-endian payload length.
    original = int.from_bytes(
        mutated[10:12],
        "big",
    )

    # Generate one different 16-bit declared length.
    replacement = (
        original
        + 1
        + rng.randrange(0xFFFF)
    ) & 0xFFFF

    # Publish the corrupted big-endian length.
    mutated[10:12] = replacement.to_bytes(
        2,
        "big",
    )

    # Return the corrupted stream.
    return (
        bytes(mutated),
        (
            f"poison payload length "
            f"{original}->{replacement}"
        ),
    )


# Corrupt one byte of the CRC trailer when present.
def _corrupt_crc(
    data: bytes,
    rng: random.Random,
) -> tuple[bytes, str]:
    """Corrupt one CRC byte without changing frame length."""

    # Require at least the four-byte CRC trailer.
    if len(data) < 4:

        # Fall back to a bit flip for short input.
        return _flip_bit(
            data,
            rng,
        )

    # Convert input into mutable storage.
    mutated = bytearray(
        data
    )

    # Select one of the final four CRC bytes.
    index = len(mutated) - 4 + rng.randrange(
        4
    )

    # Flip one deterministic low-order bit.
    mutated[index] ^= 1 << rng.randrange(8)

    # Return corrupted CRC bytes.
    return (
        bytes(mutated),
        f"corrupt CRC byte {index}",
    )


# Corrupt one Guardian synchronization byte.
def _corrupt_magic(
    data: bytes,
    rng: random.Random,
) -> tuple[bytes, str]:
    """Corrupt one of the first two magic bytes when available."""

    # Require at least one byte.
    if not data:

        # Reject impossible mutation.
        raise ValueError(
            "magic corruption requires non-empty input"
        )

    # Convert input into mutable storage.
    mutated = bytearray(
        data
    )

    # Select byte zero or one when available.
    index = rng.randrange(
        min(
            2,
            len(mutated),
        )
    )

    # Change the selected synchronization byte.
    mutated[index] ^= 0x01

    # Return corrupted synchronization.
    return (
        bytes(mutated),
        f"corrupt magic byte {index}",
    )


# Apply one selected mutation reproducibly.
def mutate(
    data: bytes,
    kind: MutationKind,
    seed: int,
) -> MutationCase:
    """Return one deterministic mutation of *data*."""

    # Normalize source data into immutable bytes.
    source = bytes(
        data
    )

    # Require source bytes because every mutation must actually change something.
    if not source:

        # Reject empty campaigns early.
        raise ValueError(
            "mutation source cannot be empty"
        )

    # Create one local RNG so global random state is never modified.
    rng = random.Random(
        seed
    )

    # Map each mutation class to its implementation.
    handlers = {
        MutationKind.FLIP_BIT: _flip_bit,
        MutationKind.REPLACE_BYTE: _replace_byte,
        MutationKind.DELETE_BYTE: _delete_byte,
        MutationKind.DUPLICATE_SLICE: _duplicate_slice,
        MutationKind.TRUNCATE: _truncate,
        MutationKind.INSERT_NOISE: _insert_noise,
        MutationKind.POISON_LENGTH: _poison_length,
        MutationKind.CORRUPT_CRC: _corrupt_crc,
        MutationKind.CORRUPT_MAGIC: _corrupt_magic,
    }

    # Execute the selected mutation.
    mutated, description = handlers[
        MutationKind(kind)
    ](
        source,
        rng,
    )

    # Require the mutation to change the source stream.
    if mutated == source:

        # Reject ineffective mutation logic.
        raise AssertionError(
            f"mutation did not change source: {kind.name}"
        )

    # Return immutable reproduction metadata.
    return MutationCase(
        kind=MutationKind(kind),
        seed=seed,
        data=mutated,
        description=description,
    )


# Return one deterministic chunking plan for arbitrary stream fragmentation.
def chunk_stream(
    data: bytes,
    seed: int,
    maximum_chunk: int = 17,
) -> tuple[bytes, ...]:
    """Split bytes into deterministic non-empty transport fragments."""

    # Normalize source bytes.
    source = bytes(
        data
    )

    # Reject invalid chunk bounds.
    if maximum_chunk <= 0:

        # Require a positive upper bound.
        raise ValueError(
            "maximum_chunk must be positive"
        )

    # Return an empty tuple for an empty stream.
    if not source:

        # Preserve empty-stream semantics.
        return ()

    # Create a local reproducible RNG.
    rng = random.Random(
        seed
    )

    # Collect immutable chunks.
    chunks: list[bytes] = []

    # Track next unconsumed byte.
    offset = 0

    # Split until every byte is owned by one chunk.
    while offset < len(source):

        # Select a bounded positive fragment size.
        width = rng.randint(
            1,
            min(
                maximum_chunk,
                len(source) - offset,
            ),
        )

        # Append the next exact fragment.
        chunks.append(
            source[
                offset:
                offset + width
            ]
        )

        # Advance stream ownership.
        offset += width

    # Return immutable chunk plan.
    return tuple(
        chunks
    )
