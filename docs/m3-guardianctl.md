# M3 — guardianctl

## Objective

M3 creates the first operator-facing Guardian host tool.

The console proves that the project is no longer only a protocol library: it is now a usable host/device system.

## Architecture

```text
Operator
   |
   v
guardianctl
   |
   +--> argument validation
   +--> text / JSON presentation
   |
   v
GuardianClient
   |
   +--> request sequence allocation
   +--> command payload decoding
   |
   v
GuardianTcpTransport
   |
   +--> bounded connect timeout
   +--> bounded response timeout
   +--> incremental stream parser
   +--> response correlation
   +--> remote ERROR handling
   |
   v
Guardian Protocol v0.1
   |
   v
M2 Device Simulator
```

## Implemented Commands

```text
guardianctl ping
guardianctl info
guardianctl status
```

## Failure Model

Expected failures do not produce Python tracebacks at the CLI boundary.

Instead:

```text
guardianctl: timed out waiting for Guardian device at 127.0.0.1:9401
```

or:

```text
guardianctl: device rejected command 0x01: INVALID_PAYLOAD
```

The process returns a non-zero status suitable for shell scripts and CI.

## Machine-Readable Output

`--json` provides automation-friendly output without changing the protocol layer.

## Security Boundary

M3 still communicates with the unauthenticated development simulator.

No command should be described as cryptographically authenticated yet.

Authentication, authorization and anti-replay protection remain M10.

## Next Milestone

M4 replaces the software-only transport path with the first physical STM32F401 UART integration.

The high-level command contract remains unchanged:

```text
GuardianClient
     |
     +--> TCP simulator
     |
     `--> UART STM32
```
