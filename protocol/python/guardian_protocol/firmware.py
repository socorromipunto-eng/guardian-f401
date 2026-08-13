"""Guardian M12 secure firmware lifecycle payload and package codecs."""

# Import hashlib for deterministic SHA-256 image digests.
import hashlib

# Import hmac for the simulator-only demonstration signing backend.
import hmac

# Import struct for fixed big-endian wire serialization.
import struct

# Import dataclass for immutable shared protocol models.
from dataclasses import dataclass

# Import IntEnum for wire-compatible lifecycle values.
from enum import IntEnum


# Define the first firmware-lifecycle schema revision.
FIRMWARE_SCHEMA_VERSION = 0x01

# Define the signed firmware package file magic.
FIRMWARE_PACKAGE_MAGIC = b"GFU1"

# Define the maximum signature bytes carried by a Guardian manifest.
FIRMWARE_MAX_SIGNATURE_SIZE = 64

# Define a conservative transport chunk that fits inside M10 SECURE_COMMAND.
FIRMWARE_CHUNK_MAX_DATA = 192

# Define the exact signed-manifest domain label.
FIRMWARE_SIGNED_DOMAIN = b"GF-M12-IMAGE"

# Define the exact canonical signed-manifest size.
FIRMWARE_SIGNED_MANIFEST_SIZE = 64

# Define one production-oriented asymmetric signature identifier.
FIRMWARE_SIGNATURE_ED25519 = 0x01

# Reserve a high algorithm identifier for simulator/test-only HMAC signing.
FIRMWARE_SIGNATURE_DEMO_HMAC_SHA256 = 0xFE

# Define fixed manifest fields before the variable signature.
_MANIFEST_PREFIX = struct.Struct(">BBIIHHHI32sB")

# Define one firmware chunk prefix.
_CHUNK_PREFIX = struct.Struct(">BIH")

# Define the fixed public firmware status payload.
_STATUS = struct.Struct(">BBBBIIIIIIHHH")

# Define one schema-only action payload.
_ACTION = struct.Struct(">B")

# Define the package container prefix.
_PACKAGE_PREFIX = struct.Struct(">4sH")


# Define the signature algorithms published by M12.
class FirmwareSignatureAlgorithm(IntEnum):
    """Guardian M12 firmware signature algorithm identifiers."""

    # Identify an externally implemented Ed25519 verifier.
    ED25519 = FIRMWARE_SIGNATURE_ED25519

    # Identify the simulator/test-only HMAC-SHA-256 backend.
    DEMO_HMAC_SHA256 = FIRMWARE_SIGNATURE_DEMO_HMAC_SHA256


# Define the lifecycle states visible through GET_FIRMWARE_STATUS.
class FirmwareLifecycleState(IntEnum):
    """Guardian M12 firmware lifecycle states."""

    # No candidate image is currently being staged.
    IDLE = 0

    # Candidate image bytes are being written sequentially.
    RECEIVING = 1

    # Candidate digest and signature have been verified.
    VERIFIED = 2

    # Candidate is marked for activation on the next boot.
    PENDING_ACTIVATION = 3

    # Candidate boot was confirmed and rollback floor advanced.
    CONFIRMED = 4

    # Candidate boot failed and the previous image remained authoritative.
    ROLLED_BACK = 5

    # Candidate processing failed.
    FAILED = 6


# Define stable public lifecycle failure identifiers.
class FirmwareFailureCode(IntEnum):
    """Guardian M12 firmware lifecycle failure codes."""

    # No failure is currently recorded.
    NONE = 0

    # Lifecycle storage/backend configuration is incomplete.
    UNCONFIGURED = 1

    # Manifest or chunk semantics are invalid.
    INVALID_PAYLOAD = 2

    # Candidate version violates monotonic rollback policy.
    ROLLBACK_BLOCKED = 3

    # Staging storage erase/write operation failed.
    STORAGE_ERROR = 4

    # Staged image digest differs from the signed manifest digest.
    HASH_MISMATCH = 5

    # Image signature is invalid or unsupported.
    SIGNATURE_INVALID = 6

    # Candidate activation metadata could not be persisted.
    ACTIVATION_ERROR = 7

    # Runtime boot confirmation could not advance rollback state.
    CONFIRMATION_ERROR = 8

    # A chunk arrived at an unexpected offset.
    OUT_OF_ORDER = 9


