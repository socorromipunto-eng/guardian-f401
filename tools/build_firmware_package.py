"""Build Guardian M12 signed firmware packages without installing the repository."""

# Import argparse for the dependency-free command-line interface.
import argparse

# Import hashlib for exact firmware SHA-256 digests.
import hashlib

# Import os for optional signing-key environment configuration.
import os

# Import sys so repository-local source roots can be loaded.
import sys

# Import Path for binary image/signature/package files.
from pathlib import Path


# Resolve the repository root from this tools script location.
_REPOSITORY_ROOT = Path(__file__).resolve().parents[1]

# Add the Guardian Protocol Python package.
sys.path.insert(
    0,
    str(
        _REPOSITORY_ROOT
        / "protocol"
        / "python"
    ),
)

# Import the shared M12 package/signature codecs.
from guardian_protocol import (
    FirmwareManifest,
    FirmwarePackage,
    FirmwareSignatureAlgorithm,
    canonical_firmware_manifest,
    demo_sign_firmware_manifest,
    encode_firmware_package,
)


# Define the intentionally public simulator-only firmware signing key.
_DEFAULT_DEMO_SIGNING_KEY_HEX = (
    "7f6e5d4c3b2a19081726354453627180"
    "90a1b2c3d4e5f60718293a4b5c6d7e8f"
)


# Parse one dotted semantic version.
def _parse_version(
    value: str,
) -> tuple[int, int, int]:
    """Return one validated MAJOR.MINOR.PATCH tuple."""

    # Split exact semantic components.
    parts = value.split(
        "."
    )

    # Require exactly three components.
    if len(parts) != 3:

        # Reject ambiguous version text.
        raise argparse.ArgumentTypeError(
            "version must use MAJOR.MINOR.PATCH"
        )

    # Convert every component to decimal integer.
    try:

        # Parse all three version fields.
        version = tuple(
            int(part, 10)
            for part in parts
        )
    except ValueError as exc:

        # Reject non-decimal version components.
        raise argparse.ArgumentTypeError(
            "version components must be decimal integers"
        ) from exc

    # Require every semantic field to fit uint16.
    if any(
        not 0 <= component <= 0xFFFF
        for component in version
    ):

        # Reject out-of-range version fields.
        raise argparse.ArgumentTypeError(
            "version components must be between 0 and 65535"
        )

    # Return the typed three-item tuple.
    return (
        version[0],
        version[1],
        version[2],
    )


# Build one manifest with a caller-selected signature placeholder.
def _manifest(
    image: bytes,
    version_counter: int,
    version: tuple[int, int, int],
    key_id: int,
    algorithm: FirmwareSignatureAlgorithm,
    signature: bytes,
) -> FirmwareManifest:
    """Return one immutable manifest over exact image bytes."""

    # Calculate the exact complete image digest.
    digest = hashlib.sha256(
        image
    ).digest()

    # Return signed metadata.
    return FirmwareManifest(
        signature_algorithm=algorithm,
        key_id=key_id,
        version_counter=version_counter,
        firmware_major=version[0],
        firmware_minor=version[1],
        firmware_patch=version[2],
        image_size=len(image),
        image_sha256=digest,
        signature=signature,
    )


