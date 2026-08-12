# Guardian F401 Device Simulator

The simulator implements Guardian commands plus M5 asynchronous telemetry.

## Run

```text
python tools/run_simulator.py
```

## Live Telemetry

In another terminal:

```text
python tools/guardianctl.py telemetry --period-ms 500 --count 10
```

The simulator generates deterministic synthetic values only for transport, parser, scheduling and UI verification.

Real sensor acquisition remains M6.

The simulator binds to `127.0.0.1` by default.
