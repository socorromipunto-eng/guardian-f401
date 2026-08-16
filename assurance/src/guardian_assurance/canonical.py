"""RFC 8785 canonicalization after Guardian validation."""

from .errors import AssuranceError, ErrorCode
from .limits import AssuranceLimits
from .validation import parse_and_validate_envelope


def canonicalize_envelope(raw: bytes, limits: AssuranceLimits | None = None) -> bytes:
    value = parse_and_validate_envelope(raw, limits)
    try:
        import rfc8785

        return rfc8785.dumps(value)
    except AssuranceError:
        raise
    except Exception as exc:
        raise AssuranceError(ErrorCode.CANONICALIZATION, "RFC 8785 encoding failed") from exc

