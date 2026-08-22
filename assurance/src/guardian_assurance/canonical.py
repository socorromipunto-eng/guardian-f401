"""RFC 8785 canonicalization after Guardian validation."""

from .errors import AssuranceError, ErrorCode
from .limits import AssuranceLimits
from .validation import parse_and_validate_envelope


def canonicalize_envelope(raw: bytes, limits: AssuranceLimits | None = None) -> bytes:
    value = parse_and_validate_envelope(raw, limits)

    # Resolve the pinned canonicalizer before entering the encoding boundary.
    # A missing dependency is an environment fault, not an input rejection,
    # so it must not enter the AssuranceError taxonomy.
    try:
        import rfc8785
    except ModuleNotFoundError as exc:
        raise RuntimeError(
            "guardian_assurance requires the pinned rfc8785 dependency. "
            "Install it with: python -m pip install -r assurance/requirements.lock"
        ) from exc

    try:
        return rfc8785.dumps(value)
    except AssuranceError:
        raise
    except Exception as exc:
        raise AssuranceError(ErrorCode.CANONICALIZATION, "RFC 8785 encoding failed") from exc