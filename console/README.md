# guardianctl

`guardianctl` is the host-side diagnostics and management console for Guardian F401.

M3 connects to the M2 software device using the same Guardian Protocol that will later travel over STM32 UART.

## Commands

```text
python tools/guardianctl.py ping
python tools/guardianctl.py info
python tools/guardianctl.py status
```

Default endpoint:

```text
127.0.0.1:9401
```

## Start the Simulator

Terminal 1:

```text
python tools/run_simulator.py
```

Expected:

```text
Guardian F401 simulator listening on 127.0.0.1:9401
Commands: PING, DEVICE_INFO, GET_STATUS
Press Ctrl+C to stop.
```

## Use guardianctl

Terminal 2:

```text
python tools/guardianctl.py ping
```

Example:

```text
Guardian device reachable
Endpoint: 127.0.0.1:9401
Reply: PONG
Latency: 0.85 ms
```

Metadata:

```text
python tools/guardianctl.py info
```

Example:

```text
Model: Guardian-F401-SIM
Firmware: 0.2.0
Device ID: F4010001
Protocol: 0.1
```

Runtime status:

```text
python tools/guardianctl.py status
```

Example:

```text
State: IDLE
Uptime: 00:00:12
RX frames: 3
TX frames: 2
Protocol errors: 0
Last error: 0x00
```

## JSON Output

Every M3 command can emit stable machine-readable JSON:

```text
python tools/guardianctl.py --json info
python tools/guardianctl.py --json status
```

## Custom Endpoint

```text
python tools/guardianctl.py --host 127.0.0.1 --port 9401 --timeout 2.0 ping
```

## Design

```text
CLI
 |
 v
GuardianClient
 |
 v
SequenceManager
 |
 v
GuardianTcpTransport
 |
 v
Guardian Protocol
 |
 v
M2 Device Simulator
```

M4 will add an STM32 UART transport while keeping the high-level Guardian client contract.
