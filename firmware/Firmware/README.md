# M12 Firmware Lifecycle

Portable M12 source:

```text
firmware/Firmware/Inc/guardian_firmware_lifecycle.h
firmware/Firmware/Src/guardian_firmware_lifecycle.c
```

The module owns:

```text
signed manifest validation
sequential staging policy
SHA-256 comparison
signature-verification callback boundary
monotonic anti-rollback policy
pending activation state
persistent pending-image completion
boot confirmation / safe rollback state
public lifecycle diagnostics
```

It does not own flash addresses or a private/public-key implementation.

Production STM32 integration must supply bounded platform callbacks and trusted Ed25519 verification.

See `docs/m12-firmware-lifecycle.md`.