# Build the command-line grammar.
def build_parser() -> argparse.ArgumentParser:
    """Return the M12 package-builder argument parser."""

    # Create the top-level parser.
    parser = argparse.ArgumentParser(
        description=(
            "Build demo-HMAC packages or prepare/assemble externally "
            "signed Ed25519 Guardian M12 firmware packages."
        )
    )

    # Create required build modes.
    modes = parser.add_subparsers(
        dest="mode",
        required=True,
    )

    # Register simulator-only demo package generation.
    demo = modes.add_parser(
        "demo",
        help="build a simulator-only HMAC-signed .gfu package",
    )

    # Accept an optional real input image.
    demo.add_argument(
        "--image",
        type=Path,
        help="input firmware image; omitted generates deterministic demo bytes",
    )

    # Require an output package path.
    demo.add_argument(
        "--output",
        type=Path,
        required=True,
        help="output .gfu package path",
    )

    # Configure monotonic candidate version.
    demo.add_argument(
        "--version-counter",
        type=int,
        default=12,
        help="monotonic anti-rollback counter (default: 12)",
    )

    # Configure semantic version.
    demo.add_argument(
        "--version",
        type=_parse_version,
        default=(0, 12, 0),
        help="semantic MAJOR.MINOR.PATCH (default: 0.12.0)",
    )

    # Configure trusted key slot identifier.
    demo.add_argument(
        "--key-id",
        type=int,
        default=1,
        help="simulator verifier key identifier (default: 1)",
    )

    # Configure the test-only key from CLI or environment.
    demo.add_argument(
        "--signing-key-hex",
        default=os.environ.get(
            "GUARDIAN_FIRMWARE_SIGNING_KEY_HEX",
            _DEFAULT_DEMO_SIGNING_KEY_HEX,
        ),
        help=(
            "64 hex characters for the simulator-only HMAC signing key; "
            "defaults to GUARDIAN_FIRMWARE_SIGNING_KEY_HEX or the public demo key"
        ),
    )

    # Register production-oriented external-signature preparation.
    prepare = modes.add_parser(
        "prepare",
        help="write the exact canonical manifest bytes to sign with an external Ed25519 key",
    )

    # Require the production image.
    prepare.add_argument(
        "image",
        type=Path,
        help="input firmware image",
    )

    # Require the exact canonical signing-input output.
    prepare.add_argument(
        "--output",
        type=Path,
        required=True,
        help="output 64-byte canonical manifest-to-sign file",
    )

    # Require monotonic version.
    prepare.add_argument(
        "--version-counter",
        type=int,
        required=True,
        help="monotonic anti-rollback counter",
    )

    # Require semantic version.
    prepare.add_argument(
        "--version",
        type=_parse_version,
        required=True,
        help="semantic MAJOR.MINOR.PATCH",
    )

    # Require trusted production key slot.
    prepare.add_argument(
        "--key-id",
        type=int,
        required=True,
        help="trusted production verifier key identifier",
    )

    # Register externally signed package assembly.
    assemble = modes.add_parser(
        "assemble",
        help="assemble a .gfu package from image metadata and a 64-byte external Ed25519 signature",
    )

    # Require the production image.
    assemble.add_argument(
        "image",
        type=Path,
        help="input firmware image",
    )

    # Require the external raw signature.
    assemble.add_argument(
        "signature",
        type=Path,
        help="raw 64-byte Ed25519 signature over the prepare output",
    )

    # Require output package path.
    assemble.add_argument(
        "--output",
        type=Path,
        required=True,
        help="output .gfu package path",
    )

    # Require monotonic version counter.
    assemble.add_argument(
        "--version-counter",
        type=int,
        required=True,
        help="monotonic anti-rollback counter",
    )

    # Require semantic version.
    assemble.add_argument(
        "--version",
        type=_parse_version,
        required=True,
        help="semantic MAJOR.MINOR.PATCH",
    )

    # Require trusted production key slot.
    assemble.add_argument(
        "--key-id",
        type=int,
        required=True,
        help="trusted production verifier key identifier",
    )

    # Return the complete grammar.
    return parser


# Read one local image with precise failure reporting.
def _read(
    path: Path,
    description: str,
) -> bytes:
    """Return exact local file bytes."""

    # Read the complete binary file.
    try:

        # Return exact bytes.
        return path.read_bytes()
    except OSError as exc:

        # Convert filesystem failure into concise CLI termination.
        raise SystemExit(
            f"cannot read {description} '{path}': {exc}"
        ) from exc