# Store one signed firmware manifest.
@dataclass(frozen=True, slots=True)
class FirmwareManifest:
    """Guardian M12 signed firmware image metadata."""

    # Store the selected signature algorithm.
    signature_algorithm: FirmwareSignatureAlgorithm

    # Store the trusted verifier key slot identifier.
    key_id: int

    # Store the monotonic anti-rollback version counter.
    version_counter: int

    # Store semantic firmware major version.
    firmware_major: int

    # Store semantic firmware minor version.
    firmware_minor: int

    # Store semantic firmware patch version.
    firmware_patch: int

    # Store the exact firmware image size.
    image_size: int

    # Store the SHA-256 digest of the complete image.
    image_sha256: bytes

    # Store the signature over the canonical signed manifest.
    signature: bytes


# Store one sequential firmware data chunk.
@dataclass(frozen=True, slots=True)
class FirmwareChunk:
    """Guardian M12 sequential firmware-image chunk."""

    # Store the zero-based image byte offset.
    offset: int

    # Store the image bytes written at this offset.
    data: bytes


# Store one public firmware lifecycle snapshot.
@dataclass(frozen=True, slots=True)
class FirmwareStatus:
    """Guardian M12 public firmware lifecycle diagnostics."""

    # Store the current lifecycle state.
    state: FirmwareLifecycleState

    # Store the current failure identifier.
    failure: FirmwareFailureCode

    # Store the candidate signature algorithm or zero when absent.
    signature_algorithm: int

    # Store the confirmed active monotonic version.
    active_version_counter: int

    # Store the persisted anti-rollback floor.
    rollback_floor: int

    # Store the candidate monotonic version or zero.
    candidate_version_counter: int

    # Store sequentially staged image bytes.
    bytes_received: int

    # Store expected candidate image size.
    image_size: int

    # Store candidate verifier key identifier.
    key_id: int

    # Store candidate semantic major version.
    firmware_major: int

    # Store candidate semantic minor version.
    firmware_minor: int

    # Store candidate semantic patch version.
    firmware_patch: int


# Store one complete signed package container.
@dataclass(frozen=True, slots=True)
class FirmwarePackage:
    """One M12 signed manifest plus exact firmware image bytes."""

    # Store signed metadata.
    manifest: FirmwareManifest

    # Store exact image bytes.
    image: bytes


# Validate one unsigned integer field.
def _require_uint(
    value: int,
    bits: int,
    field_name: str,
) -> int:
    """Return *value* after exact unsigned range validation."""

    # Calculate the inclusive maximum.
    maximum = (1 << bits) - 1

    # Reject values outside the published wire field.
    if not 0 <= value <= maximum:

        # Raise a precise configuration/codec error.
        raise ValueError(
            f"{field_name} must fit in an unsigned {bits}-bit field"
        )

    # Return the validated integer.
    return value


# Validate manifest signature semantics.
def _validate_signature(
    algorithm: FirmwareSignatureAlgorithm,
    signature: bytes,
) -> bytes:
    """Return immutable signature bytes after algorithm-specific validation."""

    # Normalize the algorithm.
    normalized_algorithm = FirmwareSignatureAlgorithm(
        algorithm
    )

    # Normalize signature bytes.
    value = bytes(
        signature
    )

    # Require the standard Ed25519 signature width.
    if normalized_algorithm == FirmwareSignatureAlgorithm.ED25519:

        # Reject non-standard Ed25519 signature sizes.
        if len(value) != 64:

            # Raise a precise format error.
            raise ValueError(
                "Ed25519 firmware signature must contain exactly 64 bytes"
            )

    # Require one full HMAC-SHA-256 value for demo signing.
    elif normalized_algorithm == FirmwareSignatureAlgorithm.DEMO_HMAC_SHA256:

        # Reject truncated or extended demo signatures.
        if len(value) != 32:

            # Raise a precise format error.
            raise ValueError(
                "demo HMAC firmware signature must contain exactly 32 bytes"
            )

    # Return immutable validated bytes.
    return value


