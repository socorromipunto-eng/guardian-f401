"""CRC32 implementation used by Guardian Protocol v0.1."""

# Define the reflected IEEE CRC-32 polynomial used by the protocol.
_CRC32_POLYNOMIAL = 0xEDB88320

# Define the initial CRC accumulator value.
_CRC32_INITIAL = 0xFFFFFFFF

# Define the final XOR mask applied after all input bytes are processed.
_CRC32_FINAL_XOR = 0xFFFFFFFF


# Compute the CRC32 exactly as specified by Guardian Protocol v0.1.
def crc32_ieee(data: bytes) -> int:
    """Return the reflected IEEE CRC-32 value for *data*."""

    # Start with the protocol-defined initial accumulator.
    crc = _CRC32_INITIAL

    # Process every byte in wire order.
    for byte in data:

        # Mix the new byte into the low eight bits of the accumulator.
        crc ^= byte

        # Process every bit in the current byte.
        for _ in range(8):

            # Check whether the reflected least-significant bit is set.
            if crc & 0x00000001:

                # Shift and apply the reflected polynomial when the bit is set.
                crc = (crc >> 1) ^ _CRC32_POLYNOMIAL
            else:

                # Shift without polynomial feedback when the bit is clear.
                crc >>= 1

    # Apply the protocol-defined final XOR and constrain the result to 32 bits.
    return (crc ^ _CRC32_FINAL_XOR) & 0xFFFFFFFF