# Execute the selected package build mode.
def main(
    argv: list[str] | None = None,
) -> int:
    """Build one M12 signing input or signed package."""

    # Parse explicit or process arguments.
    args = build_parser().parse_args(
        argv
    )

    # Validate common unsigned integer fields.
    if not 1 <= args.version_counter <= 0xFFFFFFFF:

        # Reject zero/out-of-range monotonic versions.
        raise SystemExit(
            "--version-counter must be between 1 and 0xFFFFFFFF"
        )

    # Validate key identifier.
    if not 0 <= args.key_id <= 0xFFFFFFFF:

        # Reject out-of-range key slots.
        raise SystemExit(
            "--key-id must be between 0 and 0xFFFFFFFF"
        )

    # Build a simulator-only signed package.
    if args.mode == "demo":

        # Generate deterministic demo bytes when no image was supplied.
        image = (
            _read(
                args.image,
                "firmware image",
            )
            if args.image is not None
            else (
                b"Guardian-F401-M12-DEMO\x00"
                * 256
            )
        )

        # Reject empty image.
        if not image:

            # Require real candidate content.
            raise SystemExit(
                "firmware image cannot be empty"
            )

        # Require exact 256-bit hexadecimal demo key.
        if len(args.signing_key_hex) != 64:

            # Reject ambiguous demo key width.
            raise SystemExit(
                "--signing-key-hex must contain exactly 64 hexadecimal characters"
            )

        # Decode hexadecimal demonstration key.
        try:

            # Convert human configuration into exact bytes.
            key = bytes.fromhex(
                args.signing_key_hex
            )
        except ValueError as exc:

            # Reject non-hexadecimal configuration.
            raise SystemExit(
                "--signing-key-hex contains non-hexadecimal characters"
            ) from exc

        # Build unsigned metadata with the required placeholder width.
        unsigned = _manifest(
            image=image,
            version_counter=args.version_counter,
            version=args.version,
            key_id=args.key_id,
            algorithm=FirmwareSignatureAlgorithm.DEMO_HMAC_SHA256,
            signature=bytes(32),
        )

        # Sign with the simulator/test-only HMAC backend.
        manifest = demo_sign_firmware_manifest(
            unsigned,
            key,
        )

        # Encode one complete package.
        package = encode_firmware_package(
            FirmwarePackage(
                manifest=manifest,
                image=image,
            )
        )

        # Write exact package bytes.
        args.output.write_bytes(
            package
        )

        # Print concise package metadata.
        print(
            f"Created demo package: {args.output}"
        )

        # Print signed image size.
        print(
            f"Image bytes: {len(image)}"
        )

        # Print exact SHA-256 digest.
        print(
            f"SHA-256: {manifest.image_sha256.hex()}"
        )

        # Print monotonic version counter.
        print(
            f"Version counter: {manifest.version_counter}"
        )

        # Report successful package creation.
        return 0

    # Read the production image for prepare/assemble modes.
    image = _read(
        args.image,
        "firmware image",
    )

    # Reject empty image.
    if not image:

        # Require real candidate content.
        raise SystemExit(
            "firmware image cannot be empty"
        )

    # Prepare exact bytes for external Ed25519 signing.
    if args.mode == "prepare":

        # Build metadata using a zeroed 64-byte placeholder signature.
        manifest = _manifest(
            image=image,
            version_counter=args.version_counter,
            version=args.version,
            key_id=args.key_id,
            algorithm=FirmwareSignatureAlgorithm.ED25519,
            signature=bytes(64),
        )

        # Build the exact domain-separated signing transcript.
        canonical = canonical_firmware_manifest(
            manifest
        )

        # Write exactly sixty-four bytes for the external signer.
        args.output.write_bytes(
            canonical
        )

        # Print concise production signing metadata.
        print(
            f"Created signing input: {args.output}"
        )

        # Print exact input size.
        print(
            f"Signing input bytes: {len(canonical)}"
        )

        # Print image digest for independent verification.
        print(
            f"Image SHA-256: {manifest.image_sha256.hex()}"
        )

        # Report successful signing-input creation.
        return 0

    # Read the externally generated Ed25519 signature.
    signature = _read(
        args.signature,
        "signature",
    )

    # Require the standard Ed25519 signature width.
    if len(signature) != 64:

        # Reject malformed external signature files.
        raise SystemExit(
            "external Ed25519 signature must contain exactly 64 raw bytes"
        )

    # Build the externally signed production manifest.
    manifest = _manifest(
        image=image,
        version_counter=args.version_counter,
        version=args.version,
        key_id=args.key_id,
        algorithm=FirmwareSignatureAlgorithm.ED25519,
        signature=signature,
    )

    # Encode the complete signed package.
    package = encode_firmware_package(
        FirmwarePackage(
            manifest=manifest,
            image=image,
        )
    )

    # Write the exact package bytes.
    args.output.write_bytes(
        package
    )

    # Print concise package metadata.
    print(
        f"Created externally signed package: {args.output}"
    )

    # Print the signed image digest.
    print(
        f"Image SHA-256: {manifest.image_sha256.hex()}"
    )

    # Report successful package assembly.
    return 0


# Execute the tool when run directly.
if __name__ == "__main__":

    # Exit using the build result.
    raise SystemExit(
        main()
    )
