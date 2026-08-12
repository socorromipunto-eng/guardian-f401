# Guardian F401 Device Simulator

M2 implements a software Guardian device that speaks the same binary protocol planned for the STM32F401 firmware.

## Why the Simulator Exists

The host console must not depend on unfinished physical hardware.

```text
guardianctl
     |
     v
Guardian Protocol
     |
     +------> Python Simulator
     |
     `------> STM32F401
```

Both endpoints will expose the same command contract.

## Supported M2 Commands

- `PING`
- `DEVICE_INFO`
- `GET_STATUS`

## Development Transport

M2 uses TCP only as a convenient local byte transport.

The Guardian Protocol itself remains independent from TCP.

The simulator binds to `127.0.0.1` by default so it is not exposed to the local network.

## Run

From the repository root:

```text
python tools/run_simulator.py
```

Optional endpoint:

```text
python tools/run_simulator.py --host 127.0.0.1 --port 9401
```

Expected startup output:

```text
Guardian F401 simulator listening on 127.0.0.1:9401
Commands: PING, DEVICE_INFO, GET_STATUS
Press Ctrl+C to stop.
```

M3 will add `guardianctl`, which will connect to this endpoint and exercise the same request/response protocol used later by the physical STM32.
