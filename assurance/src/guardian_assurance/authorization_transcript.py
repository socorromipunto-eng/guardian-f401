"""Deterministic M15 authorization transcript construction.

This module canonicalizes an already valid authorization object and
constructs purpose-separated bytes for later signature verification.

Building a transcript does not verify a signature, authenticate an
authorization, accept replay state, consume an authorization, establish
freshness, grant policy authority, or permit actuation.
"""

from __future__ import annotations

import json
from typing import Any

import rfc8785

from .authorization_objects import (
    BOOTSTRAP_TYPE,
    EPOCH_TRANSITION_TYPE,
    AuthorizationObjectError,
    BootstrapAuthorization,
    EpochTransitionAuthorization,
    parse_and_validate_authorization_object,
)
from .transcript import TranscriptError, validate_key_id


AUTHORIZATION_TRANSCRIPT_VERSION = (
    "guardian-f401:m15:authorization-transcript:v1"
)
AUTHORIZATION_SIGNATURE_ALGORITHM = "ed25519"

BOOTSTRAP_AUTHORIZATION_PURPOSE = (
    b"GUARDIAN-F401:M15:AUTHORIZATION:V1:BOOTSTRAP"
)
EPOCH_TRANSITION_AUTHORIZATION_PURPOSE = (
    b"GUARDIAN-F401:M15:AUTHORIZATION:V1:EPOCH-TRANSITION"
)

AUTHORIZATION_CANONICAL_OBJECT_MAX_BYTES = 8_192


class AuthorizationTranscriptError(ValueError):
    """Reject an invalid authorization transcript construction request."""


def _encode_u16(value: int) -> bytes:
    if not 0 <= value <= 0xFFFF:
        raise AuthorizationTranscriptError("value does not fit uint16")
    return value.to_bytes(2, byteorder="big", signed=False)


def _encode_u32(value: int) -> bytes:
    if not 0 <= value <= 0xFFFFFFFF:
        raise AuthorizationTranscriptError("value does not fit uint32")
    return value.to_bytes(4, byteorder="big", signed=False)


def _purpose_domain(
    authorization: BootstrapAuthorization | EpochTransitionAuthorization,
) -> bytes:
    if authorization.authorization_type == BOOTSTRAP_TYPE:
        return BOOTSTRAP_AUTHORIZATION_PURPOSE

    if authorization.authorization_type == EPOCH_TRANSITION_TYPE:
        return EPOCH_TRANSITION_AUTHORIZATION_PURPOSE

    raise AuthorizationTranscriptError("unsupported authorization purpose")


def _authorization_mapping(
    authorization: BootstrapAuthorization | EpochTransitionAuthorization,
) -> dict[str, Any]:
    if isinstance(authorization, BootstrapAuthorization):
        return {
            "schema_version": authorization.schema_version,
            "authorization_type": authorization.authorization_type,
            "authorization_id": authorization.authorization_id,
            "authority_id": authorization.authority_id,
            "producer_id": authorization.producer_id,
            "authorization_sequence": str(authorization.authorization_sequence),
            "producer_epoch": authorization.producer_epoch,
            "initial_high_water_state": authorization.initial_high_water_state,
            "initial_logical_time": authorization.initial_logical_time,
        }

    if isinstance(authorization, EpochTransitionAuthorization):
        return {
            "schema_version": authorization.schema_version,
            "authorization_type": authorization.authorization_type,
            "authorization_id": authorization.authorization_id,
            "authority_id": authorization.authority_id,
            "producer_id": authorization.producer_id,
            "authorization_sequence": str(authorization.authorization_sequence),
            "from_epoch": authorization.from_epoch,
            "to_epoch": authorization.to_epoch,
            "transition_sequence": str(authorization.transition_sequence),
        }

    raise AuthorizationTranscriptError("unsupported authorization object")


def canonicalize_authorization_object(raw: bytes) -> bytes:
    """Validate and RFC 8785 canonicalize one authorization object."""

    try:
        authorization = parse_and_validate_authorization_object(raw)
    except AuthorizationObjectError as exc:
        raise AuthorizationTranscriptError(
            "authorization object validation failed"
        ) from exc

    mapping = _authorization_mapping(authorization)

    try:
        canonical = rfc8785.dumps(mapping)
    except Exception as exc:
        raise AuthorizationTranscriptError(
            "RFC 8785 authorization canonicalization failed"
        ) from exc

    if len(canonical) > AUTHORIZATION_CANONICAL_OBJECT_MAX_BYTES:
        raise AuthorizationTranscriptError(
            "canonical authorization object exceeds 8192 bytes"
        )

    return canonical


def build_authorization_transcript(
    raw_authorization_object: bytes,
    *,
    key_id: str,
    authority_id: str,
    signature_algorithm: str = AUTHORIZATION_SIGNATURE_ALGORITHM,
    transcript_version: str = AUTHORIZATION_TRANSCRIPT_VERSION,
) -> bytes:
    """Build deterministic purpose-separated authorization transcript bytes."""

    if transcript_version != AUTHORIZATION_TRANSCRIPT_VERSION:
        raise AuthorizationTranscriptError(
            "unsupported authorization transcript version"
        )

    if signature_algorithm != AUTHORIZATION_SIGNATURE_ALGORITHM:
        raise AuthorizationTranscriptError(
            "unsupported authorization signature algorithm"
        )

    try:
        key_id_bytes = validate_key_id(key_id)
    except TranscriptError as exc:
        raise AuthorizationTranscriptError("invalid authority key_id") from exc

    try:
        authority_id_bytes = authority_id.encode("ascii")
    except (AttributeError, UnicodeEncodeError) as exc:
        raise AuthorizationTranscriptError("authority_id must be ASCII") from exc

    if not 1 <= len(authority_id_bytes) <= 128:
        raise AuthorizationTranscriptError("authority_id exceeds M15 bound")

    try:
        authorization = parse_and_validate_authorization_object(
            raw_authorization_object
        )
    except AuthorizationObjectError as exc:
        raise AuthorizationTranscriptError(
            "authorization object validation failed"
        ) from exc

    if authorization.authority_id != authority_id:
        raise AuthorizationTranscriptError(
            "authority_id does not match authorization object"
        )

    purpose_domain = _purpose_domain(authorization)
    canonical = canonicalize_authorization_object(raw_authorization_object)

    return b"".join(
        (
            purpose_domain,
            _encode_u16(len(key_id_bytes)),
            key_id_bytes,
            _encode_u16(len(authority_id_bytes)),
            authority_id_bytes,
            _encode_u32(len(canonical)),
            canonical,
        )
    )
