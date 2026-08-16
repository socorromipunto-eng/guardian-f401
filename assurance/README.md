# Guardian F401 M14 assurance slice 1

This isolated software-only slice implements bounded parsing, duplicate-key
rejection, closed-envelope validation, domain separation and RFC 8785
canonicalization for assurance records.

It is experimental research software. It does not authorize physical actions,
provide hardware validation, or establish production or certification claims.

Run from the repository root with an isolated environment containing the
approved `rfc8785==0.1.4` wheel:

```text
PYTHONPATH=assurance/src python -m unittest discover -s assurance/tests -v
```

