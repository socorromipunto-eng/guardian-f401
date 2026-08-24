ADR-M15-001 — Node Identity and Signed Assurance Messages



Status: DRAFT

Milestone: M15

Decision Type: Architecture / Security / Assurance

Target: Guardian F401

Depends on: M14 Canonical Envelope and Closed Payload Contracts

Supersedes: None

Implementation status: NOT IMPLEMENTED



1\. Context



Guardian F401 currently provides a bounded assurance layer based on strict validation, closed payload contracts, deterministic canonicalization, and explicit separation between advisory information and physical authority.



M14 established canonical assurance objects for:



Observation

Decision

Witness



The canonical envelope currently carries identity-related claims including:



producer\_id

producer\_epoch

logical\_time

object\_id



These fields are structurally validated, but they do not currently constitute cryptographic proof of producer identity.



A valid value such as:



{

&#x20; "producer\_id": "guardian-f401-primary"

}



states an identity claim.



It does not prove that the object was produced by the Guardian node associated with that identity.



M15 therefore introduces the architectural boundary between:



validated data



and



authenticated producer statements.



This ADR defines the node-identity and signed-message architecture required to establish that boundary.



It does not define persistent freshness, quorum, distributed failover, physical authority, or production key-storage hardware.



2\. Architectural principle



Guardian preserves the following ordering:



Untrusted Input

&#x20;     ↓

Bounded Parsing

&#x20;     ↓

Strict Validation

&#x20;     ↓

Closed Contract

&#x20;     ↓

Canonicalization

&#x20;     ↓

Cryptographic Authentication

&#x20;     ↓

Freshness Evaluation

&#x20;     ↓

Policy Evaluation

&#x20;     ↓

Authority Decision



The fundamental rule is:



Validation precedes signature evaluation for assurance semantics.



Cryptographic authentication must not cause malformed, unbounded, ambiguous, or contract-invalid input to become acceptable.



Likewise:



A valid signature does not grant authority.



The following properties remain distinct:



VALIDATED

&#x20;   ≠

AUTHENTICATED

&#x20;   ≠

FRESH

&#x20;   ≠

TRUTHFUL

&#x20;   ≠

AUTHORIZED



No transition between those states is implicit.



3\. Decision



Guardian shall introduce a cryptographically bound node-identity model for assurance messages.



The target production signature architecture shall use an asymmetric per-node identity model.



The current architectural target is Ed25519, consistent with the existing M12 production firmware-signature design direction.



However, M15 shall keep cryptographic key custody behind an explicit provider boundary.



The assurance layer shall not assume that a private key resides directly in STM32F401 application flash.



4\. Node identity model



A Guardian node identity consists conceptually of:



Guardian Node Identity

├── producer\_id

├── key\_id

├── signature\_algorithm

├── public verification credential

├── private signing credential

├── identity lifecycle state

└── provisioning evidence



These elements have different meanings and must not be conflated.



4.1 producer\_id



producer\_id identifies the logical producer represented by an assurance object.



It is a claim until cryptographically bound to a trusted credential.



A syntactically valid producer\_id alone proves nothing about the actual producer.



Therefore:



producer\_id ≠ cryptographic identity

5\. STM32 factory UID



The STM32F401 factory UID may be used as a hardware attribute during provisioning or inventory.



It shall not be treated as:



a secret;

a private key;

a cryptographic credential;

proof of possession;

authorization evidence.



Therefore:



Factory UID ≠ secret

Factory UID ≠ private key

Factory UID ≠ signature

Factory UID ≠ authority



The UID may participate in a provisioning record that binds physical hardware to a logical Guardian identity, but such a binding requires independent evidence.



6\. device\_id



Existing Guardian device\_id values may continue to identify devices within existing application or protocol contexts.



M15 shall not silently redefine an existing device\_id as a cryptographic identity.



Any relationship between:



device\_id

producer\_id

factory UID

cryptographic key



must be explicit and documented.



No equality between those concepts shall be inferred merely because values happen to be derived from the same device.



7\. Key identity



Signed assurance messages require an explicit key\_id.



key\_id identifies the trusted verification credential associated with a signature.



