# M10 — Authenticated Sessions, Authorization and Anti-Replay

## Objective

M10 protects the state-changing command surface created by M8 and M9.

```text
PSK provisioning + secure nonce source
                |
                v
AUTH_BEGIN
client nonce -> device nonce + server proof
                |
                v
client verifies device
                |
                v
AUTH_FINISH
client proof
                |
                v
derived session key
                |
                v
SECURE_COMMAND
HMAC + uint64 counter + role policy
                |
                v
BASELINE_CONTROL / CONTROL_COMMAND
```

## Threat Model Addressed

M10 is designed to reject:

```text
forged privileged commands
privileged commands from an insufficient role
captured privileged request replay
message payload modification
outer request-sequence substitution
wrong-session message substitution
```

M10 does not encrypt traffic.

An observer can still read protocol payloads on the transport.

Public read-only queries and asynchronous telemetry remain outside the authenticated secure envelope in M10.

## Key Material

Production firmware contains no repository PSK.

The application exposes:

```text
guardian_firmware_app_configure_security()
```

The caller must provide:

```text
32-byte high-entropy PSK
maximum role assigned to that PSK
cryptographically strong random-byte callback
session timeout
```

The firmware application enables the privileged-command security gate before provisioning.

Therefore an unprovisioned M10 firmware application rejects direct baseline/control commands rather than silently falling back.

## Nonce Requirement

`AUTH_BEGIN` requires unpredictable device nonce material from the platform.

The M10 firmware does not derive cryptographic nonces from uptime, sequence numbers, ADC samples or other predictable values.

If the nonce callback fails, authentication fails closed.

## Mutual PSK Authentication

The device returns:

```text
SERVER_PROOF =
HMAC-SHA-256(
    PSK,
    "GF-M10-SERVER" || transcript
)[0:16]
```

The client verifies the server proof before sending:

```text
CLIENT_PROOF =
HMAC-SHA-256(
    PSK,
    "GF-M10-CLIENT" || transcript
)[0:16]
```

The per-session key is:

```text
SESSION_KEY =
HMAC-SHA-256(
    PSK,
    "GF-M10-SESSION" || transcript
)
```

Separate labels prevent one HMAC purpose from being reused as another.

## Authorization

Roles:

```text
OBSERVER
OPERATOR
ADMIN
```

M10 policy:

```text
BASELINE_CONTROL requires OPERATOR
CONTROL_COMMAND requires OPERATOR
```

`ADMIN` is reserved for later firmware-lifecycle security.

An `OBSERVER` can authenticate successfully while still receiving an authenticated `UNAUTHORIZED` result for operator-only commands.

## Anti-Replay

Each authenticated session starts with:

```text
next_counter = 1
```

The device accepts only:

```text
request.counter == next_counter
```

A valid authenticated request consumes the counter before application authorization.

The policy intentionally has no replay window because Guardian uses synchronous command-response exchanges.

A duplicate or skipped counter returns:

```text
REPLAY_DETECTED
```

After an ambiguous transport failure, guardianctl discards local session state and authenticates again.

## Session Replacement

A new `AUTH_BEGIN` creates only pending challenge state.

It does not destroy the current authenticated session.

The active session is replaced only after a correct `AUTH_FINISH` client proof.

This prevents unauthenticated challenge traffic from immediately invalidating an existing session.

## Session Expiry

The default inactivity timeout is:

```text
300 seconds
```

Expired session-key material is erased from runtime state.

Pending unauthenticated challenges expire after:

```text
30 seconds
```

## Constant-Time Verification

Portable C compares proof/tag bytes without early exit.

Sensitive C buffers use explicit volatile zeroization helpers.

## Simulator

M2-M9 compatibility mode remains the simulator default.

Enable M10 enforcement explicitly:

```text
python tools/run_simulator.py --secure
```

The simulator includes an intentionally public demonstration key:

```text
00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f
```

This key exists only for software demonstration and automated tests.

Never provision it on physical hardware.

PowerShell demo:

```text
$env:GUARDIAN_PSK_HEX="00112233445566778899aabbccddeeff102132435465768798a9bacbdcedfe0f"
python tools/guardianctl.py security authenticate
python tools/guardianctl.py baseline start --samples 64
python tools/guardianctl.py control arm
python tools/guardianctl.py security status
```

guardianctl can also accept:

```text
--psk-hex
--role observer
--role operator
--role admin
```

## Firmware Integration

Add:

```text
firmware/Security/Src/guardian_crypto.c
firmware/Security/Src/guardian_security.c
```

Include:

```text
firmware/Security/Inc
```

Before accepting privileged commands on hardware, provision a production key and nonce source through the application API.

The reference firmware already requires secure wrapping for:

```text
BASELINE_CONTROL
CONTROL_COMMAND
```

## Security Boundaries

M10 is PSK-based device authentication.

It is not per-user identity infrastructure.

It does not provide:

```text
payload encryption
authenticated asynchronous telemetry
certificate validation
secure firmware update
rollback protection
key rotation protocol
persistent anti-replay across power cycles
```

These boundaries remain explicit rather than being implied by CRC or sequence fields.

## Next Milestone

M11 focuses on robustness:

```text
parser fuzzing
secure-envelope fuzzing
fault injection
counter/tag corruption campaigns
bounded recovery
malformed authenticated payload tests
```
