# M2 — Device Simulator

## Objective

M2 creates a software implementation of Guardian device behavior before the physical STM32 transport is introduced.

## Architecture

```text
TCP byte stream
      |
      v
IncrementalParser
      |
      v
GuardianDevice
      |
      +--> PING
      +--> DEVICE_INFO
      `--> GET_STATUS
      |
      v
encode_frame()
      |
      v
TCP byte stream
```

TCP is only the M2 development transport.

Command semantics remain transport-independent.

## Security Posture

The simulator binds to loopback by default.

M2 intentionally does not add authentication.

This preserves a clean development sequence:

```text
M1 framing
   |
M2 simulated device
   |
M3 host console
   |
M4 physical UART
   |
...
   |
M10 authentication / authorization / anti-replay
```

The current project must therefore not represent M2 traffic as cryptographically protected.

## Robustness Demonstrated

Automated tests verify:

- extreme one-byte request fragmentation;
- multiple parser feed boundaries;
- CRC rejection;
- recovery after a damaged frame;
- bounded command payload parsing;
- unknown-command failure;
- invalid-payload failure;
- device diagnostics;
- real loopback TCP request/response behavior.

## Next Milestone

M3 will implement `guardianctl`.

Expected commands:

```text
guardianctl ping
guardianctl info
guardianctl status
```