It shall support:



key replacement;

key rotation;

multiple historical verification keys;

algorithm migration;

revocation;

forensic reconstruction.



A producer\_id therefore may have more than one key\_id during its lifecycle.



Conceptually:



producer\_id

&#x20;  │

&#x20;  ├── key\_id 1 → retired

&#x20;  ├── key\_id 2 → active

&#x20;  └── key\_id 3 → future



Key identifiers shall not contain private key material.



8\. Signature algorithm



The target production algorithm for this architecture is:



Ed25519



The exact algorithm identifier shall be versioned and explicitly encoded.



M15 shall not infer a signature algorithm from signature length or key length.



Algorithm agility must be explicit.



No silent fallback to another algorithm is permitted.



9\. HMAC



Guardian already contains SHA-256 and HMAC-SHA-256 primitives.



Those primitives remain valid for the security contexts for which they were designed.



However, shared-secret HMAC shall not be treated as the target federated node-identity architecture.



A shared secret does not provide the desired separation between independent future Guardian producers.



Compromise of a shared authentication secret may allow one participant to impersonate another participant sharing that secret.



Therefore:



HMAC may support explicitly scoped symmetric authentication mechanisms, testing, or existing protocol functions, but it shall not silently substitute for the M15 production node-signature architecture.



Existing M12 DEMO\_HMAC\_SHA256 remains a demonstration mechanism and shall not become a production identity mechanism.



10\. Identity profiles



Guardian shall distinguish at least two identity profiles.



10.1 Development / Test Identity



Permitted for:



unit testing;

integration testing;

CI;

simulation;

host validation;

interoperability development.



Software-accessible private keys may be used only where their non-production status is explicit.



Test credentials shall be distinguishable from production credentials.



They must never silently become production identities.



10.2 Production Identity



Production identity requires an approved key-provisioning and custody mechanism.



Possible implementations may include:



secure element;

external cryptographic device;

protected provisioning mechanism;

future hardware security capability;

another reviewed key-storage architecture.



This ADR does not select that backend.



Until such a mechanism is implemented and validated:



Guardian shall not claim non-extractable per-device production identity.



11\. Signing provider boundary



The assurance layer shall not directly depend on private-key storage implementation.



Signing shall be exposed through an abstraction conceptually equivalent to:



sign(

&#x20;   algorithm,

&#x20;   key\_id,

&#x20;   canonical\_transcript

) -> signature



The provider owns:



private-key access;

key-storage mechanism;

cryptographic implementation;

hardware integration;

failure reporting.



The assurance layer owns:



validation;

canonicalization;

transcript construction;

identity metadata;

policy-independent verification semantics.



This separation allows the assurance logic to remain portable across future Guardian hardware.



12\. Verification provider boundary



Verification shall likewise be abstracted conceptually as:



verify(

&#x20;   producer\_id,

&#x20;   algorithm,

&#x20;   key\_id,

&#x20;   canonical\_transcript,

&#x20;   signature

) -> verification\_result



The verifier must resolve the trusted public credential independently.



The message itself shall never be authoritative about which public key should be trusted.



In particular:



An attacker-controlled message must not be able to introduce its own public key and thereby establish its own trust.



13\. Signed transcript



Guardian shall not sign an arbitrary in-memory representation.



The signature shall bind an exact deterministic byte representation derived from the accepted canonical object.



The conceptual sequence is:



raw bytes

&#x20;  ↓

strict parse

&#x20;  ↓

validate envelope

&#x20;  ↓

validate payload

&#x20;  ↓

RFC 8785 canonical representation

&#x20;  ↓

Guardian signature domain separation

&#x20;  ↓

signed transcript

&#x20;  ↓

signature



The exact byte-level transcript shall be frozen before implementation.



Any modification to the transcript format after release requires explicit versioning.



14\. Domain separation



Guardian signatures shall include explicit domain separation.



A signature valid for one Guardian purpose must not automatically be valid for another.



For example, signatures over:



firmware manifests

assurance observations

assurance decisions

witness statements

attestation exchanges



shall not be interchangeable merely because the same cryptographic algorithm is used.



Conceptually:



