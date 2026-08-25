"""Persistence contract for composite M15 authorization transactions.

This module defines persistence states only.

PREPARED, COMMITTED, VERIFIED, and AUTHORIZATION_CONSUMED remain
distinct facts. Durable storage implementation is provided separately.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from typing import Protocol

from .authorization_transaction import (
    AuthorizationPreparedTransaction,
    AuthorizationTransactionState,
)


class AuthorizationLoadCode(str, Enum):
    STATE_VALID = "AUTHORIZATION_STATE_VALID"
    STATE_NOT_ESTABLISHED = "AUTHORIZATION_STATE_NOT_ESTABLISHED"
    AUTHORIZATION_STATE_INVALID = "AUTHORIZATION_STATE_INVALID"
    AUTHORIZATION_STATE_UNAVAILABLE = "AUTHORIZATION_STATE_UNAVAILABLE"
    AUTHORIZATION_STATE_ROLLBACK = "AUTHORIZATION_STATE_ROLLBACK"


class AuthorizationPersistCode(str, Enum):
    PREPARED = "PREPARED"
    COMMITTED = "COMMITTED"
    VERIFIED = "VERIFIED"
    AUTHORIZATION_STATE_PERSIST_FAILURE = "AUTHORIZATION_STATE_PERSIST_FAILURE"
    AUTHORIZATION_STATE_STALE_PREPARATION = "AUTHORIZATION_STATE_STALE_PREPARATION"


@dataclass(frozen=True)
class AuthorizationLoadResult:
    code: AuthorizationLoadCode
    state: AuthorizationTransactionState | None = None
    detail: str | None = None

    def __post_init__(self) -> None:
        if self.code is AuthorizationLoadCode.STATE_VALID:
            if self.state is None:
                raise ValueError("STATE_VALID requires transaction state")
        elif self.state is not None:
            raise ValueError("non-STATE_VALID load result must not carry state")

    @property
    def valid(self) -> bool:
        return self.code is AuthorizationLoadCode.STATE_VALID


@dataclass(frozen=True)
class AuthorizationPersistResult:
    code: AuthorizationPersistCode
    state: AuthorizationTransactionState | None = None
    detail: str | None = None

    @property
    def prepared(self) -> bool:
        return self.code is AuthorizationPersistCode.PREPARED

    @property
    def committed(self) -> bool:
        return self.code is AuthorizationPersistCode.COMMITTED

    @property
    def verified(self) -> bool:
        return self.code is AuthorizationPersistCode.VERIFIED


class AuthorizationTransactionBackend(Protocol):
    """Persistence interface for one composite authorization generation."""

    def load(self, replay_scope_key: str) -> AuthorizationLoadResult:
        """Load the authoritative verified composite generation."""
        ...

    def prepare(
        self,
        prepared: AuthorizationPreparedTransaction,
    ) -> AuthorizationPersistResult:
        """Accept one prepared candidate without making it authoritative."""
        ...

    def commit(
        self,
        prepared: AuthorizationPreparedTransaction,
    ) -> AuthorizationPersistResult:
        """Attempt durable commit of the exact prepared candidate."""
        ...

    def verify(
        self,
        prepared: AuthorizationPreparedTransaction,
    ) -> AuthorizationPersistResult:
        """Verify durable authoritative state equals the candidate."""
        ...
