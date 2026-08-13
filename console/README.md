# guardianctl

M13 keeps all previous guardianctl protocol, telemetry, health, control, security and firmware-lifecycle commands.

It also adds a separate read-only physical qualification runner:

```text
python tools/hardware_validation.py --serial-port COM5
```

The runner uses the existing public guardianctl serial transport and writes:

```text
hardware-validation.json
```

Default physical qualification observes:

```text
PING
DEVICE_INFO
GET_STATUS
GET_SECURITY_STATUS
GET_FIRMWARE_STATUS
GET_CONTROL_STATUS
telemetry
DSP features
health status
```

It deliberately excludes baseline changes, control actions, authentication and firmware upload.

Install the existing serial dependency first:

```text
python -m pip install -r console/requirements-serial.txt
```

See `docs/m13-hardware-validation.md`.
