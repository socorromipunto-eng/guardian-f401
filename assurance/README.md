# Guardian F401 M14-S2 isolated implementation candidate

This candidate extends the locally closed M14-S1 slice with closed observation,
decision and witness payload schemas, normative JSON-value depth/node semantics
and a safe-integer-only numeric contract.

It is an external review candidate. It is not authorized for repository copy,
commit, push, merge or publication. It provides software-only research evidence
and no hardware, real-time, production, safety or certification claim.

The corrected suite contains 87 discovered test methods, including the retained
S1 intentions and the S2 coverage corrections. Run with the previously verified
`rfc8785==0.1.4` wheel:

```text
PYTHONPATH=payload/assurance/src python -m unittest discover -s payload/assurance/tests -v
```
