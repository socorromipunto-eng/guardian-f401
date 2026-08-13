# guardianctl

M12 adds signed firmware package inspection and ADMIN-authenticated upload.

Public lifecycle status:

```text
python tools/guardianctl.py firmware status
```

Secure upload requires an M10 PSK and the `ADMIN` role:

```text
$env:GUARDIAN_PSK_HEX="00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f"
python tools/guardianctl.py --role admin firmware upload demo-m12.gfu
```

Explicitly mark the verified candidate pending activation:

```text
python tools/guardianctl.py --role admin firmware upload demo-m12.gfu --activate
```

The host validates `.gfu` framing, signed image size and SHA-256 before contacting the device.

Boot confirmation is deliberately not a remote guardianctl command because confirmation advances the device rollback floor.
