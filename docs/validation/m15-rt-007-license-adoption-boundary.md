# RT-M15-007 — License / Adoption Boundary

## Status

CLOSED_FOR_LICENSE_BOUNDARY

## Classification

License-governance, public-visibility and adoption-claim finding.

## Finding

Guardian F401 is publicly visible but is not released under an open-source
license.

Repository inspection found two open-source-style text matches and five
production/adoption-style text matches.

All inspected matches were adjudicated in context.

No false open-source, adoption or production-deployment claim was demonstrated.

## RT-07A — Proprietary license identity

Status:

`CONFIRMED`

The repository LICENSE establishes proprietary terms and reserves rights not
expressly granted.

The project copyright holder is identified as:

`Antonio José Socorro Marín`

The license includes warranty and liability disclaimers and explicitly reserves
all rights not expressly granted.

## RT-07B — Public visibility versus open source

Status:

`CONFIRMED`

The README states:

`public visibility does not create an open-source license.`

The README also states:

`The original Guardian F401 code is not released under an open-source license.`

Therefore public GitHub visibility must not be interpreted as an open-source
grant.

## RT-07C — Reuse / redistribution / derivative-use boundary

Status:

`CONFIRMED`

The README states that public repository visibility does not grant additional
permission for:

- commercial use;
- redistribution;
- modification;
- sublicensing;
- incorporation of original Guardian F401 code into another product or service;

except for rights provided by applicable law and GitHub Terms of Service for
public repositories.

The LICENSE further states that research collaboration, evaluation,
reproduction, derivative-development and commercial licenses may be available
under a separate written agreement.

## RT-07D — Third-party licensing

Status:

`CONFIRMED_FOR_CURRENT_DOCUMENTATION`

The LICENSE states that third-party software, libraries, headers, tools,
trademarks and other materials remain subject to their respective licenses,
notices and terms.

The README and THIRD_PARTY_NOTICES.md preserve the same distinction.

Nothing in the Guardian F401 license expands rights in third-party materials.

## RT-07E — Open-source claim inspection

Observed open-source-style matches:

`2`

Both were negative license-boundary statements.

Examples include:

- `public visibility does not create an open-source license`;
- `The original Guardian F401 code is not released under an open-source license`.

True unsupported open-source claims:

`0`

## RT-07F — Adoption / production claim inspection

Observed production/adoption-style matches:

`5`

The reviewed candidates were restrictions, negative statements or unrelated
false positives.

Examples include:

- `No production deployment is authorized`;
- production deployment appearing in residual-restriction lists;
- release/version discrepancy wording unrelated to production adoption;
- AXF adjudication text unrelated to third-party adoption.

True unsupported adoption or production-deployment claims:

`0`

## Governance interpretation

The proprietary license may affect external adoption, reuse, collaboration or
commercialization.

That is a project-governance and licensing decision.

It is not, by itself, a technical defect.

The relevant repository requirement is clarity and consistency regarding the
rights actually granted.

## Normative boundary

Guardian F401 documentation shall preserve:

`PUBLIC REPOSITORY != OPEN SOURCE`

`PUBLICATION != PERMISSION TO REDISTRIBUTE`

`PUBLIC VISIBILITY != COMMERCIAL LICENSE`

`COMMERCIAL AVAILABILITY != GENERAL LICENSE GRANT`

`THIRD-PARTY MATERIAL != GUARDIAN-LICENSED MATERIAL`

## Decision

RT-M15-007 is:

`CLOSED_FOR_LICENSE_BOUNDARY`

The repository clearly distinguishes proprietary project rights from public
visibility.

No false open-source claim was demonstrated.

No unsupported production-adoption or third-party-adoption claim was
demonstrated.

No immediate README or LICENSE correction is required.

## Residual governance considerations

Future releases should continue to verify:

- consistency between LICENSE, README, NOTICE and THIRD_PARTY_NOTICES;
- third-party license obligations;
- redistribution status of generated/vendor material;
- any newly introduced external dependency;
- wording associated with commercial licensing;
- compatibility between publication channels and project-license terms.

## Residual restrictions

This adjudication does not establish:

- legal advice;
- patent rights;
- trademark rights;
- third-party license compatibility in every jurisdiction;
- commercial-license availability for a specific party;
- open-source status;
- external adoption;
- production deployment.
