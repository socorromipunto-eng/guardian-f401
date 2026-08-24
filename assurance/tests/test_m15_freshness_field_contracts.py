"""M15-03 reuse tests for authoritative M14 freshness-field contracts."""

from __future__ import annotations

import unittest

from guardian_assurance.errors import AssuranceError
from guardian_assurance.validation import (
    parse_and_validate_envelope,
    validate_logical_time,
    validate_producer_epoch,
)


SAFE_INTEGER_MAX = 9_007_199_254_740_991


def envelope() -> dict[str, object]:
    return {
        "domain": "guardian-f401:m14:assurance:v1:observation",
        "object_type": "observation",
        "producer_id": "guardian-primary",
        "producer_epoch": "0123456789abcdef0123456789abcdef",
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


class M15FreshnessFieldContractTests(unittest.TestCase):

    def test_valid_producer_epoch(self) -> None:
        value = "0123456789abcdef0123456789abcdef"
        self.assertEqual(validate_producer_epoch(value), value)

    def test_epoch_exact_32_lowercase_hex_contract(self) -> None:
        invalid = (
            "",
            "0" * 31,
            "0" * 33,
            "A" + "0" * 31,
            "G" + "0" * 31,
            "x" * 32,
        )

        for value in invalid:
            with self.subTest(value=value):
                with self.assertRaises(AssuranceError):
                    validate_producer_epoch(value)

    def test_epoch_non_string_rejected(self) -> None:
        with self.assertRaises(AssuranceError):
            validate_producer_epoch(0)

    def test_logical_time_zero_accepted(self) -> None:
        self.assertEqual(validate_logical_time(0), 0)

    def test_logical_time_safe_max_accepted(self) -> None:
        self.assertEqual(
            validate_logical_time(SAFE_INTEGER_MAX),
            SAFE_INTEGER_MAX,
        )

    def test_logical_time_above_safe_max_rejected(self) -> None:
        with self.assertRaises(AssuranceError):
            validate_logical_time(SAFE_INTEGER_MAX + 1)

    def test_negative_logical_time_rejected(self) -> None:
        with self.assertRaises(AssuranceError):
            validate_logical_time(-1)

    def test_boolean_logical_time_rejected(self) -> None:
        with self.assertRaises(AssuranceError):
            validate_logical_time(True)

    def test_float_logical_time_rejected(self) -> None:
        with self.assertRaises(AssuranceError):
            validate_logical_time(1.0)

    def test_m14_envelope_keeps_epoch_contract(self) -> None:
        value = envelope()
        value["producer_epoch"] = "x" * 32

        with self.assertRaises(AssuranceError):
            parse_and_validate_envelope(value)

    def test_m14_envelope_keeps_logical_time_contract(self) -> None:
        value = envelope()
        value["logical_time"] = True

        with self.assertRaises(AssuranceError):
            parse_and_validate_envelope(value)


if __name__ == "__main__":
    unittest.main(verbosity=2)
