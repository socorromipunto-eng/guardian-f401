# Guardian F401 — M16 NodeLink Normative Specification v0.2 (Adjudication Draft)

Status: APPROVED - HUMAN ADJUDICATED
Milestone: M16  
Decision scope: Node-to-node supervision protocol contract  
Implementation authority: NOT GRANTED by this document  
NodeSupervisor architecture authority: NOT GRANTED by this document  
NodeSupervisor implementation authority: NOT GRANTED by this document

## 1. Purpose

Guardian NodeLink is the bounded node-to-node supervision protocol for the
heterogeneous Guardian pair.

NodeLink is a distinct trust and semantic domain from Guardian Protocol v0.1.
It does not silently extend the existing host/device command and telemetry
contract.

This specification closes the protocol-level questions that must be resolved
before a NodeSupervisor architecture may consume peer traffic.

## 2. Governing invariants

The following implications are prohibited:

```text
PARSE_OK != AUTHENTICATED
CRC_VALID != AUTHENTICATED
AUTHENTICATED != FRESH
FRESH != TRUTHFUL
FRESH != AUTHORIZED
PROTOCOL_COMPATIBLE != SEMANTICALLY_COMPATIBLE
SEMANTICALLY_COMPATIBLE != AUTHORIZED
AUTHORIZED != ACTUATION_AUTHORIZED
REMOTE_STATE != LOCAL_STATE
REMOTE_MESSAGE != LOCAL_AUTHORITY
REMOTE_DISAGREEMENT != UNILATERAL_OVERRIDE
REMOTE_MESSAGE -> DIRECT_ACTUATION = PROHIBITED
AI/advisory != authority != actuation
```

Unknown or ambiguous state in an authority-relevant path fails closed.

## 3. Processing order

Incoming NodeLink content SHALL be processed in this order:

```text
1. RESOURCE_LIMITS
2. BOUNDED_PARSE
3. WIRE_CONTRACT_VALIDATION
4. CLOSED_CONTRACT_VALIDATION
5. PRODUCER_AND_KEY_RESOLUTION
6. CRYPTOGRAPHIC_AUTHENTICATION
7. PRE_FRESHNESS_SEMANTIC_COMPATIBILITY
8. EPOCH_RESOLUTION
9. REPLAY_AND_FRESHNESS_EVALUATION
10. FULL_MESSAGE_SEMANTIC_COMPATIBILITY
11. EVIDENCE_GENERATION
12. LOCAL_POLICY_EVALUATION
13. LOCAL_AUTHORITY_DECISION
14. OPTIONAL_LOCAL_ACTION
```

No stage implies success of any later stage.

A failure before stage 12 SHALL NOT produce an authority grant.

## 4. Wire contract

All multi-byte integer fields are unsigned big-endian.

### 4.1 Fixed header

| Offset | Size | Field | v0.2 rule |
|---:|---:|---|---|
| 0 | 1 | `magic_0` | MUST equal `0x47` (`G`) |
| 1 | 1 | `magic_1` | MUST equal `0x4E` (`N`) |
| 2 | 1 | `protocol_version` | MUST equal a supported NodeLink wire version |
| 3 | 1 | `message_type` | MUST resolve to a defined message type |
| 4 | 1 | `flags` | Unknown set bits MUST be rejected unless explicitly versioned |
| 5 | 1 | `node_state` | MUST resolve to the bounded state vocabulary |
| 6 | 2 | `payload_length` | MUST be within the governed payload bound |
| 8 | 4 | `sequence` | Zero is reserved; wrap is not continuation |
| 12 | 4 | `sender_epoch` | Authenticated assertion only; not proof of current epoch |
| 16 | 4 | `sender_node_id` | Transport-level numeric identifier only |

Header size remains 20 bytes for wire version 0x01.

### 4.2 Payload and trailer

The v0.1 payload bound of 64 bytes remains the current governed bound unless
separately changed by a versioned protocol decision.

The current trailer remains IEEE CRC32 over `header + payload`, encoded as
4-byte big-endian.

CRC32 provides corruption detection only.

```text
CRC_VALID != AUTHENTICATED
```

Maximum frame for wire version 0x01 remains 88 bytes.

## 5. Required semantic context

Wire version alone is insufficient for authority-relevant interpretation.

Every peer relationship SHALL resolve an effective semantic context sufficient
to determine, as applicable:

```text
NODE_IDENTITY
CHIP_FAMILY
HARDWARE_REVISION
PLATFORM_OR_BOOT_REVISION
FIRMWARE_VERSION
PROTOCOL_VERSION
SEMANTIC_PROFILE_VERSION
SYSTEM_ROLE
SECURITY_CONFIGURATION
COMPATIBILITY_CONTRACT_VERSION
```