GUARDIAN / ASSURANCE / <version> / <object-purpose>



must participate in the signed transcript.



The final encoding will be defined before implementation.



15\. Existing M14 envelope



M15 shall preserve the semantic meaning of the existing M14 envelope unless a separately reviewed schema-version change is required.



Existing fields include:



domain

object\_type

producer\_id

producer\_epoch

object\_id

logical\_time

payload



M15 must not silently add optional security semantics to an existing closed contract.



If signature metadata requires an envelope change, the resulting contract shall receive an explicit new version.



Closed-schema behavior remains mandatory.



16\. producer\_epoch



producer\_epoch identifies a producer execution/lifecycle epoch according to the Guardian contract.



M15 authentication may cryptographically bind producer\_epoch into the signed transcript.



However:



A signed producer\_epoch does not by itself prove freshness.



Persistent interpretation across reset belongs to the subsequent anti-replay/freshness milestone.



M15 therefore authenticates the claimed epoch without yet claiming that the receiver can prove that the epoch is current.



17\. logical\_time



Likewise, logical\_time shall be covered by authentication where present.



But:



signed logical\_time ≠ trusted wall-clock time

signed logical\_time ≠ persistent freshness



Persistent anti-replay semantics are explicitly outside the scope of this ADR.



18\. Verification order



Incoming signed assurance data shall follow a bounded processing order.



At minimum:



1\. Apply raw-input resource limits

2\. Parse using strict UTF-8/JSON rules

3\. Reject duplicate members

4\. Validate closed envelope

5\. Validate closed payload contract

6\. Construct deterministic canonical representation

7\. Resolve trusted producer/key binding

8\. Verify signature

9\. Return explicit authentication state



Freshness checks will be introduced by a subsequent ADR.



Policy and authority evaluation remain downstream.



19\. Failure taxonomy



Authentication failures must not corrupt the existing M14 input-validation taxonomy.



Guardian shall distinguish at least:



INPUT\_INVALID

AUTHENTICATION\_FAILED

IDENTITY\_UNKNOWN

KEY\_UNKNOWN

ALGORITHM\_UNSUPPORTED

CRYPTO\_PROVIDER\_FAILURE



Exact identifiers remain an implementation decision.



An unavailable cryptographic backend must not be reported as malformed user input.



Likewise, a structurally invalid message must not be reported merely as a bad signature.



This preserves the M14 principle:



Environment and implementation failures are not input-validation failures.



20\. Key replacement and revocation



The architecture must permit a producer credential to be replaced without changing the logical identity of the producer.



Conceptually:



producer\_id = stable logical identity

key\_id      = particular cryptographic credential



Verification policy must be able to represent at least:



ACTIVE

RETIRED

REVOKED

UNKNOWN



The exact persistent trust-store implementation is deferred.



Revocation distribution for disconnected systems is also deferred and requires separate threat analysis.



21\. Authority invariant



Authentication introduces no new actuator authority.



The following implication is explicitly forbidden:



signature\_valid == true

&#x20;       ↓

actuator\_authorized == true



Instead:



signature\_valid

&#x20;       ↓

authenticated statement

&#x20;       ↓

freshness evaluation

&#x20;       ↓

evidence / corroboration

&#x20;       ↓

deterministic policy

&#x20;       ↓

explicit authority decision

&#x20;       ↓

physical gate



A correctly signed Decision remains data until an independently defined authority mechanism accepts it.



A correctly signed AI-generated recommendation remains advisory.



22\. Compromised signing node



This architecture does not claim to defend against every false statement produced by a legitimately compromised node.



A compromised node possessing its legitimate private credential may produce:



valid syntax;

valid canonical form;

valid signature;

semantically false information.



Therefore:



Cryptographic authenticity establishes provenance and integrity of the signed statement. It does not establish correspondence between that statement and physical reality.



Future independent witnesses, heterogeneous nodes and attestation mechanisms are intended to increase the cost of producing mutually consistent false evidence.



They cannot make physical truth cryptographically absolute.



23\. Regulatory and assurance consequence



This architecture is intended to improve demonstrability and traceability.



It does not constitute:



IEC 61508 certification;

IEC 62443 certification;

