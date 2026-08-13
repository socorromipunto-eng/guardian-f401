# Firmware

M12 adds a transport-independent secure firmware lifecycle on top of M10 authenticated ADMIN commands.

Portable lifecycle source:

```text
firmware/Firmware/Inc/guardian_firmware_lifecycle.h
firmware/Firmware/Src/guardian_firmware_lifecycle.c
```

The module implements:

```text
signed release manifest parsing
sequential candidate staging policy
SHA-256 image verification
trusted signature-verifier callback
monotonic anti-rollback policy
pending activation state
local boot confirmation
safe rollback state
```

Production firmware embeds no private signing key.

Board integration must supply staging storage, trusted public-key verification, atomic pending-image metadata and atomic rollback-floor persistence.

See `docs/m12-firmware-lifecycle.md`.
