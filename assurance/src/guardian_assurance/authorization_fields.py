"""Bounded field contracts for M15 authorization objects.

These validators define authorization-domain syntax only.

Syntactic compatibility with existing Guardian identifiers does not
collapse their security semantics.
"""

from __future__ import annotations

import re
from typing import Any


AUTHORITY_ID_MAX_CHARS = 128
AUTHORIZATION_ID_HEX_CHARS = 32
UINT64_MAX = (1 << 64) - 1

_AUTHORITY_ID = re.compile(r"^[A-Za-z0-9._:-]{1,128}$")
_AUTHORIZATION_ID = re.compile(r"^[0-9a-f]{32}$")


class AuthorizationFieldError(ValueError):
    """Reject one invalid authorization-domain field."""


def validate_authority_id(authority_id: Any) -> str:
    """Validate one bounded authorization-principal identifier."""

    if not isinstance(authority_id, str):
        raise AuthorizationFieldError("authority_id must be a string")

    if _AUTHORITY_ID.fullmatch(authority_id) is None:
        raise AuthorizationFieldError("invalid authority_id")

    return authority_id


def validate_authorization_id(authorization_id: Any) -> str:
    """Validate one lowercase 128-bit hexadecimal authorization id."""

    if not isinstance(authorization_id, str):
        raise AuthorizationFieldError("authorization_id must be a string")

    if _AUTHORIZATION_ID.fullmatch(authorization_id) is None:
        raise AuthorizationFieldError("invalid authorization_id")

    return authorization_id


def parse_uint64_decimal_wire(value: Any, field_name: str) -> int:
    """Parse one canonical unsigned 64-bit decimal wire string.

    Signed authorization objects encode uint64 counters as canonical decimal
    strings so the full uint64 domain remains compatible with RFC 8785/JCS.
    """

    if not isinstance(value, str):
        raise AuthorizationFieldError(f"invalid {field_name}")

    if value == "0":
        return 0

    if not value:
        raise AuthorizationFieldError(f"invalid {field_name}")

    if value[0] == "0":
        raise AuthorizationFieldError(f"invalid {field_name}")

    if not value.isascii() or not value.isdigit():
        raise AuthorizationFieldError(f"invalid {field_name}")

    parsed = int(value, 10)

    if not 0 <= parsed <= UINT64_MAX:
        raise AuthorizationFieldError(f"invalid {field_name}")

    return parsed

def validate_authorization_sequence(authorization_sequence: Any) -> int:
    """Validate one unsigned 64-bit authorization replay sequence."""

    if isinstance(authorization_sequence, bool):
        raise AuthorizationFieldError("invalid authorization_sequence")

    if not isinstance(authorization_sequence, int):
        raise AuthorizationFieldError("invalid authorization_sequence")

    if not 0 <= authorization_sequence <= UINT64_MAX:
        raise AuthorizationFieldError("invalid authorization_sequence")

    return authorization_sequence
