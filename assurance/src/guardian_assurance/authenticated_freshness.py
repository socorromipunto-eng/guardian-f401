"""Guardian M15 authenticated-freshness integration.

This module composes signed-assurance authentication with M15-03
persistent freshness evaluation.

Authentication failure terminates processing before freshness.
AUTHENTICATED does not itself imply FRESH.
FRESH does not imply authorization, authority, or actuation permission.
"""

from __future__ import annotations

from dataclasses import dataclass

from .authentication import (
    AuthenticationResult,
    authenticate_signed_assurance,
)
from .crypto_provider import HostEd25519Provider
from .freshness_orchestrator import (
    FreshnessOutcome,
    establish_freshness,
)
from .freshness_persistence import FreshnessPersistenceBackend
from .trust_store import TrustStore


@dataclass(frozen=True)
class AuthenticatedFreshnessResult:
    """Combined authentication and freshness result.

    The authentication result is always present.
    The freshness result exists only after successful authentication.
    """

    authentication: AuthenticationResult
    freshness: FreshnessOutcome | None = None

    @property
    def authenticated(self) -> bool:
        return self.authentication.authenticated

    @property
    def fresh(self) -> bool:
        return self.freshness is not None and self.freshness.fresh


def authenticate_and_establish_freshness(
    raw_wrapper: bytes,
    *,
    trust_store: TrustStore,
    provider: HostEd25519Provider,
    freshness_backend: FreshnessPersistenceBackend,
) -> AuthenticatedFreshnessResult:
    """Authenticate one signed assurance statement, then evaluate freshness.

    The exact producer_id, producer_epoch, and logical_time handed to
    freshness come from the successful AuthenticationResult.
    """

    authentication = authenticate_signed_assurance(
        raw_wrapper,
        trust_store=trust_store,
        provider=provider,
    )

    if not authentication.authenticated:
        return AuthenticatedFreshnessResult(
            authentication=authentication,
            freshness=None,
        )

    if (
        authentication.producer_id is None
        or authentication.producer_epoch is None
        or authentication.logical_time is None
    ):
        raise RuntimeError(
            "authenticated result is missing authenticated freshness claims"
        )

    freshness = establish_freshness(
        freshness_backend,
        producer_id=authentication.producer_id,
        producer_epoch=authentication.producer_epoch,
        logical_time=authentication.logical_time,
    )

    return AuthenticatedFreshnessResult(
        authentication=authentication,
        freshness=freshness,
    )