# Validate every signed manifest field.
def validate_firmware_manifest(
    manifest: FirmwareManifest,
) -> FirmwareManifest:
    """Return a normalized immutable M12 firmware manifest."""

    # Normalize the signature algorithm.
    algorithm = FirmwareSignatureAlgorithm(
        manifest.signature_algorithm
    )

    # Validate integer fields.
    key_id = _require_uint(
        manifest.key_id,
        32,
        "key_id",
    )

    # Require a non-zero monotonic candidate counter.
    version_counter = _require_uint(
        manifest.version_counter,
        32,
        "version_counter",
    )

    # Reject zero because it represents no candidate in status payloads.
    if version_counter == 0:

        # Preserve unambiguous lifecycle status semantics.
        raise ValueError(
            "version_counter must be non-zero"
        )

    # Validate semantic version components.
    major = _require_uint(
        manifest.firmware_major,
        16,
        "firmware_major",
    )

    # Validate semantic minor version.
    minor = _require_uint(
        manifest.firmware_minor,
        16,
        "firmware_minor",
    )

    # Validate semantic patch version.
    patch = _require_uint(
        manifest.firmware_patch,
        16,
        "firmware_patch",
    )

    # Validate exact image size.
    image_size = _require_uint(
        manifest.image_size,
        32,
        "image_size",
    )

    # Reject empty firmware images.
    if image_size == 0:

        # Require a real image.
        raise ValueError(
            "image_size must be non-zero"
        )

    # Normalize digest bytes.
    digest = bytes(
        manifest.image_sha256
    )

    # Require the complete SHA-256 digest.
    if len(digest) != 32:

        # Reject ambiguous digest width.
        raise ValueError(
            "image_sha256 must contain exactly 32 bytes"
        )

    # Validate algorithm-specific signature bytes.
    signature = _validate_signature(
        algorithm,
        manifest.signature,
    )

    # Return a fully normalized immutable manifest.
    return FirmwareManifest(
        signature_algorithm=algorithm,
        key_id=key_id,
        version_counter=version_counter,
        firmware_major=major,
        firmware_minor=minor,
        firmware_patch=patch,
        image_size=image_size,
        image_sha256=digest,
        signature=signature,
    )


# Build the exact canonical bytes covered by the firmware signature.
def canonical_firmware_manifest(
    manifest: FirmwareManifest,
) -> bytes:
    """Return the exact 64-byte M12 signed-manifest transcript."""

    # Validate and normalize all fields first.
    normalized = validate_firmware_manifest(
        manifest
    )

    # Build the domain-separated canonical transcript.
    canonical = (
        FIRMWARE_SIGNED_DOMAIN
        + bytes(
            (
                FIRMWARE_SCHEMA_VERSION,
                int(
                    normalized.signature_algorithm
                ),
            )
        )
        + normalized.key_id.to_bytes(
            4,
            "big",
        )
        + normalized.version_counter.to_bytes(
            4,
            "big",
        )
        + normalized.firmware_major.to_bytes(
            2,
            "big",
        )
        + normalized.firmware_minor.to_bytes(
            2,
            "big",
        )
        + normalized.firmware_patch.to_bytes(
            2,
            "big",
        )
        + normalized.image_size.to_bytes(
            4,
            "big",
        )
        + normalized.image_sha256
    )

    # Guard the frozen transcript size.
    if len(canonical) != FIRMWARE_SIGNED_MANIFEST_SIZE:

        # Fail loudly if future edits silently change signature semantics.
        raise AssertionError(
            "canonical firmware manifest size changed unexpectedly"
        )

    # Return immutable canonical bytes.
    return canonical