Not every field must necessarily be present inside every frame. The binding MAY
be supplied by an authenticated session, credential, provisioning record or
other governed context, but the receiver SHALL be able to resolve the required
context before using peer data for supervision-, authority-, safety- or
actuation-relevant decisions.

If required semantic context cannot be resolved:

```text
SEMANTIC_CONTEXT = UNKNOWN
AUTHORITY_RELEVANT_USE = DENIED
```

## 6. Sender identity

`sender_node_id` remains a transport-level numeric identifier.

It is not:

- cryptographic proof of identity;
- a replacement for `producer_id`;
- a replacement for `key_id`;
- a trust-store credential binding;
- evidence that the peer has authority.

The receiver SHALL bind accepted peer traffic to a governed producer/key
resolution path before authentication can succeed.

## 7. Message vocabulary

The current bounded message vocabulary remains:

| Value | Message |
|---:|---|
| `0x01` | `HELLO` |
| `0x02` | `CHALLENGE` |
| `0x03` | `RESPONSE` |
| `0x04` | `HEARTBEAT` |
| `0x05` | `HEALTH` |
| `0x06` | `SUPERVISION_STATE` |
| `0x7F` | `ERROR` |

Unknown message types SHALL be rejected.

### 7.1 Authority meaning

`SUPERVISION_STATE` communicates remote observation, corroboration,
disagreement, challenge result, or recommendation only.

It does not carry direct actuator authority.

```text
SUPERVISION_STATE != ACTUATION_COMMAND
```

## 8. Operational state vocabulary

The bounded operational states remain:

| Value | State |
|---:|---|
| `0x00` | `BOOT` |
| `0x01` | `DISCOVERING` |
| `0x02` | `ACTIVE` |
| `0x03` | `DEGRADED` |
| `0x04` | `SAFE_HOLD` |
| `0x05` | `FAULT` |

Unknown states SHALL be rejected.

A remote state claim is evidence about the remote peer.

```text
REMOTE_STATE != LOCAL_STATE_TRANSITION
```

Local state transitions remain deterministic local policy decisions.

## 9. Authentication boundary

NodeLink wire integrity and cryptographic authentication remain separate
boundaries.

The cryptographic mechanism SHALL be provided through the M15 signing /
verification provider boundary or another separately adjudicated provider.

The exact authenticated transcript SHALL bind, at minimum, the fields needed to
prevent substitution of:

- protocol version;
- message type;
- sender identity context;
- epoch;
- sequence;
- semantic context or a cryptographic reference to that context;
- payload;
- purpose/domain.

Unauthenticated content MUST NOT:

- advance accepted freshness state;
- replace an accepted epoch;
- establish semantic compatibility;
- grant authority;
- authorize actuation.

## 10. Sequence rules

Sequence zero is reserved and rejected.

Within one initialized session, accepted sequence progression SHALL be strictly
monotonic unless a later version explicitly defines a bounded replay window.

Duplicate and regressing values fail freshness evaluation.

A large forward jump SHALL be handled by an explicit bounded gap policy and
must not be accepted merely because it is numerically greater.

The transition:

```text
0xFFFFFFFF -> 0x00000000
```

is not continuation of the same session.

```text
SEQUENCE_EXHAUSTION -> SESSION_REPLACEMENT_REQUIRED
SEQUENCE_WRAP_WITHIN_SAME_SESSION = FAIL_CLOSED
```

## 11. Epoch rules

`sender_epoch` is an authenticated peer assertion when covered by the
authenticated transcript.

Authentication of an epoch does not prove that it is:

- new;
- current;
- unique;
- monotonic;
- associated with a physical reboot.

The receiver SHALL distinguish:

```text
REMOTE_EPOCH_OBSERVED
REMOTE_EPOCH_ACCEPTED
```

An epoch transition requires a governed acceptance rule.

Unknown, ambiguous, reused, rollback-like or historically impossible epoch
transitions fail closed for authority-relevant processing.

## 12. Freshness and replay outcomes

The freshness evaluator SHALL distinguish at least:

```text
FRESH
DUPLICATE_REPLAY
SEQUENCE_REGRESSION
SEQUENCE_GAP_REQUIRES_POLICY
SESSION_REPLACEMENT_REQUIRED
EPOCH_TRANSITION_REQUIRES_ADJUDICATION
EPOCH_REUSE_REJECTED
PERSISTENT_STATE_UNAVAILABLE
PERSISTENT_STATE_CORRUPTED
PERSISTENT_STATE_TORN
PERSISTENT_ROLLBACK_SUSPECTED
FRESHNESS_UNKNOWN
REJOIN_REQUIRED
```