SIL qualification;

EU conformity assessment;

AI Act conformity;

Cyber Resilience Act conformity;

Machinery Regulation conformity.



The architecture shall instead preserve evidence that may later support applicable assessment activities.



In particular, Guardian seeks to make the following boundaries technically inspectable:



data validation

producer authentication

freshness

decision provenance

advisory output

deterministic authority

physical actuation



Regulatory readiness is an evidence property, not a certification claim.



24\. Security invariants



M15 shall preserve the following invariants:



I-01  Validation occurs before assurance authentication semantics are accepted.



I-02  producer\_id alone is not proof of identity.



I-03  Factory UID is not secret key material.



I-04  device\_id is not silently promoted to cryptographic identity.



I-05  Private key material is never carried in an assurance message.



I-06  Messages cannot introduce their own trusted verification key.



I-07  Signature algorithm selection is explicit.



I-08  Signature validity does not imply freshness.



I-09  Signature validity does not imply truthfulness.



I-10  Signature validity does not imply authorization.



I-11  Advisory output cannot acquire actuator authority through authentication.



I-12  Test identity cannot silently become production identity.



I-13  Production non-extractable identity is not claimed until demonstrated.



I-14  Cryptographic-provider failure is not reclassified as malformed input.



I-15  Changes to the signed transcript require explicit versioning.

25\. Threats considered



M15 design and implementation must explicitly test or analyze:



forged producer\_id;

unknown producer;

unknown key\_id;

substituted key\_id;

key rotation;

revoked credential;

malformed signature;

truncated signature;

oversized signature;

algorithm substitution;

algorithm downgrade;

cross-protocol signature reuse;

cross-object signature reuse;

canonicalization differential;

payload modification after signing;

envelope modification after signing;

producer-epoch modification;

logical-time modification;

object-ID modification;

public-key injection;

test-key use in production configuration;

crypto-provider failure;

compromised legitimate signing node.



Replay across reset is acknowledged but belongs to the next milestone.



26\. Out of scope



This ADR does not implement or claim:



persistent anti-replay;

monotonic persistent counters;

trusted wall-clock time;

quorum;

distributed consensus;

partition handling;

split-brain prevention;

failover;

hardware-backed production key storage;

physical hardware qualification;

WCET;

SIL;

conformity assessment;

direct AI authority.

27\. Implementation sequence



After this ADR is approved, implementation shall proceed incrementally:



1\. Freeze signed transcript specification

2\. Freeze identity/key metadata schema

3\. Implement test signing-provider interface

4\. Implement verification-provider interface

5\. Add Ed25519 host/test integration

6\. Add positive signature vectors

7\. Add negative/adversarial vectors

8\. Add canonicalization/signature reproducibility tests

9\. Add identity/key lifecycle tests

10\. Add CI gate

11\. Produce evidence manifest

12\. Reassess production signing backend separately



No production hardware identity claim shall result from completion of host/software tests alone.



28\. Acceptance criteria



M15-02 may be considered software-complete only when evidence demonstrates that:



identical accepted objects produce identical signing transcripts;

mutation of every authenticated field invalidates verification;

unknown fields remain rejected;

unknown producer identities fail closed;

unknown/revoked keys fail closed;

unsupported algorithms fail closed;

test credentials are explicitly distinguishable;

crypto-provider failure is distinguishable from malformed input;

valid signatures do not bypass policy;

signature verification cannot create actuator authority;

CI reproduces all positive and negative vectors;

evidence artifacts are hash-verifiable.



Hardware-backed production identity remains a separate gate.



29\. Consequences

Positive

Establishes explicit producer provenance.

Prepares Guardian for independent witness nodes.

Enables future heterogeneous attestation.

Avoids shared-secret identity coupling between federated nodes.

Preserves M14 deterministic validation.

Maintains separation between authentication and authority.

Reuses Guardian's existing architectural direction toward Ed25519.

Negative

Introduces key lifecycle complexity.

Requires a trusted public-key registry.

Requires eventual production key custody.

Adds CPU, flash and RAM cost.

Introduces revocation and provisioning requirements.

Does not solve malicious-but-legitimately-signed observations.

