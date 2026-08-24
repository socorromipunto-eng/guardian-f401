"""Guardian M15 trust-store runtime tests."""

from __future__ import annotations

import base64
import json
import unittest

from guardian_assurance.trust_store import (
    ED25519_ALGORITHM,
    TRUST_STORE_MAX_RAW_BYTES,
    TRUST_STORE_MAX_RECORDS,
    TrustStoreError,
    TrustStoreErrorCode,
    parse_and_validate_trust_store,
    resolve_trusted_credential,
)


def key_wire(byte: int = 1) -> str:
    raw = bytes([byte]) * 32
    return base64.urlsafe_b64encode(raw).rstrip(b"=").decode("ascii")


def record(
    producer_id: str = "guardian-primary",
    key_id: str = "node-a-key-001",
    algorithm: str = "ed25519",
    public_key: str | None = None,
    lifecycle_state: str = "ACTIVE",
) -> dict[str, object]:
    return {
        "producer_id": producer_id,
        "key_id": key_id,
        "algorithm": algorithm,
        "public_key": public_key if public_key is not None else key_wire(),
        "lifecycle_state": lifecycle_state,
    }


def store(
    *,
    environment: str = "TEST",
    records: list[dict[str, object]] | None = None,
) -> dict[str, object]:
    return {
        "schema_version": "guardian-f401:m15:trust-store:v1",
        "environment": environment,
        "records": records if records is not None else [],
    }


def encoded(value: object) -> bytes:
    return json.dumps(value, separators=(",", ":")).encode("utf-8")


