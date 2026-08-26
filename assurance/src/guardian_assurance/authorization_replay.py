"""Pure bounded anti-replay evaluation for authenticated M15 authorization.

This module does not persist, consume, mutate freshness state, bootstrap a
producer, perform an epoch transition, grant policy authority, or permit
actuation.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

from .authorization_authentication import (
    AuthorizationAuthenticationResult,
    AuthorizationAuthenticationResultCode,
)
from .authorization_objects import BOOTSTRAP_TYPE, EPOCH_TRANSITION_TYPE
from .authorization_trust_store import (
    BOOTSTRAP_AUTHORITY,
    EPOCH_TRANSITION_AUTHORITY,
)


UINT64_MAX = (1 << 64) - 1
AUTHORIZATION_REPLAY_MAX_SCOPES = 4_096


class AuthorizationReplayCode(str, Enum):
    AUTHORIZATION_CANDIDATE = "AUTHORIZATION_CANDIDATE"
    AUTHORIZATION_NOT_AUTHENTICATED = "AUTHORIZATION_NOT_AUTHENTICATED"
    AUTHORIZATION_REPLAY = "AUTHORIZATION_REPLAY"
    AUTHORIZATION_SEQUENCE_EXHAUSTED = "AUTHORIZATION_SEQUENCE_EXHAUSTED"
    AUTHORIZATION_CAPACITY_EXCEEDED = "AUTHORIZATION_CAPACITY_EXCEEDED"
    AUTHORIZATION_SCOPE_INVALID = "AUTHORIZATION_SCOPE_INVALID"
    AUTHORIZATION_STATE_INVALID = "AUTHORIZATION_STATE_INVALID"


@dataclass(frozen=True)
class AuthorizationReplayScope:
    authority_id: str
    purpose_domain: str
    producer_id: str


@dataclass(frozen=True)
class AuthorizationReplayState:
    """Authoritative high-water authorization sequence for one replay scope."""

    scope: AuthorizationReplayScope
    authorization_sequence: int

    def __post_init__(self) -> None:
        if isinstance(self.authorization_sequence, bool):
            raise ValueError("authorization_sequence must be an integer")

        if not isinstance(self.authorization_sequence, int):
            raise ValueError("authorization_sequence must be an integer")

        if not 0 <= self.authorization_sequence <= UINT64_MAX:
            raise ValueError("authorization_sequence outside uint64")


@dataclass(frozen=True)
class AuthorizationReplayOutcome:
    code: AuthorizationReplayCode
    scope: AuthorizationReplayScope | None = None
    previous_state: AuthorizationReplayState | None = None
    candidate_state: AuthorizationReplayState | None = None
    authorization_id: str | None = None
    authorization_sequence: int | None = None
    detail: str | None = None

    @property
    def candidate(self) -> bool:
        return self.code is AuthorizationReplayCode.AUTHORIZATION_CANDIDATE


def _purpose_for_authorization_type(authorization_type: str) -> str:
    if authorization_type == BOOTSTRAP_TYPE:
        return BOOTSTRAP_AUTHORITY

    if authorization_type == EPOCH_TRANSITION_TYPE:
        return EPOCH_TRANSITION_AUTHORITY

    raise ValueError("unsupported authorization_type")


def derive_authorization_replay_scope(
    authenticated: AuthorizationAuthenticationResult,
) -> AuthorizationReplayScope:
    """Derive the frozen Decision-D replay scope.

    scope = authority_id + authorization purpose domain + producer_id
    """

    if (
        authenticated.authority_id is None
        or authenticated.authorization_type is None
        or authenticated.producer_id is None
    ):
        raise ValueError("authenticated authorization lacks replay scope fields")

    purpose = _purpose_for_authorization_type(
        authenticated.authorization_type
    )

    return AuthorizationReplayScope(
        authority_id=authenticated.authority_id,
        purpose_domain=purpose,
        producer_id=authenticated.producer_id,
    )


def evaluate_authorization_replay(
    authenticated: AuthorizationAuthenticationResult,
    *,
    previous_state: AuthorizationReplayState | None,
    established_scope_count: int,
) -> AuthorizationReplayOutcome:
    """Pure replay evaluation with replay window zero."""

    if (
        authenticated.code
        is not AuthorizationAuthenticationResultCode.AUTHORIZATION_AUTHENTICATED
    ):
        return AuthorizationReplayOutcome(
            code=AuthorizationReplayCode.AUTHORIZATION_NOT_AUTHENTICATED,
            detail="authorization authentication is required",
        )

    sequence = authenticated.authorization_sequence
    authorization_id = authenticated.authorization_id

    if (
        isinstance(sequence, bool)
        or not isinstance(sequence, int)
        or not 0 <= sequence <= UINT64_MAX
    ):
        return AuthorizationReplayOutcome(
            code=AuthorizationReplayCode.AUTHORIZATION_STATE_INVALID,
            authorization_id=authorization_id,
            detail="authenticated authorization_sequence is invalid",
        )

    try:
        scope = derive_authorization_replay_scope(authenticated)
    except ValueError as exc:
        return AuthorizationReplayOutcome(
            code=AuthorizationReplayCode.AUTHORIZATION_SCOPE_INVALID,
            authorization_id=authorization_id,
            authorization_sequence=sequence,
            detail=str(exc),
        )

    if previous_state is None:
        if (
            isinstance(established_scope_count, bool)
            or not isinstance(established_scope_count, int)
            or established_scope_count < 0
        ):
            return AuthorizationReplayOutcome(
                code=AuthorizationReplayCode.AUTHORIZATION_STATE_INVALID,
                scope=scope,
                authorization_id=authorization_id,
                authorization_sequence=sequence,
                detail="established_scope_count is invalid",
            )

        if established_scope_count >= AUTHORIZATION_REPLAY_MAX_SCOPES:
            return AuthorizationReplayOutcome(
                code=AuthorizationReplayCode.AUTHORIZATION_CAPACITY_EXCEEDED,
                scope=scope,
                authorization_id=authorization_id,
                authorization_sequence=sequence,
                detail="authorization replay-state capacity exhausted",
            )

        candidate = AuthorizationReplayState(
            scope=scope,
            authorization_sequence=sequence,
        )

        return AuthorizationReplayOutcome(
            code=AuthorizationReplayCode.AUTHORIZATION_CANDIDATE,
            scope=scope,
            candidate_state=candidate,
            authorization_id=authorization_id,
            authorization_sequence=sequence,
        )

    if previous_state.scope != scope:
        return AuthorizationReplayOutcome(
            code=AuthorizationReplayCode.AUTHORIZATION_STATE_INVALID,
            scope=scope,
            previous_state=previous_state,
            authorization_id=authorization_id,
            authorization_sequence=sequence,
            detail="previous replay state scope mismatch",
        )

    if previous_state.authorization_sequence == UINT64_MAX:
        return AuthorizationReplayOutcome(
            code=AuthorizationReplayCode.AUTHORIZATION_SEQUENCE_EXHAUSTED,
            scope=scope,
            previous_state=previous_state,
            authorization_id=authorization_id,
            authorization_sequence=sequence,
            detail="authorization sequence high-water is exhausted",
        )

    if sequence <= previous_state.authorization_sequence:
        return AuthorizationReplayOutcome(
            code=AuthorizationReplayCode.AUTHORIZATION_REPLAY,
            scope=scope,
            previous_state=previous_state,
            authorization_id=authorization_id,
            authorization_sequence=sequence,
            detail="authorization_sequence did not strictly advance",
        )

    candidate = AuthorizationReplayState(
        scope=scope,
        authorization_sequence=sequence,
    )

    return AuthorizationReplayOutcome(
        code=AuthorizationReplayCode.AUTHORIZATION_CANDIDATE,
        scope=scope,
        previous_state=previous_state,
        candidate_state=candidate,
        authorization_id=authorization_id,
        authorization_sequence=sequence,
    )