Does not solve replay without M15-03.

30\. Deferred decisions



The following require separate ADRs or adjudication:



M15-03 — Persistent Freshness and Anti-Replay



producer\_epoch

monotonic state

reset semantics

rollback resistance

nonce lifecycle

persistent storage



M15-04 — Attestation and Witness Exchange



attestation transcript

witness relationships

independent observations

trust evaluation



Later distributed architecture



quorum

partition handling

split-brain

failover

heterogeneous nodes

31\. Decision summary



Guardian will move from structural producer claims toward cryptographically authenticated producer statements while preserving the separation between validation, authenticity, freshness, truth and authority.



The intended chain is:



VALIDATE

&#x20;  ↓

CANONICALIZE

&#x20;  ↓

AUTHENTICATE

&#x20;  ↓

ESTABLISH FRESHNESS

&#x20;  ↓

CORROBORATE

&#x20;  ↓

APPLY DETERMINISTIC POLICY

&#x20;  ↓

AUTHORIZE



The defining invariant remains:



Intelligence may advise. Deterministic policy authorizes. Evidence proves.



And for M15 specifically:



A valid Guardian signature proves which trusted credential authenticated the accepted canonical statement. It does not prove that the statement is fresh, physically true, or authorized to cause an action.



Status: DRAFT — architecture review required before implementation.

---

## M15-03 Post-Adjudication Reconciliation

Status: APPROVED FOR BOUNDED SOFTWARE IMPLEMENTATION

The persistent freshness and anti-replay properties previously deferred to M15-03 have completed architecture adjudication under:

TD-M15-005 — Persistent Freshness State Model

TD-M15-005 SHA-256:
BE52461F33E92EE15532D290AC2DE3EFC470042407E3847F93271B693618E860

Historical statements in this ADR that persistent freshness, replay resistance across reset, nonce lifecycle, replay windows, reset semantics, rollback resistance, or persistent storage were deferred are retained for traceability.

The authoritative post-adjudication M15-03 contract is now:

AUTHENTICATED
→ freshness evaluation
→ FRESH_CANDIDATE
→ durable state commit
→ verified commit
→ FRESH

Same-epoch semantics:

logical_time greater than durable high-water mark → FRESH_CANDIDATE
logical_time equal to durable high-water mark → REPLAY
logical_time below durable high-water mark → LOGICAL_TIME_ROLLBACK

Epoch semantics:

- reset alone does not establish a new epoch;
- arbitrary unseen epochs are not automatically accepted;
- exact epoch transition authorization is required;
- immediate superseded-epoch reversal produces EPOCH_ROLLBACK;
- missing or corrupted previously established state is not FIRST_SEEN.

Persistence semantics:

- FRESH is returned only after successful durable state commit and verification;
- state invalidity, unavailability, rollback, or commit failure fails freshness closed;
- production hardware-backed rollback resistance is not claimed unless separately demonstrated.

Replay semantics:

M15-03 v1 replay acceptance window = 0

Out-of-order freshness acceptance = NOT SUPPORTED

Nonce semantics:

M15-03 v1 nonce requirement = NOT REQUIRED

Nonce/challenge semantics in other Guardian protocols are not redefined by M15-03.

Portable resource semantics:

- maximum 4,096 producer freshness states;
- each backend declares an explicit capacity less than or equal to 4,096;
- current_epoch plus previous_epoch are retained per producer;
- transition_sequence and persistent generation are unsigned 64-bit monotonic values;
- automatic eviction is prohibited.

Security boundaries remain:

AUTHENTICATED ≠ FRESH
FRESH ≠ AUTHORIZED
FRESH ≠ PHYSICALLY TRUE
FRESH ≠ ACTUATION AUTHORITY

This reconciliation authorizes bounded M15-03 software implementation.

It does not allocate an STM32F401 flash partition and does not claim production STM32 persistence capacity.

M15-04 — Attestation and Witness Exchange remains separately deferred.

---

## M15-03 Post-Implementation Evidence Reconciliation

Evidence baseline: `d37714d`

The bounded M15-03 host software path authorized by TD-M15-005 is now implemented and validated.

