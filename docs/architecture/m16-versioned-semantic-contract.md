# M16 Versioned Semantic Contract Architecture

Status: CANDIDATE

## Purpose

This document translates ADR-M16-002 into an engineering model for future
multi-chip Guardian development.

## Resolution pipeline

Guardian interpretation SHALL conceptually resolve:

Node Identity
→ Chip Family
→ Hardware Revision
→ Firmware/Platform Version
→ Protocol Version
→ Semantic Profile
→ Compatibility Contract
→ Platform-local Meaning
→ System-level Interpretation
→ Authority Boundary

No step in this chain grants actuation merely because the previous step
succeeded.

## Core distinction

### Wire Grammar
Defines how bytes and fields are represented.

### Platform Grammar
Defines what those fields mean for one platform/version context.

### System Semantics
Defines what Guardian may infer or authorize from that meaning.

A translator that understands multiple wire grammars but does not know each
platform grammar is not sufficient for high-assurance interpretation.

## Versioned meaning

If Profile 1 defines:

A = TR1

and Profile 2 later defines:

A = TR2

Guardian SHALL preserve both meanings and the boundary at which Profile 2
became effective.

Historical Profile 1 data SHALL remain interpreted under Profile 1.

Runtime logic SHOULD resolve the current applicable profile directly. History
is retained for audit and reconstruction, not replayed on every operation.

## Multi-chip and mixed-version sets

A Guardian system may contain:
- different chip families;
- multiple instances of the same family;
- different silicon revisions;
- different firmware versions;
- different semantic profiles.

Semantic resolution SHALL be per node.

Same-family nodes are not automatically semantically equivalent.

## Compatibility model

A future machine-readable compatibility mechanism SHOULD be able to represent:

(platform A, semantic profile X)
compatible-with
(platform B, semantic profile Y)

and explicit incompatibility.

Unknown combinations SHALL remain unknown; they SHALL NOT be converted into
compatible by inference.

## Semantic change boundary record

Every material semantic change SHOULD record:
- semantic profile identifier;
- predecessor profile;
- affected platform;
- applicable version range;
- changed concepts;
- compatibility effects;
- transition rule;
- rollback rule;
- decision reference;
- evidence reference;
- validation reference.

## Anti-manipulation invariants

- Validators validate; they do not silently migrate.
- Migration is explicit and separately governed.
- Historical records are immutable in meaning.
- New profiles apply only from their declared boundary forward.
- A PASS does not imply certification or complete semantic coverage.
- AI may advise but cannot redefine normative platform grammar or authority.

## M16 application

Before NodeSupervisor becomes an implementation target, M16 SHALL identify the
semantic profile boundaries for at least:
- STM32F401 node role;
- MCXN947 supervisory role;
- NodeLink protocol version;
- shared state vocabulary;
- state interpretation differences;
- version/profile compatibility behavior;
- unknown-profile failure behavior.

The initial shared state names BOOT, DISCOVERING, ACTIVE, DEGRADED, SAFE_HOLD
and FAULT SHALL NOT be assumed to have identical platform-local semantics merely
because their wire values are shared.

## Non-claims

This document does not demonstrate:
- a complete semantic profile registry;
- runtime semantic resolution;
- hardware revision attestation;
- automatic firmware-version discovery;
- production compatibility matrices;
- physical MCXN947 validation;
- certification.