These are security/freshness conditions. They are not automatic additions to
the operational state vocabulary.

For authority-relevant use:

```text
FRESHNESS_UNKNOWN -> DENY_STRONGER_TRUST
REJOIN_REQUIRED -> SAFE_HOLD
```

until a governed recovery establishes acceptable state.

## 13. Persistent freshness

Cross-reboot anti-replay SHALL NOT be claimed from session sequence alone.

If cross-reboot replay resistance is required, a persistence provider SHALL
preserve a freshness anchor bound, as applicable, to:

- peer identity;
- accepted epoch;
- accepted sequence or equivalent monotonic state;
- semantic/compatibility context;
- freshness-rule version;
- integrity state.

The storage technology remains unselected by this specification.

Persistent state rollback detection remains NOT_DEMONSTRATED.

## 14. Semantic compatibility outcomes

Before authority-relevant use, the receiver SHALL produce one of these bounded
compatibility outcomes:

```text
COMPATIBLE
INCOMPATIBLE
UNKNOWN
UNSUPPORTED
HISTORICALLY_IMPOSSIBLE
```

Only `COMPATIBLE` permits progression to later policy evaluation.

All other outcomes fail closed for authority-relevant use.

Compatibility SHALL be evaluated twice where required:

1. `PRE_FRESHNESS_COMPATIBILITY` — enough context to interpret epoch, sequence
   and freshness semantics safely;
2. `FULL_MESSAGE_SEMANTIC_COMPATIBILITY` — compatibility of the complete
   message meaning for policy use.

## 15. Rejoin boundary

Rejoin is a separately governed recovery boundary.

```text
REJOIN_REQUIRED != ACCEPT_NEXT_SIGNED_MESSAGE
```

A rejoin procedure SHALL define, before implementation:

- peer identity requirements;
- credential/key state requirements;
- freshness-anchor establishment;
- epoch acceptance;
- semantic compatibility;
- audit evidence;
- authority restrictions during recovery;
- failure behavior.

This specification defines the requirement but does not select or implement the
rejoin mechanism.

## 16. Error behavior

The receiver SHALL produce bounded failure outcomes for at least:

- malformed magic;
- unsupported protocol version;
- unsupported flags;
- unknown message type;
- invalid node state;
- zero sequence;
- sequence duplicate;
- sequence regression;
- sequence exhaustion/wrap;
- payload over bound;
- length mismatch;
- CRC failure;
- unknown sender;
- key resolution failure;
- authentication failure;
- unknown semantic context;
- incompatible semantic context;
- unknown epoch state;
- rejected epoch transition;
- replay detection;
- freshness-state loss;
- unsupported compatibility combination.

Diagnostic telemetry MAY be retained after a failure, but failure SHALL NOT
silently become stronger trust.

## 17. Evidence contract

Every NodeLink processing attempt that reaches a security- or
authority-relevant decision point SHOULD be capable of producing a bounded
evidence record containing, as applicable:

```text
PROTOCOL_VERSION
MESSAGE_TYPE
PEER_IDENTITY_REFERENCE
KEY_ID_REFERENCE
EPOCH
SEQUENCE
SEMANTIC_PROFILE_REFERENCE
COMPATIBILITY_OUTCOME
AUTHENTICATION_OUTCOME
FRESHNESS_OUTCOME
PARSE_OR_VALIDATION_ERROR
LOCAL_POLICY_RESULT
AUTHORITY_RESULT
SOURCE_COMMIT_OR_FIRMWARE_VERSION
```

Evidence generation does not grant authority.

```text
EVIDENCE_PRESENT != CLAIM_DEMONSTRATED
```

## 18. Transport independence

NodeLink core semantics remain transport-independent.

UART, SPI, CAN, CAN-FD or another physical transport may be selected by a
platform adapter.

A transport adapter SHALL preserve the frame and semantic boundaries and SHALL
NOT redefine:

- message meaning;
- identity;
- freshness;
- semantic compatibility;
- authority;
- actuation semantics.

Transport selection remains DEFERRED.

## 19. STM32F401 adapter compatibility

The existing F401 adapter remains compatible with the v0.1 wire contract in the
following limited sense:

- sequence starts at 1;
- sequence advances after successful local transmit callback completion;
- wrap is not accepted;
- local node state is bounded;
- HELLO and HEARTBEAT generation are bounded;
- transport remains callback-based;
- callback success does not prove remote receipt or authority.

However, the existing adapter does not by itself establish:

- cryptographic peer authentication;
- persistent freshness;
- semantic-profile compatibility;
- rejoin;
- remote authority;
- physical validation.

