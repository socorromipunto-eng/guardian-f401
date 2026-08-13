# M12 — Secure Firmware Lifecycle

## Objective

M12 completes the planned Guardian F401 security roadmap with a transport-independent signed firmware lifecycle.

```text
offline image
    |
    v
SHA-256 digest
    |
signed release manifest
    |
    v
ADMIN authenticated upload
    |
    v
sequential staging storage
    |
    v
digest verification
    |
trusted signature verification
    |
anti-rollback version check
    |
    v
PENDING_ACTIVATION
    |
    +--> boot succeeds -> application CONFIRM -> rollback floor advances
    |
    `--> boot fails -> previous confirmed image remains authoritative
```

## Security Design

The firmware lifecycle deliberately does **not** implement a new asymmetric cryptographic primitive.

Portable C receives a platform callback:

```text
guardian_firmware_verify_signature_fn
```

The callback receives:

```text
signature algorithm
trusted key id
exact 64-byte canonical signed manifest
signature bytes
```

Production integration should map algorithm `0x01` to an Ed25519 verifier backed by trusted public-key storage.

This keeps key storage and cryptographic-library selection outside the protocol parser and lifecycle state machine.

## Why the Simulator Uses HMAC

The software simulator includes algorithm `0xFE`:

```text
DEMO_HMAC_SHA256
```

It exists only so the complete package/upload/rollback workflow can be tested using Python's standard library.

It is intentionally unsuitable as the production firmware-signing architecture because the verifier would need the same secret used to sign releases.

The demo key is public and must never be provisioned on hardware.

## Signed Manifest

The canonical signed transcript is exactly 64 bytes:

```text
"GF-M12-IMAGE"              12 bytes
schema                       1 byte
algorithm                    1 byte
key id                       4 bytes
monotonic version counter    4 bytes
semantic version             6 bytes
image size                   4 bytes
image SHA-256               32 bytes
```

The signature field itself is excluded from the signed transcript.

The image is bound by its complete SHA-256 digest.

## Production Signing Flow

Create the exact signing input:

```text
python tools/build_firmware_package.py prepare firmware.bin \
  --output firmware.manifest.bin \
  --version-counter 120 \
  --version 1.2.0 \
  --key-id 1
```

The output is exactly 64 bytes.

Sign those bytes offline using the production Ed25519 private key.

The private key should not exist in the device repository or firmware build tree.

Assemble the package:

```text
python tools/build_firmware_package.py assemble firmware.bin firmware.sig \
  --output firmware.gfu \
  --version-counter 120 \
  --version 1.2.0 \
  --key-id 1
```

The `firmware.sig` file must contain exactly 64 raw Ed25519 signature bytes.

## Simulator Demo

Start the secure simulator:

```text
python tools/run_simulator.py --secure
```

Configure the public M10 simulator PSK:

```text
$env:GUARDIAN_PSK_HEX="00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f"
```

Build a deterministic simulator-only signed package:

```text
python tools/build_firmware_package.py demo --output demo-m12.gfu
```

Upload using an authenticated ADMIN session:

```text
python tools/guardianctl.py --role admin firmware upload demo-m12.gfu --activate
```

Inspect lifecycle state:

```text
python tools/guardianctl.py firmware status
```

Expected state after upload with `--activate`:

```text
PENDING_ACTIVATION
```

## Why Boot Confirmation Is Not a Host Command

M12 does not expose a remote `firmware confirm` command.

Confirmation advances the rollback floor and makes older firmware permanently unacceptable.

That decision belongs to the newly booted firmware after local startup/self-test criteria pass.

Portable application integration exposes:

```text
guardian_firmware_app_confirm_boot()
```

A failed pending boot uses:

```text
guardian_firmware_app_report_boot_failure()
```

The previous confirmed version remains authoritative.

## Staging Backend

The portable lifecycle requires platform callbacks for:

```text
erase staging storage
write candidate bytes
calculate staged SHA-256
verify trusted signature
persist pending activation metadata
persist rollback floor
```

The reference module does not assume an STM32 flash address map.

That is intentional because production flash layout, bootloader placement, page/sector ownership and recovery strategy must be selected together.

## Transfer Policy

M12 chunks carry at most:

```text
192 image bytes
```

This fits inside one M10 authenticated request while preserving the M1 256-byte payload bound.

Chunks are strictly sequential:

```text
offset == bytes_received
```

M12 rejects:

```text
duplicate chunks
overlapping chunks
skipped offsets
writes beyond signed image size
finalize before all image bytes arrive
```

## Rollback Policy

The candidate monotonic version counter must be strictly greater than both:

```text
confirmed active version
persisted rollback floor
```

Semantic `MAJOR.MINOR.PATCH` is display/release metadata.

Rollback decisions use only the explicit unsigned monotonic version counter.

The rollback floor advances only after boot confirmation.

The platform `complete_pending` callback must persistently clear/finalize pending-image metadata after both confirmed and failed boot outcomes. A failed boot is not considered safely rolled back until that persistent transition succeeds.

## Firmware Lifecycle States

```text
IDLE
RECEIVING
VERIFIED
PENDING_ACTIVATION
CONFIRMED
ROLLED_BACK
FAILED
```

A signature or hash failure enters `FAILED`.

A failed pending boot enters `ROLLED_BACK` without advancing the rollback floor.

## Hardware Integration Gate

Before claiming a physical secure-update implementation, complete all of these:

```text
select and document bootloader + application flash layout
select staging storage strategy
integrate trusted Ed25519 public-key storage
implement flash erase/write bounds
implement staged-image SHA-256
persist pending image metadata atomically
persist rollback floor atomically
implement reset/power-loss recovery
define boot-attempt counter and fallback policy
run Keil target build
run power-cut testing during erase/write/metadata updates
verify corrupted/forged images never boot
verify older signed images remain blocked after confirmation
```

## Threats Addressed

M12 rejects or prevents:

```text
modified candidate image bytes
manifest/image digest mismatch
untrusted firmware signatures
older signed firmware after rollback-floor advancement
unauthenticated firmware transport commands
non-ADMIN authenticated firmware mutation commands
chunk overlap/gap ambiguity
remote rollback-floor confirmation
```

## Boundaries

M12 is a lifecycle policy and transport integration layer.

It does not by itself provide:

```text
a complete STM32 bootloader
a fixed flash partition map
a production Ed25519 library
private-key management
encrypted firmware images
confidentiality of firmware bytes
hardware root-of-trust provisioning
```

Those platform decisions remain explicit rather than hidden in protocol code.
