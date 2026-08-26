"""M15 dedicated authorization trust-store tests."""

from __future__ import annotations

import base64
import json
import unittest

from guardian_assurance.authorization_trust_store import (
    AUTHORIZATION_TRUST_STORE_MAX_RAW_BYTES,
    AUTHORIZATION_TRUST_STORE_MAX_RECORDS,
    AUTHORIZATION_TRUST_STORE_SCHEMA_VERSION,
    BOOTSTRAP_AUTHORITY,
    EPOCH_TRANSITION_AUTHORITY,
    AuthorizationTrustError,
    AuthorizationTrustErrorCode,
    parse_and_validate_authorization_trust_store,
    resolve_authorization_credential,
)


def key_wire(fill: int = 1) -> str:
    return base64.urlsafe_b64encode(bytes([fill]) * 32).rstrip(b"=").decode("ascii")


def record(
    *,
    authority_id: str = "root.bootstrap-authority:01",
    key_id: str = "bootstrap-key-001",
    algorithm: str = "ed25519",
    public_key: str | None = None,
    lifecycle_state: str = "ACTIVE",
    capabilities: list[str] | None = None,
) -> dict[str, object]:
    return {
        "authority_id": authority_id,
        "key_id": key_id,
        "algorithm": algorithm,
        "public_key": public_key or key_wire(),
        "lifecycle_state": lifecycle_state,
        "capabilities": capabilities or [BOOTSTRAP_AUTHORITY],
    }


def store(
    *,
    environment: str = "TEST",
    records: list[dict[str, object]] | None = None,
) -> dict[str, object]:
    return {
        "schema_version": AUTHORIZATION_TRUST_STORE_SCHEMA_VERSION,
        "environment": environment,
        "records": records if records is not None else [record()],
    }


def encoded(value: object) -> bytes:
    return json.dumps(value, separators=(",", ":"), ensure_ascii=True).encode("utf-8")


