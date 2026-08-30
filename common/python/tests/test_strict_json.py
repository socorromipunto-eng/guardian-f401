"""Tests for the domain-neutral strict JSON decoder."""

from __future__ import annotations

import unittest

from guardian_common.strict_json import (
    DuplicateMemberError,
    InvalidJsonError,
    InvalidUtf8Error,
    decode_strict_json,
)


class StrictJsonTests(unittest.TestCase):
    def test_valid_object_decodes(self) -> None:
        value = decode_strict_json(b'{"a":1,"nested":{"b":true}}')
        self.assertEqual(value, {"a": 1, "nested": {"b": True}})

    def test_invalid_utf8_rejected(self) -> None:
        with self.assertRaises(InvalidUtf8Error):
            decode_strict_json(b'{"a":"\xff"}')

    def test_malformed_json_rejected(self) -> None:
        with self.assertRaises(InvalidJsonError):
            decode_strict_json(b'{"a":}')

    def test_duplicate_top_level_member_rejected(self) -> None:
        with self.assertRaises(DuplicateMemberError) as ctx:
            decode_strict_json(b'{"a":1,"a":2}')
        self.assertEqual(ctx.exception.key, "a")
        self.assertEqual(str(ctx.exception), "duplicate member 'a'")

    def test_duplicate_nested_member_rejected(self) -> None:
        with self.assertRaises(DuplicateMemberError) as ctx:
            decode_strict_json(b'{"outer":{"a":1,"a":2}}')
        self.assertEqual(ctx.exception.key, "a")

    def test_non_bytes_rejected(self) -> None:
        with self.assertRaises(TypeError):
            decode_strict_json('{"a":1}')  # type: ignore[arg-type]


if __name__ == "__main__":
    unittest.main()
