# Guardian F401 Device Simulator

M12 extends secure simulator mode with signed firmware staging, verification, activation and rollback-policy state.

Start the secure simulator:

```text
python tools/run_simulator.py --secure
```

The intentionally public M10 simulator PSK is:

```text
00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f
```

The intentionally public M12 simulator-only firmware signing key is:

```text
7f6e5d4c3b2a1908172635445362718090a1b2c3d4e5f60718293a4b5c6d7e8f
```

Build a simulator package:

```text
python tools/build_firmware_package.py demo --output demo-m12.gfu
```

Configure guardianctl:

```text
$env:GUARDIAN_PSK_HEX="00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f"
```

Upload and mark the candidate pending activation:

```text
python tools/guardianctl.py --role admin firmware upload demo-m12.gfu --activate
```

Inspect lifecycle state:

```text
python tools/guardianctl.py firmware status
```

The simulator signing key is for tests only.

Never use either demonstration key on physical hardware.
