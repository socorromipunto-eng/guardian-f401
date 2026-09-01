# M15 Decision & Evidence Matrix

Status: PUBLISHED / BOUNDED  
Milestone: M15  
Role: HUMAN-READABLE VIEW OF `governance/decision-register.json`

This matrix does not grant authority and does not replace ADRs, implementation artifacts, evidence, Git history, or human approval records.

| Decision | Status | Implementation | Evidence boundary | Downstream |
|---|---|---|---|---|
| DEC-M15-001 — advisory != authority != actuation | APPROVED | IMPLEMENTATION_SURFACE_PRESENT | software/governance controls only | M16, M17, M18 |
| DEC-M15-002 — repository governance architecture | APPROVED | IMPLEMENTATION_SURFACE_PRESENT | governance surface exists; completeness not demonstrated | later milestone governance |
| DEC-M15-003 — serialization/versioning/namespaces | APPROVED | IMPLEMENTATION_SURFACE_PRESENT | versioned schemas/strict JSON exist; full conformance not demonstrated | registers/evidence |
| DEC-M15-004 — schema dialect/validation/offline policy | APPROVED | IMPLEMENTATION_SURFACE_PRESENT | validation surface exists; semantic truth/authority not implied | semantic CI |

## Non-claims

M15 does not claim physical STM32F401 qualification, production readiness, external cybersecurity certification, functional-safety certification, heterogeneous deployment, AI actuation authority, complete governance population, or complete implementation of every M15 ADR.

## External-practice mapping

NASA, NIST and CIS mappings remain `NOT_ADJUDICATED`. The project may later document alignment with selected practices only after explicit mapping and evidence review. No certification or full-compliance claim is made.

## Authority boundary

A technical PASS, schema-valid artifact, semantically coherent registry, CI success, commit, pull request, merge, tag, or release does not create human authority that was not explicitly granted.
