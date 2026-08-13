# Firmware

M9 adds transport-independent supervisory control.

```text
M8 health
local interlock
local run request
      |
      v
guardian_control
      |
      v
logical run permit
```

Host ARM cannot assert the permit directly.

Application integration APIs:

```text
guardian_firmware_app_set_local_run_request()
guardian_firmware_app_set_interlock_closed()
guardian_firmware_app_run_permit()
guardian_firmware_app_control_status()
```

Startup is safe-off with interlock open.

The logical output must be connected to final board-specific hardware by the product integration layer.

See `docs/m9-control.md`.
