"""M15 authorization cryptographic authentication tests."""

from __future__ import annotations

import base64
import json
import unittest

from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat

from guardian_assurance.authorization_authentication import (
    AuthorizationAuthenticationResultCode,
    authenticate_authorization,
)
from guardian_assurance.authorization_objects import (
    BOOTSTRAP_SCHEMA_VERSION,
    BOOTSTRAP_TYPE,
    HIGH_WATER_UNSET,
)
from guardian_assurance.authorization_transcript import (
    AUTHORIZATION_SIGNATURE_ALGORITHM,
    AUTHORIZATION_TRANSCRIPT_VERSION,
    build_authorization_transcript,
)
from guardian_assurance.authorization_trust_store import (
    AUTHORIZATION_TRUST_STORE_SCHEMA_VERSION,
    BOOTSTRAP_AUTHORITY,
    parse_and_validate_authorization_trust_store,
)
from guardian_assurance.crypto_provider import HostEd25519Provider
from guardian_assurance.signed_authorization import (
    parse_and_validate_signed_authorization,
    encode_authorization_signature_base64url,
)


def key_wire(raw: bytes) -> str:
    return base64.urlsafe_b64encode(raw).rstrip(b"=").decode("ascii")


def authorization() -> dict[str, object]:
    return {
        "schema_version": BOOTSTRAP_SCHEMA_VERSION,
        "authorization_type": BOOTSTRAP_TYPE,
        "authorization_id": "0123456789abcdef0123456789abcdef",
        "authority_id": "root.bootstrap-authority:01",
        "producer_id": "plant-a.guardian-01",
        "authorization_sequence": "1",
        "producer_epoch": "11111111111111111111111111111111",
        "initial_high_water_state": HIGH_WATER_UNSET,
        "initial_logical_time": None,
    }


def encoded(value: object) -> bytes:
    return json.dumps(value, separators=(",", ":"), ensure_ascii=True).encode("utf-8")


class M15AuthorizationAuthenticationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.private_key = Ed25519PrivateKey.generate()
        self.public_key = self.private_key.public_key().public_bytes(
            Encoding.Raw,
            PublicFormat.Raw,
        )
        self.provider = HostEd25519Provider()

    def trust_store(self, *, capabilities=None, lifecycle_state="ACTIVE"):
        value = {
            "schema_version": AUTHORIZATION_TRUST_STORE_SCHEMA_VERSION,
            "environment": "TEST",
            "records": [
                {
                    "authority_id": "root.bootstrap-authority:01",
                    "key_id": "bootstrap-key-001",
                    "algorithm": "ed25519",
                    "public_key": key_wire(self.public_key),
                    "lifecycle_state": lifecycle_state,
                    "capabilities": capabilities or [BOOTSTRAP_AUTHORITY],
                }
            ],
        }
        return parse_and_validate_authorization_trust_store(
            encoded(value),
            expected_environment="TEST",
        )

    def signed_envelope(self, *, auth=None, key_id="bootstrap-key-001"):
        value = auth or authorization()
        auth_raw = encoded(value)
        transcript = build_authorization_transcript(
            auth_raw,
            key_id=key_id,
            authority_id=value["authority_id"],
        )
        signature = self.private_key.sign(transcript)
        envelope = {
            "signature_version": AUTHORIZATION_TRANSCRIPT_VERSION,
            "signature_algorithm": AUTHORIZATION_SIGNATURE_ALGORITHM,
            "authority_id": value["authority_id"],
            "key_id": key_id,
            "authorization_object": value,
            "signature": encode_authorization_signature_base64url(signature),
        }
        return encoded(envelope)

    def test_valid_bootstrap_authorization_authenticates(self) -> None:
        result = authenticate_authorization(
            self.signed_envelope(),
            trust_store=self.trust_store(),
            provider=self.provider,
        )
        self.assertEqual(
            result.code,
            AuthorizationAuthenticationResultCode.AUTHORIZATION_AUTHENTICATED,
        )
        self.assertTrue(result.authenticated)
        self.assertEqual(result.authorization_sequence, 1)

    def test_authenticated_result_preserves_exact_signed_object(self) -> None:
        raw = self.signed_envelope()
        validated = parse_and_validate_signed_authorization(raw)

        result = authenticate_authorization(
            raw,
            trust_store=self.trust_store(),
            provider=self.provider,
        )

        self.assertTrue(result.authenticated)
        self.assertEqual(
            result.authorization_object,
            validated.authorization_object,
        )
        self.assertEqual(
            result.authorization_object_raw,
            validated.authorization_object_raw,
        )
        self.assertEqual(
            result.authorization_object.producer_epoch,
            validated.authorization_object.producer_epoch,
        )
        self.assertEqual(
            result.authorization_object.initial_high_water_state,
            validated.authorization_object.initial_high_water_state,
        )
        self.assertEqual(
            result.authorization_object.initial_logical_time,
            validated.authorization_object.initial_logical_time,
        )

    def test_changed_signed_content_is_signature_invalid(self) -> None:
        raw = json.loads(self.signed_envelope().decode("utf-8"))
        raw["authorization_object"]["authorization_sequence"] = "2"
        result = authenticate_authorization(
            encoded(raw),
            trust_store=self.trust_store(),
            provider=self.provider,
        )
        self.assertEqual(
            result.code,
            AuthorizationAuthenticationResultCode.SIGNATURE_INVALID,
        )
        self.assertFalse(result.authenticated)

    def test_missing_capability_fails_before_authentication(self) -> None:
        result = authenticate_authorization(
            self.signed_envelope(),
            trust_store=self.trust_store(capabilities=["EPOCH_TRANSITION_AUTHORITY"]),
            provider=self.provider,
        )
        self.assertEqual(
            result.code,
            AuthorizationAuthenticationResultCode.CAPABILITY_MISMATCH,
        )
        self.assertFalse(result.authenticated)

    def test_revoked_key_fails(self) -> None:
        result = authenticate_authorization(
            self.signed_envelope(),
            trust_store=self.trust_store(lifecycle_state="REVOKED"),
            provider=self.provider,
        )
        self.assertEqual(
            result.code,
            AuthorizationAuthenticationResultCode.KEY_REVOKED,
        )

    def test_authenticated_result_does_not_claim_consumed_or_authorized(self) -> None:
        result = authenticate_authorization(
            self.signed_envelope(),
            trust_store=self.trust_store(),
            provider=self.provider,
        )
        self.assertTrue(result.authenticated)
        self.assertFalse(hasattr(result, "consumed"))
        self.assertFalse(hasattr(result, "authorized"))
        self.assertFalse(hasattr(result, "fresh"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
