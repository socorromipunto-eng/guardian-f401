"""Deterministic M15 signed-assurance transcript construction.

This module constructs the exact byte sequence used by a later signing or
verification provider.

It does not perform signing, verification, freshness evaluation, authority
evaluation, or physical actuation.
"""

from __future__ import annotations

import re

from .canonical import canonicalize_envelope
from .validation import parse_and_validate_envelope


TRANSCRIPT_VERSION = "guardian-f401:m15:signed-assurance:v1"
SIGNATURE_ALGORITHM = "ed25519"

COMMON_DOMAIN_PREFIX = b"GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1"

PURPOSE_DOMAINS = {
    "observation": (
        b"GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:OBSERVATION"
    ),
    "decision": (
        b"GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:DECISION"
    ),
    "witness": (
        b"GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:WITNESS"
    ),
}

_KEY_ID_RE = re.compile(r"^[A-Za-z0-9._:-]{1,128}$")


class TranscriptError(ValueError):
    """Raised when M15 transcript metadata cannot satisfy the contract."""


def _encode_u16(value: int) -> bytes:
    if not 0 <= value <= 0xFFFF:
        raise TranscriptError("value does not fit uint16")
    return value.to_bytes(2, byteorder="big", signed=False)


def _encode_u32(value: int) -> bytes:
    if not 0 <= value <= 0xFFFFFFFF:
        raise TranscriptError("value does not fit uint32")
    return value.to_bytes(4, byteorder="big", signed=False)


def validate_key_id(key_id: str) -> bytes:
    """Validate and encode the approved opaque M15 key identifier."""

    if not isinstance(key_id, str):
        raise TranscriptError("key_id must be a string")

    if _KEY_ID_RE.fullmatch(key_id) is None:
        raise TranscriptError("key_id does not satisfy the M15 grammar")

    try:
        encoded = key_id.encode("ascii")
    except UnicodeEncodeError as exc:
        raise TranscriptError("key_id must be ASCII") from exc

    if len(encoded) > 128:
        raise TranscriptError("key_id exceeds M15 bound")

    return encoded


def purpose_domain_for_object_type(object_type: str) -> bytes:
    """Return the frozen purpose-specific signing domain.

    Only object types already admitted by the M14 validation boundary may
    reach normal transcript construction.
    """

    try:
        return PURPOSE_DOMAINS[object_type]
    except KeyError as exc:
        raise TranscriptError(
            "object_type has no M15 purpose signing domain"
        ) from exc


def build_signed_assurance_transcript(
    raw_assurance_object: bytes,
    key_id: str,
) -> bytes:
    """Build the exact M15 signed-assurance transcript.

    Processing order:

        M14 parse / validate
        ->
        derive M15 purpose domain from validated object_type
        ->
        RFC 8785 canonicalize using the approved M14 path
        ->
        construct deterministic length-delimited transcript

    Exact layout:

        PURPOSE_DOMAIN
        uint16_be(key_id length)
        key_id ASCII bytes
        uint16_be(producer_id UTF-8 length)
        producer_id UTF-8 bytes
        uint32_be(canonical object length)
        RFC 8785 canonical assurance object bytes

    Purpose is therefore bound twice:

        M15 purpose-specific signing domain
        +
        canonical M14 domain/object_type

    This function establishes no freshness, physical truth, authorization,
    actuator authority, or production identity.
    """

    if not isinstance(raw_assurance_object, bytes):
        raise TranscriptError("raw_assurance_object must be bytes")

    key_id_bytes = validate_key_id(key_id)

    # M14 remains the authoritative structural and semantic boundary.
    value = parse_and_validate_envelope(raw_assurance_object)

    object_type = value.get("object_type")
    if not isinstance(object_type, str):
        raise TranscriptError("validated object has no string object_type")

    purpose_domain = purpose_domain_for_object_type(object_type)

    producer_id = value.get("producer_id")
    if not isinstance(producer_id, str):
        raise TranscriptError("validated object has no string producer_id")

    producer_id_bytes = producer_id.encode("utf-8")

    if len(producer_id_bytes) > 0xFFFF:
        raise TranscriptError("producer_id exceeds transcript uint16 bound")

    canonical = canonicalize_envelope(raw_assurance_object)

    if len(canonical) > 0xFFFFFFFF:
        raise TranscriptError(
            "canonical assurance object exceeds uint32 bound"
        )

    return b"".join(
        (
            purpose_domain,
            _encode_u16(len(key_id_bytes)),
            key_id_bytes,
            _encode_u16(len(producer_id_bytes)),
            producer_id_bytes,
            _encode_u32(len(canonical)),
            canonical,
        )
    )