"""DSP feature payload codec for Guardian M7."""

# Import struct for deterministic big-endian binary serialization.
import struct

# Import dataclass for immutable decoded feature snapshots.
from dataclasses import dataclass


# Define the first DSP feature payload schema revision.
DSP_SCHEMA_VERSION = 0x01

# Define the exact fixed M7 payload layout.
_DSP_FEATURES_STRUCT = struct.Struct(">BBIHHHHIHIHHHH")


# Store one immutable decoded DSP feature snapshot.
@dataclass(frozen=True, slots=True)
class DspFeatures:
    """Decoded Guardian M7 DSP feature payload."""

    # Preserve the acquisition block sequence analyzed by firmware.
    block_sequence: int

    # Preserve the physical vibration sample rate in hertz.
    sample_rate_hz: int

    # Store AC RMS vibration magnitude in milli-g.
    rms_mg: int

    # Store the largest absolute DC-removed sample in milli-g.
    peak_mg: int

    # Store crest factor multiplied by 1000.
    crest_factor_milli: int

    # Store dominant frequency in hundredths of one hertz.
    dominant_frequency_centi_hz: int

    # Store estimated dominant sinusoidal peak amplitude in milli-g.
    dominant_peak_mg: int

    # Store spectral centroid in hundredths of one hertz.
    spectral_centroid_centi_hz: int

    # Store 0-500 Hz energy share in permille.
    low_band_permille: int

    # Store >500-1500 Hz energy share in permille.
    mid_band_permille: int

    # Store >1500 Hz through Nyquist energy share in permille.
    high_band_permille: int

    # Preserve M6 acquisition quality flags.
    acquisition_status_flags: int


# Validate one unsigned integer field against a bit width.
def _require_unsigned(
    value: int,
    maximum: int,
    field_name: str,
) -> int:
    """Return *value* after bounded unsigned validation."""

    # Reject values outside the published field width.
    if not 0 <= value <= maximum:

        # Raise a field-specific caller diagnostic.
        raise ValueError(
            f"{field_name} must be between 0 and {maximum}"
        )

    # Return the validated integer.
    return value


# Encode one M7 DSP feature payload.
def encode_dsp_features(features: DspFeatures) -> bytes:
    """Encode one fixed Guardian M7 DSP feature payload."""

    # Validate the 32-bit acquisition block sequence.
    block_sequence = _require_unsigned(
        features.block_sequence,
        0xFFFFFFFF,
        "block_sequence",
    )

    # Validate the physical sample rate field.
    sample_rate_hz = _require_unsigned(
        features.sample_rate_hz,
        0xFFFF,
        "sample_rate_hz",
    )

    # Validate AC RMS vibration.
    rms_mg = _require_unsigned(
        features.rms_mg,
        0xFFFF,
        "rms_mg",
    )

    # Validate time-domain peak vibration.
    peak_mg = _require_unsigned(
        features.peak_mg,
        0xFFFF,
        "peak_mg",
    )

    # Validate fixed-point crest factor.
    crest_factor_milli = _require_unsigned(
        features.crest_factor_milli,
        0xFFFF,
        "crest_factor_milli",
    )

    # Validate dominant frequency.
    dominant_frequency_centi_hz = _require_unsigned(
        features.dominant_frequency_centi_hz,
        0xFFFFFFFF,
        "dominant_frequency_centi_hz",
    )

    # Validate dominant peak amplitude.
    dominant_peak_mg = _require_unsigned(
        features.dominant_peak_mg,
        0xFFFF,
        "dominant_peak_mg",
    )

    # Validate spectral centroid.
    spectral_centroid_centi_hz = _require_unsigned(
        features.spectral_centroid_centi_hz,
        0xFFFFFFFF,
        "spectral_centroid_centi_hz",
    )

    # Validate low-band energy share.
    low_band_permille = _require_unsigned(
        features.low_band_permille,
        0xFFFF,
        "low_band_permille",
    )

    # Validate middle-band energy share.
    mid_band_permille = _require_unsigned(
        features.mid_band_permille,
        0xFFFF,
        "mid_band_permille",
    )

    # Validate high-band energy share.
    high_band_permille = _require_unsigned(
        features.high_band_permille,
        0xFFFF,
        "high_band_permille",
    )

    # Validate acquisition quality flags.
    acquisition_status_flags = _require_unsigned(
        features.acquisition_status_flags,
        0xFFFF,
        "acquisition_status_flags",
    )

    # Pack the exact 32-byte payload including the reserved capability byte.
    return _DSP_FEATURES_STRUCT.pack(
        DSP_SCHEMA_VERSION,
        0,
        block_sequence,
        sample_rate_hz,
        rms_mg,
        peak_mg,
        crest_factor_milli,
        dominant_frequency_centi_hz,
        dominant_peak_mg,
        spectral_centroid_centi_hz,
        low_band_permille,
        mid_band_permille,
        high_band_permille,
        acquisition_status_flags,
    )


# Decode one M7 DSP feature payload.
def decode_dsp_features(payload: bytes) -> DspFeatures:
    """Decode one fixed Guardian M7 DSP feature payload."""

    # Convert any bytes-like input into immutable bytes.
    encoded = bytes(payload)

    # Require the exact fixed payload size.
    if len(encoded) != _DSP_FEATURES_STRUCT.size:

        # Reject truncated or undocumented trailing bytes.
        raise ValueError(
            (
                f"GET_DSP_FEATURES expected "
                f"{_DSP_FEATURES_STRUCT.size} bytes, "
                f"received {len(encoded)}"
            )
        )

    # Decode the complete big-endian feature payload.
    (
        schema_version,
        reserved,
        block_sequence,
        sample_rate_hz,
        rms_mg,
        peak_mg,
        crest_factor_milli,
        dominant_frequency_centi_hz,
        dominant_peak_mg,
        spectral_centroid_centi_hz,
        low_band_permille,
        mid_band_permille,
        high_band_permille,
        acquisition_status_flags,
    ) = _DSP_FEATURES_STRUCT.unpack(encoded)

    # Reject unsupported future schemas.
    if schema_version != DSP_SCHEMA_VERSION:

        # Raise a precise compatibility diagnostic.
        raise ValueError(
            f"unsupported DSP feature schema version: {schema_version}"
        )

    # Require the reserved capability byte to remain zero in schema v1.
    if reserved != 0:

        # Reject undefined v1 semantics.
        raise ValueError(
            "DSP feature reserved byte must be zero"
        )

    # Return one immutable decoded feature snapshot.
    return DspFeatures(
        block_sequence=block_sequence,
        sample_rate_hz=sample_rate_hz,
        rms_mg=rms_mg,
        peak_mg=peak_mg,
        crest_factor_milli=crest_factor_milli,
        dominant_frequency_centi_hz=dominant_frequency_centi_hz,
        dominant_peak_mg=dominant_peak_mg,
        spectral_centroid_centi_hz=spectral_centroid_centi_hz,
        low_band_permille=low_band_permille,
        mid_band_permille=mid_band_permille,
        high_band_permille=high_band_permille,
        acquisition_status_flags=acquisition_status_flags,
    )
