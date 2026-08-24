"""M15 authorization field-contract tests."""

from __future__ import annotations

import unittest

from guardian_assurance.authorization_fields import (
    UINT64_MAX,
    AuthorizationFieldError,
    validate_authority_id,
    validate_authorization_id,
    validate_authorization_sequence,
)


class M15AuthorizationFieldContractTests(unittest.TestCase):
    def test_valid_authority_id(self) -> None:
        value = "root.bootstrap-authority:01"
        self.assertEqual(validate_authority_id(value), value)

    def test_authority_id_max_length(self) -> None:
        value = "a" * 128
        self.assertEqual(validate_authority_id(value), value)

    def test_authority_id_empty_rejected(self) -> None:
        with self.assertRaises(AuthorizationFieldError):
            validate_authority_id("")

    def test_authority_id_over_max_rejected(self) -> None:
        with self.assertRaises(AuthorizationFieldError):
            validate_authority_id("a" * 129)

    def test_authority_id_whitespace_rejected(self) -> None:
        with self.assertRaises(AuthorizationFieldError):
            validate_authority_id("authority one")

    def test_authority_id_invalid_character_rejected(self) -> None:
        with self.assertRaises(AuthorizationFieldError):
            validate_authority_id("authority/one")

    def test_authority_id_non_string_rejected(self) -> None:
        with self.assertRaises(AuthorizationFieldError):
            validate_authority_id(1)

    def test_valid_authorization_id(self) -> None:
        value = "0123456789abcdef0123456789abcdef"
        self.assertEqual(validate_authorization_id(value), value)

    def test_authorization_id_uppercase_rejected(self) -> None:
        with self.assertRaises(AuthorizationFieldError):
            validate_authorization_id("0123456789ABCDEF0123456789ABCDEF")

    def test_authorization_id_wrong_length_rejected(self) -> None:
        for value in (
            "0" * 31,
            "0" * 33,
        ):
            with self.subTest(value=value):
                with self.assertRaises(AuthorizationFieldError):
                    validate_authorization_id(value)

    def test_authorization_id_non_hex_rejected(self) -> None:
        with self.assertRaises(AuthorizationFieldError):
            validate_authorization_id("g" * 32)

    def test_authorization_id_non_string_rejected(self) -> None:
        with self.assertRaises(AuthorizationFieldError):
            validate_authorization_id(0)

    def test_authorization_sequence_zero_accepted(self) -> None:
        self.assertEqual(validate_authorization_sequence(0), 0)

    def test_authorization_sequence_uint64_max_accepted(self) -> None:
        self.assertEqual(
            validate_authorization_sequence(UINT64_MAX),
            UINT64_MAX,
        )

    def test_authorization_sequence_above_uint64_rejected(self) -> None:
        with self.assertRaises(AuthorizationFieldError):
            validate_authorization_sequence(UINT64_MAX + 1)

    def test_authorization_sequence_negative_rejected(self) -> None:
        with self.assertRaises(AuthorizationFieldError):
            validate_authorization_sequence(-1)

    def test_authorization_sequence_bool_rejected(self) -> None:
        with self.assertRaises(AuthorizationFieldError):
            validate_authorization_sequence(True)

    def test_authorization_sequence_float_rejected(self) -> None:
        with self.assertRaises(AuthorizationFieldError):
            validate_authorization_sequence(1.0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
