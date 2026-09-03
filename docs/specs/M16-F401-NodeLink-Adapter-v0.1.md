# M16 F401 NodeLink Adapter v0.1

Status: CANDIDATE

## Purpose

This adapter binds the portable Guardian NodeLink core to the STM32F401
application role without selecting a physical NodeLink transport.

The existing Guardian host/device protocol remains bound to its existing
USART2 path.

NodeLink transport selection is intentionally deferred.

## Responsibilities

The adapter:

- owns the F401 NodeLink sequence counter;
- owns a session-local sender epoch supplied by the caller;
- owns the F401 numeric NodeLink sender identifier;
- stores the local NodeLink state;
- generates bounded HELLO and HEARTBEAT frames;
- uses a caller-supplied frame transmit callback;
- remains inert until a transmit callback is configured;
- never grants actuation authority.

## Non-responsibilities

The adapter does not:

- configure UART, SPI, CAN or CAN-FD;
- provide cryptographic authentication;
- provide persistent freshness;
- prove remote-node identity;
- interpret supervisor messages as actuation authority;
- alter Guardian Protocol v0.1;
- perform physical hardware validation.

## Heartbeat policy

The initial default heartbeat interval is 1000 ms.

A heartbeat becomes due only after the configured interval has elapsed.
At most one heartbeat is emitted per poll invocation.

The sequence starts at 1 and increments after a successful transmit.

Sequence wrap is not accepted. If the sequence reaches UINT32_MAX and is
successfully transmitted, no later heartbeat may be generated in the same
session.

## Transport callback

The adapter transmits one complete encoded NodeLink frame per callback.

This is a frame-level contract.

A later platform adapter is responsible for mapping a complete frame onto a
specific UART/SPI/CAN/CAN-FD transport and for preserving framing semantics.

## Authority boundary

Successful transport delivery means only that the local adapter handed a
frame to the configured transport callback.

It does not prove:

- remote receipt;
- remote authentication;
- remote acceptance;
- remote agreement;
- authority;
- actuation.

## Evidence boundary

Host tests demonstrate portable C behavior of this adapter.

Arm/Keil integration and physical STM32F401 execution are separate evidence
boundaries.
