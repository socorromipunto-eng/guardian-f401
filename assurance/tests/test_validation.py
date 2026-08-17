"""Tests for bounded parsing and closed-envelope validation."""

import json
import unittest

from guardian_assurance import AssuranceError, AssuranceLimits, ErrorCode, parse_and_validate_envelope


def valid_record() -> dict:
    return {
        "domain": "guardian-f401:m14:assurance:v1:observation",
        "object_type": "observation",
        "producer_id": "plant-a.guardian-01",
        "producer_epoch": "0123456789abcdef0123456789abcdef",
        "object_id": "fedcba9876543210fedcba9876543210",
        "logical_time": 1,
        "payload": {
            "schema_version": "m14.observation.v1",
            "subject_id": "machine-01",
            "claim": "operational_state",
            "result": "UNKNOWN",
            "evidence_digest": "a" * 64,
        },
    }


class ValidationTests(unittest.TestCase):
    def encoded(self, value: dict) -> bytes:
        return json.dumps(value, separators=(",", ":")).encode()

    def assert_code(self, raw: bytes, code: ErrorCode, limits=None) -> None:
        with self.assertRaises(AssuranceError) as caught:
            parse_and_validate_envelope(raw, limits)
        self.assertEqual(caught.exception.code, code)

    def test_valid_envelope(self) -> None:
        self.assertEqual(parse_and_validate_envelope(self.encoded(valid_record())), valid_record())

    def test_duplicate_member_rejected(self) -> None:
        raw = self.encoded(valid_record()).replace(b'"payload":', b'"payload":{},"payload":')
        self.assert_code(raw, ErrorCode.DUPLICATE_KEY)

    def test_escaped_duplicate_member_rejected(self) -> None:
        self.assert_code(b'{"a":1,"\\u0061":2}', ErrorCode.DUPLICATE_KEY)

    def test_invalid_utf8_rejected(self) -> None:
        self.assert_code(b"\xff", ErrorCode.INVALID_UTF8)

    def test_raw_limit_rejected(self) -> None:
        self.assert_code(b"{}", ErrorCode.RAW_LIMIT, AssuranceLimits(max_raw_bytes=1))

    def test_depth_boundary(self) -> None:
        record = valid_record()
        record["payload"] = {"a": {"b": {"c": 1}}}
        self.assert_code(self.encoded(record), ErrorCode.STRUCTURAL_LIMIT, AssuranceLimits(max_depth=4))

    def test_extra_member_rejected(self) -> None:
        record = valid_record()
        record["extra"] = True
        self.assert_code(self.encoded(record), ErrorCode.SCHEMA)

    def test_domain_mismatch_rejected(self) -> None:
        record = valid_record()
        record["domain"] = "guardian-f401:m14:assurance:v1:decision"
        self.assert_code(self.encoded(record), ErrorCode.DOMAIN)

    def test_boolean_logical_time_rejected(self) -> None:
        record = valid_record()
        record["logical_time"] = True
        self.assert_code(self.encoded(record), ErrorCode.SCHEMA)


if __name__ == "__main__":
    unittest.main(verbosity=2)
