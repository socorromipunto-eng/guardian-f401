# Guardian F401 M14 bounded assurance slice

This component implements the M14 bounded assurance boundary: strict UTF-8 JSON
parsing, duplicate-member rejection, closed observation/decision/witness payload
schemas, explicit resource bounds and RFC 8785 canonicalization.

It provides software-only evidence. It establishes no hardware, real-time,
production, safety or certification claim. Canonical or schema-valid data cannot
authorize a physical action.

## Dependency

Canonicalization requires the pinned `rfc8785==0.1.4` wheel recorded in
`requirements.lock`. The suite fails with an explicit environment error if it is
absent.

```text
python -m pip install -r assurance/requirements.lock
```

## Validation

The suite contains 87 discovered test methods and is gated on CPython 3.12.

```text
PYTHONPATH=assurance/src python -m unittest discover -s assurance/tests -v
```