# Create one simulator/test-only HMAC signature.
def demo_sign_firmware_manifest(
    manifest: FirmwareManifest,
    key: bytes,
) -> FirmwareManifest:
    """Return a manifest signed by the M12 demo-only HMAC backend."""

    # Normalize the demonstration key.
    signing_key = bytes(
        key
    )

    # Require one 256-bit high-entropy test key.
    if len(signing_key) != 32:

        # Reject ambiguous test signing configuration.
        raise ValueError(
            "demo firmware signing key must contain exactly 32 bytes"
        )

    # Create a signature-less normalized placeholder with the required demo width.
    unsigned = FirmwareManifest(
        signature_algorithm=FirmwareSignatureAlgorithm.DEMO_HMAC_SHA256,
        key_id=manifest.key_id,
        version_counter=manifest.version_counter,
        firmware_major=manifest.firmware_major,
        firmware_minor=manifest.firmware_minor,
        firmware_patch=manifest.firmware_patch,
        image_size=manifest.image_size,
        image_sha256=bytes(
            manifest.image_sha256
        ),
        signature=bytes(32),
    )

    # Calculate the complete demo HMAC over canonical signed bytes.
    signature = hmac.new(
        signing_key,
        canonical_firmware_manifest(
            unsigned
        ),
        hashlib.sha256,
    ).digest()

    # Return the signed immutable manifest.
    return FirmwareManifest(
        signature_algorithm=FirmwareSignatureAlgorithm.DEMO_HMAC_SHA256,
        key_id=unsigned.key_id,
        version_counter=unsigned.version_counter,
        firmware_major=unsigned.firmware_major,
        firmware_minor=unsigned.firmware_minor,
        firmware_patch=unsigned.firmware_patch,
        image_size=unsigned.image_size,
        image_sha256=unsigned.image_sha256,
        signature=signature,
    )


# Verify one simulator/test-only HMAC signature.
def demo_verify_firmware_manifest(
    manifest: FirmwareManifest,
    key: bytes,
) -> bool:
    """Return whether one demo-HMAC firmware signature is valid."""

    # Normalize the manifest.
    normalized = validate_firmware_manifest(
        manifest
    )

    # Reject non-demo algorithms at this test-only backend.
    if normalized.signature_algorithm != FirmwareSignatureAlgorithm.DEMO_HMAC_SHA256:

        # Report unsupported algorithm without guessing.
        return False

    # Recalculate the expected signed manifest.
    expected_manifest = demo_sign_firmware_manifest(
        normalized,
        key,
    )

    # Compare the complete signature in constant time.
    return hmac.compare_digest(
        normalized.signature,
        expected_manifest.signature,
    )


# Encode one FIRMWARE_BEGIN payload.
def encode_firmware_manifest(
    manifest: FirmwareManifest,
) -> bytes:
    """Encode one M12 signed firmware manifest."""

    # Validate and normalize all fields.
    normalized = validate_firmware_manifest(
        manifest
    )

    # Pack the fixed fields before the signature.
    prefix = _MANIFEST_PREFIX.pack(
        FIRMWARE_SCHEMA_VERSION,
        int(
            normalized.signature_algorithm
        ),
        normalized.key_id,
        normalized.version_counter,
        normalized.firmware_major,
        normalized.firmware_minor,
        normalized.firmware_patch,
        normalized.image_size,
        normalized.image_sha256,
        len(
            normalized.signature
        ),
    )

    # Append the exact algorithm-specific signature.
    return (
        prefix
        + normalized.signature
    )


