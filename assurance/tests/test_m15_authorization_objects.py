"""Closed M15 authorization-object parser tests."""

from __future__ import annotations

import json
import unittest

from guardian_assurance.authorization_fields import UINT64_MAX
from guardian_assurance.authorization_objects import (
    AUTHORIZATION_OBJECT_MAX_RAW_BYTES,
    BOOTSTRAP_SCHEMA_VERSION,
    BOOTSTRAP_TYPE,
    EPOCH_TRANSITION_SCHEMA_VERSION,
    EPOCH_TRANSITION_TYPE,
    HIGH_WATER_ESTABLISHED,
    HIGH_WATER_UNSET,
    AuthorizationObjectError,
    AuthorizationObjectErrorCode,
    BootstrapAuthorization,
    EpochTransitionAuthorization,
    parse_and_validate_authorization_object,
)


def encoded(value: object) -> bytes:
    return json.dumps(
        value,
        separators=(",", ":"),
        ensure_ascii=True,
    ).encode("utf-8")


def bootstrap() -> dict[str, object]:
    return {
        "schema_version": BOOTSTRAP_SCHEMA_VERSION,
        "authorization_type": BOOTSTRAP_TYPE,
        "authorization_id": "0123456789abcdef0123456789abcdef",
        "authority_id": "root.bootstrap-authority:01",
        "producer_id": "plant-a.guardian-01",
        "authorization_sequence": 1,
        "producer_epoch": "11111111111111111111111111111111",
        "initial_high_water_state": HIGH_WATER_UNSET,
        "initial_logical_time": None,
    }


def transition() -> dict[str, object]:
    return {
        "schema_version": EPOCH_TRANSITION_SCHEMA_VERSION,
        "authorization_type": EPOCH_TRANSITION_TYPE,
        "authorization_id": "abcdefabcdefabcdefabcdefabcdefab",
        "authority_id": "root.epoch-authority:01",
        "producer_id": "plant-a.guardian-01",
        "authorization_sequence": 7,
        "from_epoch": "11111111111111111111111111111111",
        "to_epoch": "22222222222222222222222222222222",
        "transition_sequence": 3,
    }


