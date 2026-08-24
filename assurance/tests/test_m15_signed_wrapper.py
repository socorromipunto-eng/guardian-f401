"""M15 closed signed-message wrapper tests."""

from __future__ import annotations

import base64
import json
import unittest

from guardian_assurance.signed_wrapper import (
    ED25519_BASE64URL_CHARS,
    ED25519_SIGNATURE_BYTES,
    M14_ASSURANCE_MAX_RAW_BYTES,
    SIGNED_WRAPPER_MAX_RAW_BYTES,
    SignedWrapperError,
    SignedWrapperErrorCode,
    decode_signature_base64url,
    encode_signature_base64url,
    parse_and_validate_signed_wrapper,
)


def observation() -> dict[str, object]:
    return {
        "domain": "guardian-f401:m14:assurance:v1:observation",
        "object_type": "observation",
        "producer_id": "plant-a.guardian-01",
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


def raw_signature() -> bytes:
    return bytes(range(64))


def encoded_signature() -> str:
    return (
        base64.urlsafe_b64encode(raw_signature())
        .rstrip(b"=")
        .decode("ascii")
    )


def wrapper() -> dict[str, object]:
    return {
        "signature_version": "guardian-f401:m15:signed-assurance:v1",
        "signature_algorithm": "ed25519",
        "key_id": "node-a-key-001",
        "assurance_object": observation(),
        "signature": encoded_signature(),
    }


def encoded(value: object) -> bytes:
    return json.dumps(
        value,
        separators=(",", ":"),
        ensure_ascii=False,
    ).encode("utf-8")


def wrapper_with_raw_assurance(raw_assurance: bytes) -> bytes:
    prefix = (
        b'{"signature_version":"guardian-f401:m15:signed-assurance:v1",'
        b'"signature_algorithm":"ed25519",'
        b'"key_id":"node-a-key-001",'
        b'"assurance_object":'
    )

    suffix = (
        b',"signature":"'
        + encoded_signature().encode("ascii")
        + b'"}'
    )

    return prefix + raw_assurance + suffix


class M15SignedWrapperTests(unittest.TestCase):
    def assert_code(
        self,
        raw: bytes,
        code: SignedWrapperErrorCode,
    ) -> None:
        with self.assertRaises(SignedWrapperError) as caught:
            parse_and_validate_signed_wrapper(raw)

        self.assertEqual(caught.exception.code, code)

    def test_constants_are_frozen(self) -> None:
        self.assertEqual(SIGNED_WRAPPER_MAX_RAW_BYTES, 66_048)
        self.assertEqual(M14_ASSURANCE_MAX_RAW_BYTES, 65_536)
        self.assertEqual(ED25519_SIGNATURE_BYTES, 64)
        self.assertEqual(ED25519_BASE64URL_CHARS, 86)

    def test_valid_wrapper_is_structurally_accepted(self) -> None:
        result = parse_and_validate_signed_wrapper(encoded(wrapper()))

        self.assertEqual(
            result.signature_version,
            "guardian-f401:m15:signed-assurance:v1",
        )
        self.assertEqual(result.signature_algorithm, "ed25519")
        self.assertEqual(result.key_id, "node-a-key-001")
        self.assertEqual(result.signature_bytes, raw_signature())
        self.assertEqual(
            result.assurance_object["object_type"],
            "observation",
        )

    def test_signature_encoding_round_trip(self) -> None:
        wire = encode_signature_base64url(raw_signature())

        self.assertEqual(len(wire), 86)
        self.assertNotIn("=", wire)
        self.assertEqual(
            decode_signature_base64url(wire),
            raw_signature(),
        )

    def test_signature_padding_rejected(self) -> None:
        value = wrapper()
        value["signature"] = encoded_signature() + "="

        self.assert_code(
            encoded(value),
            SignedWrapperErrorCode.SIGNATURE_ENCODING_INVALID,
        )

    def test_signature_whitespace_rejected(self) -> None:
        value = wrapper()
        signature = encoded_signature()
        value["signature"] = signature[:20] + " " + signature[21:]

        self.assert_code(
            encoded(value),
            SignedWrapperErrorCode.SIGNATURE_ENCODING_INVALID,
        )

    def test_signature_classic_base64_character_rejected(self) -> None:
        value = wrapper()
        signature = encoded_signature()
        value["signature"] = "+" + signature[1:]

        self.assert_code(
            encoded(value),
            SignedWrapperErrorCode.SIGNATURE_ENCODING_INVALID,
        )

    def test_signature_short_encoding_rejected(self) -> None:
        value = wrapper()
        value["signature"] = encoded_signature()[:-1]

        self.assert_code(
            encoded(value),
            SignedWrapperErrorCode.SIGNATURE_ENCODING_INVALID,
        )

    def test_raw_signature_wrong_length_rejected(self) -> None:
        with self.assertRaises(SignedWrapperError):
            encode_signature_base64url(b"x" * 63)

        with self.assertRaises(SignedWrapperError):
            encode_signature_base64url(b"x" * 65)

    def test_unknown_wrapper_member_rejected(self) -> None:
        value = wrapper()
        value["extra"] = True

        self.assert_code(
            encoded(value),
            SignedWrapperErrorCode.SCHEMA,
        )

    def test_missing_wrapper_member_rejected(self) -> None:
        value = wrapper()
        del value["signature"]

        self.assert_code(
            encoded(value),
            SignedWrapperErrorCode.SCHEMA,
        )

    def test_duplicate_top_level_member_rejected(self) -> None:
        raw = encoded(wrapper())

        raw = raw.replace(
            b'{"signature_version":',
            (
                b'{"signature_version":"guardian-f401:m15:signed-assurance:v1",'
                b'"signature_version":'
            ),
            1,
        )

        self.assert_code(
            raw,
            SignedWrapperErrorCode.DUPLICATE_KEY,
        )

    def test_unsupported_signature_version_rejected(self) -> None:
        value = wrapper()
        value["signature_version"] = (
            "guardian-f401:m15:signed-assurance:v2"
        )

        self.assert_code(
            encoded(value),
            SignedWrapperErrorCode.TRANSCRIPT_VERSION_UNSUPPORTED,
        )

    def test_unsupported_algorithm_rejected(self) -> None:
        value = wrapper()
        value["signature_algorithm"] = "rsa"

        self.assert_code(
            encoded(value),
            SignedWrapperErrorCode.ALGORITHM_UNSUPPORTED,
        )

    def test_invalid_key_id_rejected(self) -> None:
        value = wrapper()
        value["key_id"] = "bad key"

        self.assert_code(
            encoded(value),
            SignedWrapperErrorCode.KEY_ID_INVALID,
        )

    def test_invalid_utf8_rejected(self) -> None:
        self.assert_code(
            b"\xff",
            SignedWrapperErrorCode.INVALID_UTF8,
        )

    def test_invalid_json_rejected(self) -> None:
        self.assert_code(
            b"{",
            SignedWrapperErrorCode.INVALID_JSON,
        )

    def test_invalid_m14_object_rejected(self) -> None:
        value = wrapper()

        assurance = value["assurance_object"]
        assert isinstance(assurance, dict)

        assurance["object_type"] = "unknown"

        self.assert_code(
            encoded(value),
            SignedWrapperErrorCode.ASSURANCE_OBJECT_INVALID,
        )

    def test_inner_duplicate_is_rejected_by_m14_boundary(self) -> None:
        inner = encoded(observation())

        inner = inner.replace(
            b'{"domain":',
            (
                b'{"domain":"guardian-f401:m14:assurance:v1:observation",'
                b'"domain":'
            ),
            1,
        )

        self.assert_code(
            wrapper_with_raw_assurance(inner),
            SignedWrapperErrorCode.ASSURANCE_OBJECT_INVALID,
        )

    def test_inner_raw_65536_reaches_m14_and_is_accepted(self) -> None:
        base = encoded(observation())

        self.assertLess(len(base), 65_536)
        self.assertTrue(base.endswith(b"}"))

        padding = b" " * (65_536 - len(base))

        padded = base[:-1] + padding + b"}"

        self.assertEqual(len(padded), 65_536)

        raw = wrapper_with_raw_assurance(padded)

        self.assertLessEqual(len(raw), 66_048)

        result = parse_and_validate_signed_wrapper(raw)

        self.assertEqual(
            result.assurance_object["object_type"],
            "observation",
        )

        self.assertEqual(
            len(result.assurance_object_raw),
            65_536,
        )

    def test_inner_raw_65537_rejected_by_m14_boundary(self) -> None:
        base = encoded(observation())

        self.assertLess(len(base), 65_537)
        self.assertTrue(base.endswith(b"}"))

        padding = b" " * (65_537 - len(base))

        padded = base[:-1] + padding + b"}"

        self.assertEqual(len(padded), 65_537)

        raw = wrapper_with_raw_assurance(padded)

        self.assertLessEqual(len(raw), 66_048)

        self.assert_code(
            raw,
            SignedWrapperErrorCode.ASSURANCE_OBJECT_INVALID,
        )

    def test_outer_raw_66047_passes_raw_gate(self) -> None:
        raw = encoded(wrapper())
        padded = raw + (b" " * (66_047 - len(raw)))

        result = parse_and_validate_signed_wrapper(padded)

        self.assertEqual(
            result.assurance_object["object_type"],
            "observation",
        )

    def test_outer_raw_66048_passes_raw_gate(self) -> None:
        raw = encoded(wrapper())
        padded = raw + (b" " * (66_048 - len(raw)))

        result = parse_and_validate_signed_wrapper(padded)

        self.assertEqual(
            result.assurance_object["object_type"],
            "observation",
        )

    def test_outer_raw_66049_rejected_before_decode(self) -> None:
        raw = b"\xff" * 66_049

        self.assert_code(
            raw,
            SignedWrapperErrorCode.RAW_LIMIT,
        )

    def test_wrapper_validation_does_not_claim_signature_validity(self) -> None:
        result = parse_and_validate_signed_wrapper(encoded(wrapper()))

        self.assertEqual(result.signature_bytes, raw_signature())

        self.assertFalse(
            hasattr(result, "authenticated")
        )

        self.assertFalse(
            hasattr(result, "signature_valid")
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)