class M15AuthorizationTrustStoreTests(unittest.TestCase):
    def parse(self, value: object, environment: str = "TEST"):
        return parse_and_validate_authorization_trust_store(
            encoded(value),
            expected_environment=environment,
        )

    def test_valid_store_parses(self) -> None:
        parsed = self.parse(store())
        self.assertEqual(parsed.environment, "TEST")
        self.assertEqual(len(parsed.records), 1)

    def test_wrong_schema_rejected(self) -> None:
        value = store()
        value["schema_version"] = "guardian-f401:m15:trust-store:v1"
        with self.assertRaises(AuthorizationTrustError):
            self.parse(value)

    def test_environment_mismatch_rejected(self) -> None:
        with self.assertRaises(AuthorizationTrustError) as caught:
            self.parse(store(environment="PROD"), environment="TEST")
        self.assertEqual(
            caught.exception.code,
            AuthorizationTrustErrorCode.ENVIRONMENT_MISMATCH,
        )

    def test_unknown_top_level_member_rejected(self) -> None:
        value = store()
        value["producer_id"] = "must-not-exist"
        with self.assertRaises(AuthorizationTrustError):
            self.parse(value)

    def test_unknown_record_member_rejected(self) -> None:
        value = record()
        value["producer_id"] = "must-not-exist"
        with self.assertRaises(AuthorizationTrustError):
            self.parse(store(records=[value]))

    def test_invalid_authority_id_rejected(self) -> None:
        with self.assertRaises(AuthorizationTrustError):
            self.parse(store(records=[record(authority_id="bad authority")]))

    def test_invalid_key_id_rejected(self) -> None:
        with self.assertRaises(AuthorizationTrustError):
            self.parse(store(records=[record(key_id="bad key")]))

    def test_non_ed25519_algorithm_rejected(self) -> None:
        with self.assertRaises(AuthorizationTrustError):
            self.parse(store(records=[record(algorithm="rsa")]))

    def test_invalid_public_key_length_rejected(self) -> None:
        wire = base64.urlsafe_b64encode(b"x" * 31).rstrip(b"=").decode("ascii")
        with self.assertRaises(AuthorizationTrustError):
            self.parse(store(records=[record(public_key=wire)]))

    def test_padded_public_key_rejected(self) -> None:
        with self.assertRaises(AuthorizationTrustError):
            self.parse(store(records=[record(public_key=key_wire() + "=")]))

    def test_unknown_lifecycle_rejected(self) -> None:
        with self.assertRaises(AuthorizationTrustError):
            self.parse(store(records=[record(lifecycle_state="UNKNOWN")]))

    def test_unknown_capability_rejected(self) -> None:
        with self.assertRaises(AuthorizationTrustError):
            self.parse(store(records=[record(capabilities=["ADMIN"])]))

    def test_duplicate_capability_rejected(self) -> None:
        with self.assertRaises(AuthorizationTrustError):
            self.parse(store(records=[
                record(capabilities=[BOOTSTRAP_AUTHORITY, BOOTSTRAP_AUTHORITY])
            ]))

    def test_both_capabilities_explicitly_allowed(self) -> None:
        parsed = self.parse(store(records=[
            record(capabilities=[BOOTSTRAP_AUTHORITY, EPOCH_TRANSITION_AUTHORITY])
        ]))
        self.assertEqual(len(parsed.records[0].capabilities), 2)

    def test_duplicate_authority_key_mapping_rejected(self) -> None:
        first = record(public_key=key_wire(1))
        second = record(public_key=key_wire(2))
        with self.assertRaises(AuthorizationTrustError):
            self.parse(store(records=[first, second]))

    def test_active_bootstrap_credential_resolves(self) -> None:
        parsed = self.parse(store())
        result = resolve_authorization_credential(
            parsed,
            authority_id="root.bootstrap-authority:01",
            key_id="bootstrap-key-001",
            algorithm="ed25519",
            required_capability=BOOTSTRAP_AUTHORITY,
        )
        self.assertEqual(result.code, AuthorizationTrustErrorCode.RESOLVED)
        self.assertTrue(result.resolved)

    def test_bootstrap_credential_does_not_imply_epoch_capability(self) -> None:
        parsed = self.parse(store())
        result = resolve_authorization_credential(
            parsed,
            authority_id="root.bootstrap-authority:01",
            key_id="bootstrap-key-001",
            algorithm="ed25519",
            required_capability=EPOCH_TRANSITION_AUTHORITY,
        )
        self.assertEqual(
            result.code,
            AuthorizationTrustErrorCode.CAPABILITY_MISMATCH,
        )
        self.assertFalse(result.resolved)

    def test_unknown_authority_is_explicit(self) -> None:
        parsed = self.parse(store())
        result = resolve_authorization_credential(
            parsed,
            authority_id="missing",
            key_id="bootstrap-key-001",
            algorithm="ed25519",
            required_capability=BOOTSTRAP_AUTHORITY,
        )
        self.assertEqual(result.code, AuthorizationTrustErrorCode.AUTHORITY_UNKNOWN)

    def test_unknown_key_is_explicit(self) -> None:
        parsed = self.parse(store())
        result = resolve_authorization_credential(
            parsed,
            authority_id="root.bootstrap-authority:01",
            key_id="missing",
            algorithm="ed25519",
            required_capability=BOOTSTRAP_AUTHORITY,
        )
        self.assertEqual(result.code, AuthorizationTrustErrorCode.KEY_UNKNOWN)

    def test_revoked_credential_fails(self) -> None:
        parsed = self.parse(store(records=[record(lifecycle_state="REVOKED")]))
        result = resolve_authorization_credential(
            parsed,
            authority_id="root.bootstrap-authority:01",
            key_id="bootstrap-key-001",
            algorithm="ed25519",
            required_capability=BOOTSTRAP_AUTHORITY,
        )
        self.assertEqual(result.code, AuthorizationTrustErrorCode.KEY_REVOKED)

    def test_retired_credential_rejected_for_new_authorization(self) -> None:
        parsed = self.parse(store(records=[record(lifecycle_state="RETIRED")]))
        result = resolve_authorization_credential(
            parsed,
            authority_id="root.bootstrap-authority:01",
            key_id="bootstrap-key-001",
            algorithm="ed25519",
            required_capability=BOOTSTRAP_AUTHORITY,
        )
        self.assertEqual(
            result.code,
            AuthorizationTrustErrorCode.KEY_RETIRED_FOR_NEW_USE,
        )

    def test_retired_credential_may_resolve_historical_context(self) -> None:
        parsed = self.parse(store(records=[record(lifecycle_state="RETIRED")]))
        result = resolve_authorization_credential(
            parsed,
            authority_id="root.bootstrap-authority:01",
            key_id="bootstrap-key-001",
            algorithm="ed25519",
            required_capability=BOOTSTRAP_AUTHORITY,
            for_new_authorization=False,
        )
        self.assertEqual(result.code, AuthorizationTrustErrorCode.RESOLVED)

    def test_resolved_result_does_not_claim_authenticated(self) -> None:
        parsed = self.parse(store())
        result = resolve_authorization_credential(
            parsed,
            authority_id="root.bootstrap-authority:01",
            key_id="bootstrap-key-001",
            algorithm="ed25519",
            required_capability=BOOTSTRAP_AUTHORITY,
        )
        self.assertTrue(result.resolved)
        self.assertFalse(hasattr(result, "authenticated"))
        self.assertFalse(hasattr(result, "authorized"))

    def test_record_limit_constant_is_authorization_specific(self) -> None:
        self.assertEqual(AUTHORIZATION_TRUST_STORE_MAX_RECORDS, 1024)

    def test_raw_limit_rejected_before_parse(self) -> None:
        raw = b" " * (AUTHORIZATION_TRUST_STORE_MAX_RAW_BYTES + 1)
        with self.assertRaises(AuthorizationTrustError) as caught:
            parse_and_validate_authorization_trust_store(
                raw,
                expected_environment="TEST",
            )
        self.assertEqual(
            caught.exception.code,
            AuthorizationTrustErrorCode.RAW_LIMIT,
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
