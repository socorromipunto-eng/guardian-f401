"""M15 producer identity reuse tests.

These tests prove that trust-store code can reuse the exact producer_id
grammar already enforced by the M14 assurance boundary.
"""

import json
import unittest

from guardian_assurance import AssuranceError, ErrorCode
from guardian_assurance.validation import (
    parse_and_validate_envelope,
    validate_producer_id,
)


def observation(producer_id: str) -> dict[str, object]:
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


class M15ProducerIdentityContractTests(unittest.TestCase):
    def assert_schema_rejected(self, producer_id: object) -> None:
        with self.assertRaises(AssuranceError) as caught:
            validate_producer_id(producer_id)

        self.assertEqual(caught.exception.code, ErrorCode.SCHEMA)

    def test_normal_producer_id_is_accepted(self) -> None:
        value = "plant-a.guardian-01"
        self.assertEqual(validate_producer_id(value), value)

    def test_all_approved_identifier_characters_are_accepted(self) -> None:
        value = "Aa0._:-"
        self.assertEqual(validate_producer_id(value), value)

    def test_one_character_is_accepted(self) -> None:
        self.assertEqual(validate_producer_id("a"), "a")

    def test_exact_128_character_boundary_is_accepted(self) -> None:
        value = "a" * 128
        self.assertEqual(validate_producer_id(value), value)

    def test_empty_producer_id_is_rejected(self) -> None:
        self.assert_schema_rejected("")

    def test_129_character_producer_id_is_rejected(self) -> None:
        self.assert_schema_rejected("a" * 129)

    def test_whitespace_is_rejected(self) -> None:
        self.assert_schema_rejected("plant a")

    def test_non_ascii_identifier_is_rejected(self) -> None:
        self.assert_schema_rejected("planta-é")

    def test_non_string_is_rejected(self) -> None:
        self.assert_schema_rejected(123)

    def test_m14_envelope_uses_same_valid_contract(self) -> None:
        producer_id = "plant-a.guardian-01"
        parsed = parse_and_validate_envelope(
            encoded(observation(producer_id))
        )
        self.assertEqual(parsed["producer_id"], producer_id)

    def test_m14_envelope_uses_same_invalid_contract(self) -> None:
        producer_id = "plant a"

        with self.assertRaises(AssuranceError) as caught:
            parse_and_validate_envelope(
                encoded(observation(producer_id))
            )

        self.assertEqual(caught.exception.code, ErrorCode.SCHEMA)


if __name__ == "__main__":
    unittest.main(verbosity=2)