# Decode one FIRMWARE_BEGIN payload.
def decode_firmware_manifest(
    payload: bytes,
) -> FirmwareManifest:
    """Decode and validate one M12 signed firmware manifest."""

    # Normalize payload bytes.
    encoded = bytes(
        payload
    )

    # Require the fixed prefix.
    if len(encoded) < _MANIFEST_PREFIX.size:

        # Reject truncated metadata.
        raise ValueError(
            "firmware manifest is truncated"
        )

    # Decode all fixed manifest fields.
    (
        schema,
        algorithm_value,
        key_id,
        version_counter,
        major,
        minor,
        patch,
        image_size,
        digest,
        signature_length,
    ) = _MANIFEST_PREFIX.unpack(
        encoded[
            :_MANIFEST_PREFIX.size
        ]
    )

    # Require the frozen schema.
    if schema != FIRMWARE_SCHEMA_VERSION:

        # Reject unknown lifecycle semantics.
        raise ValueError(
            f"unsupported firmware schema version: {schema}"
        )

    # Require a bounded signature length.
    if not 1 <= signature_length <= FIRMWARE_MAX_SIGNATURE_SIZE:

        # Reject invalid or empty signatures.
        raise ValueError(
            "firmware manifest signature length is invalid"
        )

    # Require exact payload size with no trailing ambiguity.
    if len(encoded) != (
        _MANIFEST_PREFIX.size
        + signature_length
    ):

        # Reject truncation or trailing bytes.
        raise ValueError(
            "firmware manifest signature length does not match payload size"
        )

    # Decode the published algorithm.
    algorithm = FirmwareSignatureAlgorithm(
        algorithm_value
    )

    # Construct and validate the immutable manifest.
    return validate_firmware_manifest(
        FirmwareManifest(
            signature_algorithm=algorithm,
            key_id=key_id,
            version_counter=version_counter,
            firmware_major=major,
            firmware_minor=minor,
            firmware_patch=patch,
            image_size=image_size,
            image_sha256=digest,
            signature=encoded[
                _MANIFEST_PREFIX.size:
            ],
        )
    )


# Encode one sequential firmware chunk.
def encode_firmware_chunk(
    chunk: FirmwareChunk,
) -> bytes:
    """Encode one M12 FIRMWARE_CHUNK payload."""

    # Validate the zero-based image offset.
    offset = _require_uint(
        chunk.offset,
        32,
        "offset",
    )

    # Normalize chunk bytes.
    data = bytes(
        chunk.data
    )

    # Require a non-empty bounded transport chunk.
    if not 1 <= len(data) <= FIRMWARE_CHUNK_MAX_DATA:

        # Reject unsupported chunk sizes before transport.
        raise ValueError(
            (
                "firmware chunk data must contain between 1 and "
                f"{FIRMWARE_CHUNK_MAX_DATA} bytes"
            )
        )

    # Pack schema, offset and exact chunk length.
    prefix = _CHUNK_PREFIX.pack(
        FIRMWARE_SCHEMA_VERSION,
        offset,
        len(data),
    )

    # Append exact image bytes.
    return (
        prefix
        + data
    )


# Decode one sequential firmware chunk.
def decode_firmware_chunk(
    payload: bytes,
) -> FirmwareChunk:
    """Decode one M12 FIRMWARE_CHUNK payload."""

    # Normalize payload bytes.
    encoded = bytes(
        payload
    )

    # Require the fixed prefix plus at least one image byte.
    if len(encoded) <= _CHUNK_PREFIX.size:

        # Reject empty/truncated chunk.
        raise ValueError(
            "firmware chunk is truncated"
        )

    # Decode schema, offset and declared length.
    schema, offset, data_length = _CHUNK_PREFIX.unpack(
        encoded[
            :_CHUNK_PREFIX.size
        ]
    )

    # Require the frozen schema.
    if schema != FIRMWARE_SCHEMA_VERSION:

        # Reject unknown chunk semantics.
        raise ValueError(
            f"unsupported firmware schema version: {schema}"
        )

    # Require the published chunk bound.
    if not 1 <= data_length <= FIRMWARE_CHUNK_MAX_DATA:

        # Reject impossible chunk length.
        raise ValueError(
            "firmware chunk length is outside the M12 bound"
        )

    # Require exact payload size.
    if len(encoded) != (
        _CHUNK_PREFIX.size
        + data_length
    ):

        # Reject truncation or trailing ambiguity.
        raise ValueError(
            "firmware chunk length does not match payload size"
        )

    # Return immutable chunk data.
    return FirmwareChunk(
        offset=offset,
        data=encoded[
            _CHUNK_PREFIX.size:
        ],
    )