class M15TrustStoreTests(unittest.TestCase):
    def assert_code(
        self,
        raw: bytes,
        code: TrustStoreErrorCode,
        *,
        environment: str = "TEST",
    ) -> None:
        with self.assertRaises(TrustStoreError) as caught:
            parse_and_validate_trust_store(
                raw,
                expected_environment=environment,
            )
        self.assertEqual(caught.exception.code, code)

    def test_constants_are_frozen(self) -> None:
        self.assertEqual(TRUST_STORE_MAX_RAW_BYTES, 1_048_576)
        self.assertEqual(TRUST_STORE_MAX_RECORDS, 4_096)
        self.assertEqual(ED25519_ALGORITHM, "ed25519")

    def test_empty_test_store_is_valid(self) -> None:
        parsed = parse_and_validate_trust_store(
            encoded(store(environment="TEST")),
            expected_environment="TEST",
        )
        self.assertEqual(parsed.environment, "TEST")
        self.assertEqual(len(parsed.records), 0)

    def test_empty_production_store_is_valid(self) -> None:
        parsed = parse_and_validate_trust_store(
            encoded(store(environment="PRODUCTION")),
            expected_environment="PRODUCTION",
        )
        self.assertEqual(parsed.environment, "PRODUCTION")

    def test_environment_mismatch_fails_closed(self) -> None:
        self.assert_code(
            encoded(store(environment="PRODUCTION")),
            TrustStoreErrorCode.ENVIRONMENT_MISMATCH,
            environment="TEST",
        )

    def test_unknown_top_level_member_rejected(self) -> None:
        value = store()
        value["extra"] = True
        self.assert_code(encoded(value), TrustStoreErrorCode.SCHEMA)

    def test_missing_top_level_member_rejected(self) -> None:
        value = store()
        del value["records"]
        self.assert_code(encoded(value), TrustStoreErrorCode.SCHEMA)

    def test_duplicate_top_level_member_rejected(self) -> None:
        raw = b'{"schema_version":"guardian-f401:m15:trust-store:v1","schema_version":"guardian-f401:m15:trust-store:v1","environment":"TEST","records":[]}'
        self.assert_code(raw, TrustStoreErrorCode.DUPLICATE_KEY)

    def test_unknown_record_member_rejected(self) -> None:
        item = record()
        item["extra"] = True
        self.assert_code(
            encoded(store(records=[item])),
            TrustStoreErrorCode.SCHEMA,
        )

    def test_duplicate_record_member_rejected(self) -> None:
        raw = (
            b'{"schema_version":"guardian-f401:m15:trust-store:v1","environment":"TEST","records":[{"producer_id":"guardian-primary","producer_id":"guardian-primary","key_id":"node-a-key-001","algorithm":"ed25519","public_key":"'
            + key_wire().encode("ascii")
            + b'","lifecycle_state":"ACTIVE"}]}'
        )
        self.assert_code(raw, TrustStoreErrorCode.DUPLICATE_KEY)

    def test_valid_public_key_is_32_bytes(self) -> None:
        parsed = parse_and_validate_trust_store(
            encoded(store(records=[record()])),
            expected_environment="TEST",
        )
        self.assertEqual(len(parsed.records[0].public_key_bytes), 32)

    def test_padded_public_key_rejected(self) -> None:
        item = record(public_key=key_wire() + "=")
        self.assert_code(
            encoded(store(records=[item])),
            TrustStoreErrorCode.SCHEMA,
        )

    def test_public_key_invalid_alphabet_rejected(self) -> None:
        item = record(public_key="+" + key_wire()[1:])
        self.assert_code(
            encoded(store(records=[item])),
            TrustStoreErrorCode.SCHEMA,
        )

    def test_public_key_whitespace_rejected(self) -> None:
        wire = key_wire()
        item = record(public_key=wire[:10] + " " + wire[11:])
        self.assert_code(
            encoded(store(records=[item])),
            TrustStoreErrorCode.SCHEMA,
        )

    def test_31_byte_public_key_rejected(self) -> None:
        wire = base64.urlsafe_b64encode(b"x" * 31).rstrip(b"=").decode("ascii")
        self.assert_code(
            encoded(store(records=[record(public_key=wire)])),
            TrustStoreErrorCode.SCHEMA,
        )

    def test_33_byte_public_key_rejected(self) -> None:
        wire = base64.urlsafe_b64encode(b"x" * 33).rstrip(b"=").decode("ascii")
        self.assert_code(
            encoded(store(records=[record(public_key=wire)])),
            TrustStoreErrorCode.SCHEMA,
        )

    def test_invalid_producer_id_rejected(self) -> None:
        self.assert_code(
            encoded(store(records=[record(producer_id="bad producer")])),
            TrustStoreErrorCode.SCHEMA,
        )

    def test_invalid_key_id_rejected(self) -> None:
        self.assert_code(
            encoded(store(records=[record(key_id="bad key")])),
            TrustStoreErrorCode.SCHEMA,
        )

    def test_unsupported_record_algorithm_rejected(self) -> None:
        self.assert_code(
            encoded(store(records=[record(algorithm="rsa")])),
            TrustStoreErrorCode.ALGORITHM_UNSUPPORTED,
        )

    def test_invalid_lifecycle_rejected(self) -> None:
        self.assert_code(
            encoded(store(records=[record(lifecycle_state="UNKNOWN")])),
            TrustStoreErrorCode.SCHEMA,
        )

    def test_duplicate_credential_record_rejected(self) -> None:
        item = record()
        self.assert_code(
            encoded(store(records=[item, dict(item)])),
            TrustStoreErrorCode.SCHEMA,
        )

    def test_duplicate_producer_key_mapping_rejected(self) -> None:
        first = record(public_key=key_wire(1))
        second = record(public_key=key_wire(2))
        self.assert_code(
            encoded(store(records=[first, second])),
            TrustStoreErrorCode.SCHEMA,
        )

    def test_active_record_resolves(self) -> None:
        parsed = parse_and_validate_trust_store(
            encoded(store(records=[record()])),
            expected_environment="TEST",
        )
        result = resolve_trusted_credential(
            parsed,
            producer_id="guardian-primary",
            key_id="node-a-key-001",
            algorithm="ed25519",
        )
        self.assertEqual(result.code, TrustStoreErrorCode.RESOLVED)
        self.assertTrue(result.resolved)
        self.assertIsNotNone(result.record)

    def test_retired_record_rejected_for_new_authentication(self) -> None:
        parsed = parse_and_validate_trust_store(
            encoded(store(records=[record(lifecycle_state="RETIRED")])),
            expected_environment="TEST",
        )
        result = resolve_trusted_credential(
            parsed,
            producer_id="guardian-primary",
            key_id="node-a-key-001",
            algorithm="ed25519",
        )
        self.assertEqual(
            result.code,
            TrustStoreErrorCode.KEY_RETIRED_FOR_NEW_USE,
        )

    def test_retired_record_may_resolve_for_historical_context(self) -> None:
        parsed = parse_and_validate_trust_store(
            encoded(store(records=[record(lifecycle_state="RETIRED")])),
            expected_environment="TEST",
        )
        result = resolve_trusted_credential(
            parsed,
            producer_id="guardian-primary",
            key_id="node-a-key-001",
            algorithm="ed25519",
            for_new_authentication=False,
        )
        self.assertEqual(result.code, TrustStoreErrorCode.RESOLVED)

    def test_revoked_record_fails_current_authentication(self) -> None:
        parsed = parse_and_validate_trust_store(
            encoded(store(records=[record(lifecycle_state="REVOKED")])),
            expected_environment="TEST",
        )
        result = resolve_trusted_credential(
            parsed,
            producer_id="guardian-primary",
            key_id="node-a-key-001",
            algorithm="ed25519",
        )
        self.assertEqual(result.code, TrustStoreErrorCode.KEY_REVOKED)

    def test_unknown_producer_is_explicit(self) -> None:
        parsed = parse_and_validate_trust_store(
            encoded(store(records=[record()])),
            expected_environment="TEST",
        )
        result = resolve_trusted_credential(
            parsed,
            producer_id="other",
            key_id="node-a-key-001",
            algorithm="ed25519",
        )
        self.assertEqual(result.code, TrustStoreErrorCode.IDENTITY_UNKNOWN)

    def test_unknown_key_is_explicit(self) -> None:
        parsed = parse_and_validate_trust_store(
            encoded(store(records=[record()])),
            expected_environment="TEST",
        )
        result = resolve_trusted_credential(
            parsed,
            producer_id="guardian-primary",
            key_id="missing",
            algorithm="ed25519",
        )
        self.assertEqual(result.code, TrustStoreErrorCode.KEY_UNKNOWN)

    def test_unsupported_lookup_algorithm_is_explicit(self) -> None:
        parsed = parse_and_validate_trust_store(
            encoded(store(records=[record()])),
            expected_environment="TEST",
        )
        result = resolve_trusted_credential(
            parsed,
            producer_id="guardian-primary",
            key_id="node-a-key-001",
            algorithm="rsa",
        )
        self.assertEqual(
            result.code,
            TrustStoreErrorCode.ALGORITHM_UNSUPPORTED,
        )

    def test_raw_oversize_rejected_before_utf8(self) -> None:
        raw = b"\xff" * (TRUST_STORE_MAX_RAW_BYTES + 1)
        self.assert_code(raw, TrustStoreErrorCode.RAW_LIMIT)

    def test_raw_exact_boundary_reaches_parser(self) -> None:
        raw = encoded(store())
        padded = raw + b" " * (TRUST_STORE_MAX_RAW_BYTES - len(raw))
        parsed = parse_and_validate_trust_store(
            padded,
            expected_environment="TEST",
        )
        self.assertEqual(parsed.environment, "TEST")

    def test_4097_records_rejected(self) -> None:
        repeated = [
            record(
                producer_id=f"p{i}",
                key_id=f"k{i}",
            )
            for i in range(TRUST_STORE_MAX_RECORDS + 1)
        ]
        self.assert_code(
            encoded(store(records=repeated)),
            TrustStoreErrorCode.RECORD_LIMIT,
        )

    def test_4096_records_pass_record_count_gate(self) -> None:
        repeated = [
            record(
                producer_id=f"p{i}",
                key_id=f"k{i}",
            )
            for i in range(TRUST_STORE_MAX_RECORDS)
        ]
        parsed = parse_and_validate_trust_store(
            encoded(store(records=repeated)),
            expected_environment="TEST",
        )
        self.assertEqual(len(parsed.records), TRUST_STORE_MAX_RECORDS)

    def test_hashes_are_reproducible(self) -> None:
        raw = encoded(store(records=[record()]))
        first = parse_and_validate_trust_store(raw, expected_environment="TEST")
        second = parse_and_validate_trust_store(raw, expected_environment="TEST")
        self.assertEqual(first.raw_sha256, second.raw_sha256)
        self.assertEqual(first.canonical_sha256, second.canonical_sha256)

    def test_json_member_order_does_not_change_canonical_hash(self) -> None:
        first = encoded(store(records=[record()]))
        second = (
            b'{"records":[{"public_key":"'
            + key_wire().encode("ascii")
            + b'","algorithm":"ed25519","key_id":"node-a-key-001","producer_id":"guardian-primary","lifecycle_state":"ACTIVE"}],"environment":"TEST","schema_version":"guardian-f401:m15:trust-store:v1"}'
        )
        a = parse_and_validate_trust_store(first, expected_environment="TEST")
        b = parse_and_validate_trust_store(second, expected_environment="TEST")
        self.assertEqual(a.canonical_sha256, b.canonical_sha256)

    def test_resolved_result_does_not_claim_authenticated(self) -> None:
        parsed = parse_and_validate_trust_store(
            encoded(store(records=[record()])),
            expected_environment="TEST",
        )
        result = resolve_trusted_credential(
            parsed,
            producer_id="guardian-primary",
            key_id="node-a-key-001",
            algorithm="ed25519",
        )
        self.assertTrue(result.resolved)
        self.assertFalse(hasattr(result, "authenticated"))
        self.assertFalse(hasattr(result, "authorized"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
