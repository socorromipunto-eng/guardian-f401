"""Guardian M15 authentication orchestration tests."""

from __future__ import annotations

import base64
import json
import unittest

from guardian_assurance.authentication import (
    AuthenticationResultCode,
    authenticate_signed_assurance,
)
from guardian_assurance.crypto_provider import (
    ED25519_ALGORITHM,
    HostEd25519Provider,
    generate_test_keypair,
)
from guardian_assurance.signed_wrapper import (
    encode_signature_base64url,
)
from guardian_assurance.transcript import (
    build_signed_assurance_transcript,
)
from guardian_assurance.trust_store import (
    TrustStoreErrorCode,
    parse_and_validate_trust_store,
)


def assurance_object(
    producer_id: str = "guardian-primary",
) -> dict[str, object]:
    return {
        "domain": "guardian-f401:m14:assurance:v1:observation",
        "object_type": "observation",
        "producer_id": producer_id,
        "producer_epoch": "0" * 32,
        "object_id": "f" * 32,
        "logical_time": 1,
        "payload": {
            "schema_version": "m14.observation.v1",
            "subject_id": "machine-01",
            "claim": "state",
            "result": "UNKNOWN",
            "evidence_digest": "a" * 64,
        },
    }


def encoded(value: object) -> bytes:
    return json.dumps(value, separators=(",", ":")).encode("utf-8")


def public_key_wire(raw: bytes) -> str:
    return base64.urlsafe_b64encode(raw).rstrip(b"=").decode("ascii")


def trust_store_raw(
    *,
    producer_id: str = "guardian-primary",
    key_id: str = "node-a-key-001",
    public_key: bytes,
    lifecycle_state: str = "ACTIVE",
) -> bytes:
    return encoded({
        "schema_version": "guardian-f401:m15:trust-store:v1",
        "environment": "TEST",
        "records": [
            {
                "producer_id": producer_id,
                "key_id": key_id,
                "algorithm": "ed25519",
                "public_key": public_key_wire(public_key),
                "lifecycle_state": lifecycle_state,
            }
        ],
    })


def signed_wrapper(
    *,
    private_key: bytes,
    assurance: dict[str, object] | None = None,
    key_id: str = "node-a-key-001",
    algorithm: str = "ed25519",
) -> bytes:
    value = assurance if assurance is not None else assurance_object()
    assurance_raw = encoded(value)

    transcript = build_signed_assurance_transcript(
        assurance_raw,
        key_id,
    )

    provider = HostEd25519Provider()
    signing = provider.sign_for_test(
        algorithm=ED25519_ALGORITHM,
        private_key=private_key,
        transcript=transcript,
    )

    if signing.signature is None:
        raise AssertionError("test signing failed")

    return encoded({
        "signature_version": "guardian-f401:m15:signed-assurance:v1",
        "signature_algorithm": algorithm,
        "key_id": key_id,
        "assurance_object": value,
        "signature": encode_signature_base64url(signing.signature),
    })