# Encode one schema-only FINALIZE or ACTIVATE payload.
def encode_firmware_action() -> bytes:
    """Encode one schema-only M12 firmware lifecycle action."""

    # Return the frozen one-byte schema payload.
    return _ACTION.pack(
        FIRMWARE_SCHEMA_VERSION
    )


# Validate one schema-only FINALIZE or ACTIVATE payload.
def decode_firmware_action(
    payload: bytes,
) -> None:
    """Validate one schema-only M12 firmware lifecycle action."""

    # Normalize payload bytes.
    encoded = bytes(
        payload
    )

    # Require exactly one schema byte.
    if len(encoded) != _ACTION.size:

        # Reject undefined lifecycle action bytes.
        raise ValueError(
            "firmware action payload must contain exactly one byte"
        )

    # Decode the schema.
    (schema,) = _ACTION.unpack(
        encoded
    )

    # Require the frozen schema.
    if schema != FIRMWARE_SCHEMA_VERSION:

        # Reject unknown action semantics.
        raise ValueError(
            f"unsupported firmware schema version: {schema}"
        )


# Encode one public firmware status payload.
def encode_firmware_status(
    status: FirmwareStatus,
) -> bytes:
    """Encode one fixed M12 firmware lifecycle status."""

    # Normalize lifecycle state.
    state = FirmwareLifecycleState(
        status.state
    )

    # Normalize failure code.
    failure = FirmwareFailureCode(
        status.failure
    )

    # Validate compact algorithm byte.
    algorithm = _require_uint(
        status.signature_algorithm,
        8,
        "signature_algorithm",
    )

    # Pack all fixed public lifecycle fields.
    return _STATUS.pack(
        FIRMWARE_SCHEMA_VERSION,
        int(state),
        int(failure),
        algorithm,
        _require_uint(
            status.active_version_counter,
            32,
            "active_version_counter",
        ),
        _require_uint(
            status.rollback_floor,
            32,
            "rollback_floor",
        ),
        _require_uint(
            status.candidate_version_counter,
            32,
            "candidate_version_counter",
        ),
        _require_uint(
            status.bytes_received,
            32,
            "bytes_received",
        ),
        _require_uint(
            status.image_size,
            32,
            "image_size",
        ),
        _require_uint(
            status.key_id,
            32,
            "key_id",
        ),
        _require_uint(
            status.firmware_major,
            16,
            "firmware_major",
        ),
        _require_uint(
            status.firmware_minor,
            16,
            "firmware_minor",
        ),
        _require_uint(
            status.firmware_patch,
            16,
            "firmware_patch",
        ),
    )


# Decode one public firmware status payload.
def decode_firmware_status(
    payload: bytes,
) -> FirmwareStatus:
    """Decode one fixed M12 firmware lifecycle status."""

    # Normalize payload bytes.
    encoded = bytes(
        payload
    )

    # Require exact fixed status size.
    if len(encoded) != _STATUS.size:

        # Reject malformed lifecycle status.
        raise ValueError(
            (
                f"firmware status expected {_STATUS.size} bytes, "
                f"received {len(encoded)}"
            )
        )

    # Decode every public status field.
    (
        schema,
        state_value,
        failure_value,
        algorithm,
        active_counter,
        rollback_floor,
        candidate_counter,
        bytes_received,
        image_size,
        key_id,
        major,
        minor,
        patch,
    ) = _STATUS.unpack(
        encoded
    )

    # Require the frozen schema.
    if schema != FIRMWARE_SCHEMA_VERSION:

        # Reject unknown lifecycle semantics.
        raise ValueError(
            f"unsupported firmware schema version: {schema}"
        )

    # Return typed immutable status.
    return FirmwareStatus(
        state=FirmwareLifecycleState(
            state_value
        ),
        failure=FirmwareFailureCode(
            failure_value
        ),
        signature_algorithm=algorithm,
        active_version_counter=active_counter,
        rollback_floor=rollback_floor,
        candidate_version_counter=candidate_counter,
        bytes_received=bytes_received,
        image_size=image_size,
        key_id=key_id,
        firmware_major=major,
        firmware_minor=minor,
        firmware_patch=patch,
    )


