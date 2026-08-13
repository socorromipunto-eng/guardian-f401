# Guardian F401 Device Simulator

M10 secure mode is opt-in so earlier compatibility demos remain unchanged.

Start secure mode:

```text
python tools/run_simulator.py --secure
```

The intentionally public test/demo PSK is:

```text
00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f
```

Set the same key in guardianctl:

```text
$env:GUARDIAN_PSK_HEX="00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f"
python tools/guardianctl.py security authenticate
python tools/guardianctl.py baseline start --samples 64
```

Never use the simulator demo key on physical hardware.