Those gaps remain explicit.

## 20. Host validation requirements

Before NodeSupervisor architecture may be authorized to consume NodeLink,
host-level validation SHALL cover at minimum:

- existing v0.1 encode/decode and malformed-frame cases;
- unknown/unsupported semantic-profile combinations;
- protocol-compatible but semantically-incompatible peers;
- unauthenticated epoch must not update freshness state;
- authenticated but stale/replayed message rejection;
- duplicate and regressing sequence;
- sequence exhaustion;
- missing persistence state;
- corrupted/torn persistence state;
- rollback-suspected state;
- rejoin-required path;
- remote state cannot directly force local state;
- remote supervision cannot directly authorize actuation;
- evidence records preserve failure reason and applicability context.

Host validation remains distinct from physical validation.

## 21. NodeSupervisor consumption boundary

A future NodeSupervisor MAY consume only outputs that have passed the complete
NodeLink processing chain required for their intended use.

NodeSupervisor SHALL NOT consume raw or merely parsed frames as authoritative
state.

The minimum input contract for an authority-relevant NodeSupervisor path is:

```text
AUTHENTICATED
+ PRE_FRESHNESS_COMPATIBLE
+ FRESHNESS_ACCEPTABLE
+ FULL_SEMANTIC_COMPATIBLE
+ EVIDENCE_AVAILABLE
```

Even that result is only input to local policy.

```text
VALIDATED_NODELINK_INPUT != ACTUATION_AUTHORITY
```

## 22. Acceptance criteria for authority closure

NodeLink specification authority may be considered closed only when:

- NL-AC-01 wire layout is controlled and versioned;
- NL-AC-02 message and state vocabularies are closed for the active version;
- NL-AC-03 sender identity semantics are explicit;
- NL-AC-04 authenticated transcript requirements are explicit;
- NL-AC-05 semantic-profile resolution is explicit;
- NL-AC-06 freshness/replay outcomes are bounded;
- NL-AC-07 rejoin requirements are explicit;
- NL-AC-08 compatibility outcomes are bounded;
- NL-AC-09 error behavior is fail-closed for authority-relevant use;
- NL-AC-10 transport remains unable to redefine semantics;
- NL-AC-11 evidence outputs are defined;
- NL-AC-12 direct remote-message-to-actuation paths are prohibited;
- NL-AC-13 host tests exist for the above boundaries;
- NL-AC-14 document/register/governance authority is materialized and adjudicated.

## 23. Non-claims

This specification does not demonstrate or authorize:

- physical STM32F401 NodeLink execution;
- physical MCXN947 NodeLink execution;
- MCXN947 NodeLink implementation;
- physical dual-node timing;
- persistent anti-replay implementation;
- persistent-state rollback detection;
- persistent epoch storage implementation;
- secure-element integration;
- production key custody;
- transport selection;
- hardware fault tolerance;
- distributed quorum;
- production readiness;
- certification;
- AI advisory-plane implementation;
- NodeSupervisor architecture;
- NodeSupervisor implementation;
- actuation authority for any supervisory peer.

## 24. Adjudicated result

Human-adjudicated result:

```text
NODELINK_ARCHITECTURE_DIRECTION=ACCEPTED
NODELINK_WIRE_CONTRACT=ACCEPTED_WITH_VERSION_BOUNDARY
NODELINK_MESSAGE_VOCABULARY=ACCEPTED
NODELINK_STATE_VOCABULARY=ACCEPTED
NODELINK_AUTHENTICATION_BOUNDARY=ACCEPTED_WITH_PROVIDER_DEPENDENCY
NODELINK_FRESHNESS_CONTRACT=ACCEPTED_WITH_PERSISTENCE_GAP
NODELINK_SEMANTIC_COMPATIBILITY=ACCEPTED
NODELINK_REJOIN_REQUIREMENT=ACCEPTED_WITH_IMPLEMENTATION_GAP
NODELINK_TRANSPORT=DEFERRED
NODELINK_AUTHORITY_BOUNDARY=ACCEPTED
NODELINK_EVIDENCE_CONTRACT=ACCEPTED_WITH_IMPLEMENTATION_GAP
NODELINK_IMPLEMENTATION_AUTHORITY=NO
NODESUPERVISOR_ARCHITECTURE_AUTHORITY=NO
NODESUPERVISOR_IMPLEMENTATION_AUTHORITY=NO
```

Human adjudication of the NodeLink v0.2 specification content is complete. This approval establishes the specification decision only; it does not authorize NodeLink implementation, NodeSupervisor architecture, NodeSupervisor implementation, physical actuation, release, tag, publication, or certification.
