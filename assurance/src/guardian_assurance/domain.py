"""Domain-separation rules for M14 assurance objects."""

DOMAIN_PREFIX = "guardian-f401:m14:assurance:v1:"


def expected_domain(object_type: str) -> str:
    return DOMAIN_PREFIX + object_type