# Build one complete package from exact image bytes and a signed manifest.
def encode_firmware_package(
    package: FirmwarePackage,
) -> bytes:
    """Encode one M12 .gfu package container."""

    # Normalize exact image bytes.
    image = bytes(
        package.image
    )

    # Validate signed manifest.
    manifest = validate_firmware_manifest(
        package.manifest
    )

    # Require image length to match signed metadata.
    if len(image) != manifest.image_size:

        # Reject package/image mismatch.
        raise ValueError(
            "firmware package image length does not match manifest"
        )

    # Require image digest to match signed metadata before packaging.
    digest = hashlib.sha256(
        image
    ).digest()

    # Reject mismatched content.
    if not hmac.compare_digest(
        digest,
        manifest.image_sha256,
    ):

        # Prevent assembling a package with unrelated signed metadata.
        raise ValueError(
            "firmware package image SHA-256 does not match manifest"
        )

    # Encode the variable signed manifest.
    encoded_manifest = encode_firmware_manifest(
        manifest
    )

    # Require manifest length to fit the package field.
    _require_uint(
        len(encoded_manifest),
        16,
        "manifest_length",
    )

    # Return package prefix, manifest and exact image bytes.
    return (
        _PACKAGE_PREFIX.pack(
            FIRMWARE_PACKAGE_MAGIC,
            len(
                encoded_manifest
            ),
        )
        + encoded_manifest
        + image
    )


# Decode one complete .gfu package.
def decode_firmware_package(
    payload: bytes,
) -> FirmwarePackage:
    """Decode and validate one M12 .gfu package container."""

    # Normalize package bytes.
    encoded = bytes(
        payload
    )

    # Require the fixed package prefix.
    if len(encoded) < _PACKAGE_PREFIX.size:

        # Reject truncated package.
        raise ValueError(
            "firmware package is truncated"
        )

    # Decode package magic and manifest length.
    magic, manifest_length = _PACKAGE_PREFIX.unpack(
        encoded[
            :_PACKAGE_PREFIX.size
        ]
    )

    # Require exact package magic.
    if magic != FIRMWARE_PACKAGE_MAGIC:

        # Reject unrelated binary files.
        raise ValueError(
            "firmware package magic is invalid"
        )

    # Require the complete declared manifest.
    manifest_end = (
        _PACKAGE_PREFIX.size
        + manifest_length
    )

    # Reject truncated manifest.
    if manifest_end > len(encoded):

        # Preserve precise package failure.
        raise ValueError(
            "firmware package manifest is truncated"
        )

    # Decode the signed manifest.
    manifest = decode_firmware_manifest(
        encoded[
            _PACKAGE_PREFIX.size:
            manifest_end
        ]
    )

    # Preserve exact image bytes.
    image = encoded[
        manifest_end:
    ]

    # Require exact signed image size.
    if len(image) != manifest.image_size:

        # Reject truncation or unexpected trailing bytes.
        raise ValueError(
            "firmware package image length does not match manifest"
        )

    # Require package content digest to match the signed manifest.
    digest = hashlib.sha256(
        image
    ).digest()

    # Compare digest in constant time.
    if not hmac.compare_digest(
        digest,
        manifest.image_sha256,
    ):

        # Reject corrupted package content.
        raise ValueError(
            "firmware package image SHA-256 does not match manifest"
        )

    # Return immutable package.
    return FirmwarePackage(
        manifest=manifest,
        image=image,
    )
