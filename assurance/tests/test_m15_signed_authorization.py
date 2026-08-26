"""M15 SignedAuthorizationEnvelopeV1 parser tests."""

from __future__ import annotations

import json
import unittest

from guardian_assurance.authorization_objects import (
    BOOTSTRAP_SCHEMA_VERSION,
    BOOTSTRAP_TYPE,
    HIGH_WATER_UNSET,
)
from guardian_assurance.authorization_transcript import (
    AUTHORIZATION_SIGNATURE_ALGORITHM,
    AUTHORIZATION_TRANSCRIPT_VERSION,
)
from guardian_assurance.signed_authorization import (
    ED25519_SIGNATURE_BYTES,
    SIGNED_AUTHORIZATION_MAX_RAW_BYTES,
    SignedAuthorizationError,
    SignedAuthorizationErrorCode,
    encode_authorization_signature_base64url,
    parse_and_validate_signed_authorization,
)


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


def envelope(
    *,
    authority_id: str = "root.bootstrap-authority:01",
    key_id: str = "bootstrap-key-001",
    authorization_object: dict[str, object] | None = None,
    signature: bytes | None = None,
) -> dict[str, object]:
    return {
        "signature_version": AUTHORIZATION_TRANSCRIPT_VERSION,
        "signature_algorithm": AUTHORIZATION_SIGNATURE_ALGORITHM,
        "authority_id": authority_id,
        "key_id": key_id,
        "authorization_object": authorization_object or authorization(),
        "signature": encode_authorization_signature_base64url(
            signature or bytes(range(64))
        ),
    }


def encoded(value: object) -> bytes:
    return json.dumps(value, separators=(",", ":"), ensure_ascii=True).encode("utf-8")


class M15SignedAuthorizationTests(unittest.TestCase):
    def test_valid_signed_authorization_parses(self) -> None:
        result = parse_and_validate_signed_authorization(encoded(envelope()))
        self.assertEqual(result.authority_id, "root.bootstrap-authority:01")
        self.assertEqual(result.key_id, "bootstrap-key-001")
        self.assertEqual(len(result.signature_bytes), ED25519_SIGNATURE_BYTES)

    def test_exact_outer_schema_rejects_extra_member(self) -> None:
        value = envelope()
        value["producer_id"] = "must-not-exist-here"
        with self.assertRaises(SignedAuthorizationError):
            parse_and_validate_signed_authorization(encoded(value))

    def test_missing_member_rejected(self) -> None:
        value = envelope()
        value.pop("key_id")
        with self.assertRaises(SignedAuthorizationError):
            parse_and_validate_signed_authorization(encoded(value))

    def test_wrong_signature_version_rejected(self) -> None:
        value = envelope()
        value["signature_version"] = "guardian-f401:m15:authorization-transcript:v2"
        with self.assertRaises(SignedAuthorizationError) as caught:
            parse_and_validate_signed_authorization(encoded(value))
        self.assertEqual(caught.exception.code, SignedAuthorizationErrorCode.VERSION_UNSUPPORTED)

    def test_wrong_algorithm_rejected(self) -> None:
        value = envelope()
        value["signature_algorithm"] = "rsa"
        with self.assertRaises(SignedAuthorizationError) as caught:
            parse_and_validate_signed_authorization(encoded(value))
        self.assertEqual(caught.exception.code, SignedAuthorizationErrorCode.ALGORITHM_UNSUPPORTED)

    def test_authority_id_must_match_signed_object(self) -> None:
        value = envelope(authority_id="root.other-authority:01")
        with self.assertRaises(SignedAuthorizationError) as caught:
            parse_and_validate_signed_authorization(encoded(value))
        self.assertEqual(caught.exception.code, SignedAuthorizationErrorCode.AUTHORITY_ID_MISMATCH)

    def test_invalid_key_id_rejected(self) -> None:
        value = envelope(key_id="bad key")
        with self.assertRaises(SignedAuthorizationError) as caught:
            parse_and_validate_signed_authorization(encoded(value))
        self.assertEqual(caught.exception.code, SignedAuthorizationErrorCode.KEY_ID_INVALID)

    def test_signature_padding_rejected(self) -> None:
        value = envelope()
        value["signature"] = str(value["signature"]) + "="
        with self.assertRaises(SignedAuthorizationError) as caught:
            parse_and_validate_signed_authorization(encoded(value))
        self.assertEqual(caught.exception.code, SignedAuthorizationErrorCode.SIGNATURE_ENCODING_INVALID)

    def test_signature_encoder_rejects_wrong_raw_length(self) -> None:
        with self.assertRaises(SignedAuthorizationError) as caught:
            encode_authorization_signature_base64url(b"x" * 63)
        self.assertEqual(
            caught.exception.code,
            SignedAuthorizationErrorCode.SIGNATURE_ENCODING_INVALID,
        )
    def test_duplicate_top_level_member_rejected(self) -> None:
        raw = (
            b'{"signature_version":"guardian-f401:m15:authorization-transcript:v1",'
            b'"signature_version":"guardian-f401:m15:authorization-transcript:v1",'
            b'"signature_algorithm":"ed25519","authority_id":"root.bootstrap-authority:01",'
            b'"key_id":"bootstrap-key-001","authorization_object":{},'
            b'"signature":"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"}'
        )
        with self.assertRaises(SignedAuthorizationError) as caught:
            parse_and_validate_signed_authorization(raw)
        self.assertEqual(caught.exception.code, SignedAuthorizationErrorCode.DUPLICATE_KEY)

    def test_raw_limit_rejected_before_parse(self) -> None:
        raw = b" " * (SIGNED_AUTHORIZATION_MAX_RAW_BYTES + 1)
        with self.assertRaises(SignedAuthorizationError) as caught:
            parse_and_validate_signed_authorization(raw)
        self.assertEqual(caught.exception.code, SignedAuthorizationErrorCode.RAW_LIMIT)

    def test_invalid_utf8_rejected(self) -> None:
        with self.assertRaises(SignedAuthorizationError) as caught:
            parse_and_validate_signed_authorization(b"\xff")
        self.assertEqual(caught.exception.code, SignedAuthorizationErrorCode.INVALID_UTF8)

    def test_raw_authorization_object_bytes_are_preserved(self) -> None:
        auth = authorization()
        auth_raw = json.dumps(auth, separators=(",", ": "), ensure_ascii=True)
        signature = encode_authorization_signature_base64url(bytes(range(64)))
        raw = (
            '{"signature_version":"' + AUTHORIZATION_TRANSCRIPT_VERSION + '",'
            '"signature_algorithm":"ed25519",'
            '"authority_id":"root.bootstrap-authority:01",'
            '"key_id":"bootstrap-key-001",'
            '"authorization_object":' + auth_raw + ','
            '"signature":"' + signature + '"}'
        ).encode("utf-8")
        result = parse_and_validate_signed_authorization(raw)
        self.assertEqual(result.authorization_object_raw, auth_raw.encode("utf-8"))

    def test_parsed_result_does_not_claim_verified_or_authenticated(self) -> None:
        result = parse_and_validate_signed_authorization(encoded(envelope()))
        self.assertFalse(hasattr(result, "verified"))
        self.assertFalse(hasattr(result, "authenticated"))
        self.assertFalse(hasattr(result, "consumed"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