class M15AuthenticationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.provider = HostEd25519Provider()
        self.private_key, self.public_key = generate_test_keypair()

    def trusted_store(
        self,
        *,
        lifecycle_state: str = "ACTIVE",
        producer_id: str = "guardian-primary",
        key_id: str = "node-a-key-001",
        public_key: bytes | None = None,
    ):
        return parse_and_validate_trust_store(
            trust_store_raw(
                producer_id=producer_id,
                key_id=key_id,
                public_key=(
                    public_key
                    if public_key is not None
                    else self.public_key
                ),
                lifecycle_state=lifecycle_state,
            ),
            expected_environment="TEST",
        )

    def test_valid_trusted_signature_authenticates(self) -> None:
        result = authenticate_signed_assurance(
            signed_wrapper(private_key=self.private_key),
            trust_store=self.trusted_store(),
            provider=self.provider,
        )

        self.assertEqual(
            result.code,
            AuthenticationResultCode.AUTHENTICATED,
        )
        self.assertTrue(result.authenticated)
        self.assertEqual(
            result.producer_epoch,
            "0" * 32,
        )
        self.assertEqual(result.logical_time, 1)
        self.assertEqual(result.producer_id, "guardian-primary")
        self.assertEqual(result.key_id, "node-a-key-001")
        self.assertEqual(result.algorithm, "ed25519")
        self.assertEqual(result.trust_code, TrustStoreErrorCode.RESOLVED)

    def test_unknown_producer_fails_closed(self) -> None:
        raw = signed_wrapper(
            private_key=self.private_key,
            assurance=assurance_object(producer_id="unknown-producer"),
        )

        result = authenticate_signed_assurance(
            raw,
            trust_store=self.trusted_store(),
            provider=self.provider,
        )

        self.assertEqual(
            result.code,
            AuthenticationResultCode.IDENTITY_UNKNOWN,
        )
        self.assertFalse(result.authenticated)
        self.assertIsNone(result.producer_epoch)
        self.assertIsNone(result.logical_time)

    def test_unknown_key_fails_closed(self) -> None:
        raw = signed_wrapper(
            private_key=self.private_key,
            key_id="other-key",
        )

        result = authenticate_signed_assurance(
            raw,
            trust_store=self.trusted_store(),
            provider=self.provider,
        )

        self.assertEqual(
            result.code,
            AuthenticationResultCode.KEY_UNKNOWN,
        )

    def test_revoked_key_beats_valid_signature(self) -> None:
        raw = signed_wrapper(private_key=self.private_key)

        result = authenticate_signed_assurance(
            raw,
            trust_store=self.trusted_store(
                lifecycle_state="REVOKED",
            ),
            provider=self.provider,
        )

        self.assertEqual(
            result.code,
            AuthenticationResultCode.KEY_REVOKED,
        )
        self.assertFalse(result.authenticated)
        self.assertIsNone(result.crypto_code)

    def test_retired_key_beats_valid_signature_for_new_use(self) -> None:
        raw = signed_wrapper(private_key=self.private_key)

        result = authenticate_signed_assurance(
            raw,
            trust_store=self.trusted_store(
                lifecycle_state="RETIRED",
            ),
            provider=self.provider,
        )

        self.assertEqual(
            result.code,
            AuthenticationResultCode.KEY_RETIRED_FOR_NEW_USE,
        )
        self.assertFalse(result.authenticated)
        self.assertIsNone(result.crypto_code)

    def test_wrong_trusted_public_key_is_signature_invalid(self) -> None:
        raw = signed_wrapper(private_key=self.private_key)
        _, other_public = generate_test_keypair()

        result = authenticate_signed_assurance(
            raw,
            trust_store=self.trusted_store(public_key=other_public),
            provider=self.provider,
        )

        self.assertEqual(
            result.code,
            AuthenticationResultCode.SIGNATURE_INVALID,
        )
        self.assertFalse(result.authenticated)

    def test_changed_signed_content_is_signature_invalid(self) -> None:
        raw = json.loads(
            signed_wrapper(private_key=self.private_key).decode("utf-8")
        )

        raw["assurance_object"]["logical_time"] = 2

        result = authenticate_signed_assurance(
            encoded(raw),
            trust_store=self.trusted_store(),
            provider=self.provider,
        )

        self.assertEqual(
            result.code,
            AuthenticationResultCode.SIGNATURE_INVALID,
        )

    def test_unsupported_algorithm_fails_closed(self) -> None:
        raw = json.loads(
            signed_wrapper(private_key=self.private_key).decode("utf-8")
        )
        raw["signature_algorithm"] = "rsa"

        result = authenticate_signed_assurance(
            encoded(raw),
            trust_store=self.trusted_store(),
            provider=self.provider,
        )

        self.assertEqual(
            result.code,
            AuthenticationResultCode.ALGORITHM_UNSUPPORTED,
        )

    def test_signature_encoding_failure_is_not_signature_invalid(self) -> None:
        raw = json.loads(
            signed_wrapper(private_key=self.private_key).decode("utf-8")
        )
        raw["signature"] = raw["signature"] + "="

        result = authenticate_signed_assurance(
            encoded(raw),
            trust_store=self.trusted_store(),
            provider=self.provider,
        )

        self.assertEqual(
            result.code,
            AuthenticationResultCode.SIGNATURE_ENCODING_INVALID,
        )

    def test_unsupported_transcript_version_fails_closed(self) -> None:
        raw = json.loads(
            signed_wrapper(private_key=self.private_key).decode("utf-8")
        )
        raw["signature_version"] = "guardian-f401:m15:signed-assurance:v2"

        result = authenticate_signed_assurance(
            encoded(raw),
            trust_store=self.trusted_store(),
            provider=self.provider,
        )

        self.assertEqual(
            result.code,
            AuthenticationResultCode.TRANSCRIPT_VERSION_UNSUPPORTED,
        )

    def test_authenticated_result_does_not_claim_freshness(self) -> None:
        result = authenticate_signed_assurance(
            signed_wrapper(private_key=self.private_key),
            trust_store=self.trusted_store(),
            provider=self.provider,
        )

        self.assertTrue(result.authenticated)
        self.assertFalse(hasattr(result, "fresh"))
        self.assertFalse(hasattr(result, "authorized"))
        self.assertFalse(hasattr(result, "physical_truth"))
        self.assertFalse(hasattr(result, "actuation_allowed"))

    def test_non_authenticated_reason_is_preserved(self) -> None:
        raw = signed_wrapper(
            private_key=self.private_key,
            key_id="missing-key",
        )

        result = authenticate_signed_assurance(
            raw,
            trust_store=self.trusted_store(),
            provider=self.provider,
        )

        self.assertEqual(result.code, AuthenticationResultCode.KEY_UNKNOWN)
        self.assertEqual(result.trust_code, TrustStoreErrorCode.KEY_UNKNOWN)
        self.assertIsNotNone(result.detail)


if __name__ == "__main__":
    unittest.main(verbosity=2)