Validated implementation sequence:

```text
signed assurance
-> trust resolution
-> cryptographic signature verification
-> AUTHENTICATED
-> authenticated producer_id / producer_epoch / logical_time
-> persistent-state load
-> freshness evaluation
-> FRESH_CANDIDATE
-> persistence prepare
-> commit
-> verify
-> FRESH
```

Complete assurance regression:

```text
307 tests passed
```

The following separations remain normative:

```text
AUTHENTICATED != FRESH
FRESH_CANDIDATE != FRESH
COMMITTED != VERIFIED
FRESH != AUTHORIZED
```

Current implementation boundaries:

- FIRST_SEEN does not bootstrap persistent state;
- explicit bootstrap authorization remains outside normal freshness orchestration;
- explicit epoch-transition authorization remains outside normal freshness orchestration;
- arbitrary-storage rollback resistance is not demonstrated;
- no hardware-backed monotonic anchor is claimed;
- production STM32F401 persistence is not implemented by this host slice;
- STM32F401 flash allocation, endurance, and physical power-loss behavior are not validated;
- freshness does not establish policy authority or actuation permission;
- M15-04 remains separately deferred.

Historical statements in this ADR that described persistent freshness as future or deferred remain preserved as chronological architecture evidence. For the bounded host software path, they are superseded by this reconciliation and TD-M15-005.

---

## M15-03 Authorization Architecture Reconciliation - TD-M15-006

Evidence authority: TD-M15-006 - Bootstrap and Epoch-Transition Authorization Model

TD-M15-006 status: RESOLVED

Architecture disposition: APPROVED FOR BOUNDED SOFTWARE IMPLEMENTATION

The authorization architecture required by the earlier M15-03 freshness decisions has now completed architecture adjudication.

The following authorization properties are frozen:

- bootstrap authorization and epoch-transition authorization are separate privileges;
- ordinary assurance authentication does not grant bootstrap authority;
- ordinary assurance authentication does not grant epoch-transition authority;
- authorization uses dedicated signed authorization objects;
- authorization uses a dedicated authorization trust domain;
- BOOTSTRAP_AUTHORITY and EPOCH_TRANSITION_AUTHORITY are explicit capabilities;
- authorization replay protection is independent of producer logical_time;
- authorization replay acceptance window is zero;
- authorization consumption and protected freshness-state mutation form one verified logical transaction;
- BootstrapAuthorizationV1 and EpochTransitionAuthorizationV1 use distinct schemas and purpose domains;
- authorization signatures bind exact canonical authorization bytes;
- authorization trust does not fall back to ordinary producer assurance trust.

The following semantic separations remain normative:

ASSURANCE_AUTHENTICATED != BOOTSTRAP_AUTHORIZED

ASSURANCE_AUTHENTICATED != EPOCH_TRANSITION_AUTHORIZED

FIRST_SEEN != BOOTSTRAP_AUTHORIZED

EPOCH_TRANSITION_REQUIRED != EPOCH_TRANSITION_AUTHORIZED

AUTHORIZATION_AUTHENTICATED != AUTHORIZATION_CANDIDATE

AUTHORIZATION_CANDIDATE != AUTHORIZATION_CONSUMED

EPOCH_TRANSITION_AUTHORIZED != FRESH

FRESH != AUTHORIZED

PREPARED != COMMITTED

COMMITTED != VERIFIED

Current implementation disposition:

TD-M15-006 architecture condition: CLOSED

Bounded host authorization software implementation: AUTHORIZED

Bounded host authorization validation evidence: PENDING

Production STM32 authorization persistence: NOT AUTHORIZED

Hardware-backed rollback resistance: NOT DEMONSTRATED

M15-04: PENDING

The authorization architecture permits bounded host implementation only.

It does not establish that the authorization software is already implemented or validated.

It does not allocate STM32F401 flash, establish production persistence capacity, demonstrate hardware-backed rollback resistance, or authorize M15-04.

Historical ADR statements that bootstrap or epoch-transition authorization remained deferred are retained as chronological evidence.

For architecture status they are superseded by TD-M15-006.

For implementation and validation status, the bounded authorization software remains pending evidence.