class M15AuthorizationObjectTests(unittest.TestCase):
    def assert_code(
        self,
        raw: bytes,
        code: AuthorizationObjectErrorCode,
    ) -> None:
        with self.assertRaises(AuthorizationObjectError) as caught:
            parse_and_validate_authorization_object(raw)
        self.assertEqual(caught.exception.code, code)

    def test_valid_bootstrap_unset(self) -> None:
        result = parse_and_validate_authorization_object(encoded(bootstrap()))
        self.assertIsInstance(result, BootstrapAuthorization)
        self.assertEqual(result.initial_high_water_state, HIGH_WATER_UNSET)
        self.assertIsNone(result.initial_logical_time)

    def test_valid_bootstrap_established(self) -> None:
        value = bootstrap()
        value["initial_high_water_state"] = HIGH_WATER_ESTABLISHED
        value["initial_logical_time"] = 42
        result = parse_and_validate_authorization_object(encoded(value))
        self.assertIsInstance(result, BootstrapAuthorization)
        self.assertEqual(result.initial_logical_time, 42)

    def test_valid_epoch_transition(self) -> None:
        result = parse_and_validate_authorization_object(encoded(transition()))
        self.assertIsInstance(result, EpochTransitionAuthorization)
        self.assertEqual(result.transition_sequence, 3)

    def test_unknown_authorization_type_rejected(self) -> None:
        value = bootstrap()
        value["authorization_type"] = "admin-override"
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_bootstrap_wrong_schema_rejected(self) -> None:
        value = bootstrap()
        value["schema_version"] = EPOCH_TRANSITION_SCHEMA_VERSION
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_transition_wrong_schema_rejected(self) -> None:
        value = transition()
        value["schema_version"] = BOOTSTRAP_SCHEMA_VERSION
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_bootstrap_missing_field_rejected(self) -> None:
        value = bootstrap()
        value.pop("producer_epoch")
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_bootstrap_extra_field_rejected(self) -> None:
        value = bootstrap()
        value["from_epoch"] = "0" * 32
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_transition_missing_field_rejected(self) -> None:
        value = transition()
        value.pop("to_epoch")
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_transition_extra_field_rejected(self) -> None:
        value = transition()
        value["producer_epoch"] = "3" * 32
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_duplicate_member_rejected(self) -> None:
        raw = (
            b'{"schema_version":"guardian-f401:m15:bootstrap-authorization:v1",'
            b'"schema_version":"guardian-f401:m15:bootstrap-authorization:v1"}'
        )
        self.assert_code(raw, AuthorizationObjectErrorCode.DUPLICATE_KEY)

    def test_invalid_utf8_rejected(self) -> None:
        self.assert_code(b"\xff", AuthorizationObjectErrorCode.INVALID_UTF8)

    def test_invalid_json_rejected(self) -> None:
        self.assert_code(b"{", AuthorizationObjectErrorCode.INVALID_JSON)

    def test_nan_rejected(self) -> None:
        raw = encoded(bootstrap()).replace(b"null", b"NaN")
        self.assert_code(raw, AuthorizationObjectErrorCode.INVALID_JSON)

    def test_float_rejected(self) -> None:
        value = bootstrap()
        value["authorization_sequence"] = 1.5
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_raw_limit_rejected(self) -> None:
        raw = b" " * (AUTHORIZATION_OBJECT_MAX_RAW_BYTES + 1)
        self.assert_code(raw, AuthorizationObjectErrorCode.RAW_LIMIT)

    def test_invalid_authority_id_rejected(self) -> None:
        value = bootstrap()
        value["authority_id"] = "bad authority"
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_invalid_authorization_id_rejected(self) -> None:
        value = bootstrap()
        value["authorization_id"] = "A" * 32
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_invalid_producer_id_rejected(self) -> None:
        value = bootstrap()
        value["producer_id"] = "bad producer"
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_invalid_producer_epoch_rejected(self) -> None:
        value = bootstrap()
        value["producer_epoch"] = "x" * 32
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_authorization_sequence_uint64_max_accepted(self) -> None:
        value = bootstrap()
        value["authorization_sequence"] = UINT64_MAX
        result = parse_and_validate_authorization_object(encoded(value))
        self.assertEqual(result.authorization_sequence, UINT64_MAX)

    def test_authorization_sequence_over_uint64_rejected(self) -> None:
        value = bootstrap()
        value["authorization_sequence"] = UINT64_MAX + 1
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_authorization_sequence_bool_rejected(self) -> None:
        value = bootstrap()
        value["authorization_sequence"] = True
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_unset_requires_null_logical_time(self) -> None:
        value = bootstrap()
        value["initial_logical_time"] = 0
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_established_requires_logical_time(self) -> None:
        value = bootstrap()
        value["initial_high_water_state"] = HIGH_WATER_ESTABLISHED
        value["initial_logical_time"] = None
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_established_logical_time_zero_is_valid(self) -> None:
        value = bootstrap()
        value["initial_high_water_state"] = HIGH_WATER_ESTABLISHED
        value["initial_logical_time"] = 0
        result = parse_and_validate_authorization_object(encoded(value))
        self.assertEqual(result.initial_logical_time, 0)

    def test_unknown_high_water_state_rejected(self) -> None:
        value = bootstrap()
        value["initial_high_water_state"] = "ZERO"
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_transition_same_epoch_rejected(self) -> None:
        value = transition()
        value["to_epoch"] = value["from_epoch"]
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_transition_sequence_uint64_max_accepted(self) -> None:
        value = transition()
        value["transition_sequence"] = UINT64_MAX
        result = parse_and_validate_authorization_object(encoded(value))
        self.assertEqual(result.transition_sequence, UINT64_MAX)

    def test_transition_sequence_over_uint64_rejected(self) -> None:
        value = transition()
        value["transition_sequence"] = UINT64_MAX + 1
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_transition_sequence_bool_rejected(self) -> None:
        value = transition()
        value["transition_sequence"] = True
        self.assert_code(encoded(value), AuthorizationObjectErrorCode.SCHEMA)

    def test_parsed_object_does_not_claim_authentication(self) -> None:
        result = parse_and_validate_authorization_object(encoded(bootstrap()))
        self.assertFalse(hasattr(result, "authenticated"))
        self.assertFalse(hasattr(result, "authorized"))
        self.assertFalse(hasattr(result, "consumed"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
