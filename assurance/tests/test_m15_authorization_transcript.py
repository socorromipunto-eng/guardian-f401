"""M15 deterministic authorization transcript tests."""

from __future__ import annotations

import json
import unittest

from guardian_assurance.authorization_objects import (
    BOOTSTRAP_SCHEMA_VERSION,
    BOOTSTRAP_TYPE,
    EPOCH_TRANSITION_SCHEMA_VERSION,
    EPOCH_TRANSITION_TYPE,
    HIGH_WATER_UNSET,
)
from guardian_assurance.authorization_transcript import (
    AUTHORIZATION_SIGNATURE_ALGORITHM,
    AUTHORIZATION_TRANSCRIPT_VERSION,
    BOOTSTRAP_AUTHORIZATION_PURPOSE,
    EPOCH_TRANSITION_AUTHORIZATION_PURPOSE,
    AuthorizationTranscriptError,
    build_authorization_transcript,
    canonicalize_authorization_object,
)


def encoded(value: object, *, sort_keys: bool = False) -> bytes:
    return json.dumps(
        value,
        separators=(",", ":"),
        ensure_ascii=True,
        sort_keys=sort_keys,
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
        "authorization_sequence": 2,
        "from_epoch": "11111111111111111111111111111111",
        "to_epoch": "22222222222222222222222222222222",
        "transition_sequence": 1,
    }


class M15AuthorizationTranscriptTests(unittest.TestCase):
    def test_bootstrap_uses_bootstrap_purpose(self) -> None:
        transcript = build_authorization_transcript(
            encoded(bootstrap()),
            key_id="bootstrap-key-001",
            authority_id="root.bootstrap-authority:01",
        )
        self.assertTrue(transcript.startswith(BOOTSTRAP_AUTHORIZATION_PURPOSE))

    def test_transition_uses_transition_purpose(self) -> None:
        transcript = build_authorization_transcript(
            encoded(transition()),
            key_id="epoch-key-001",
            authority_id="root.epoch-authority:01",
        )
        self.assertTrue(
            transcript.startswith(EPOCH_TRANSITION_AUTHORIZATION_PURPOSE)
        )

    def test_purpose_domains_are_distinct(self) -> None:
        self.assertNotEqual(
            BOOTSTRAP_AUTHORIZATION_PURPOSE,
            EPOCH_TRANSITION_AUTHORIZATION_PURPOSE,
        )

    def test_canonicalization_is_order_independent(self) -> None:
        value = bootstrap()
        first = encoded(value, sort_keys=False)
        second = encoded(dict(reversed(list(value.items()))), sort_keys=False)
        self.assertEqual(
            canonicalize_authorization_object(first),
            canonicalize_authorization_object(second),
        )

    def test_transcript_is_order_independent(self) -> None:
        value = bootstrap()
        first = encoded(value)
        second = encoded(dict(reversed(list(value.items()))))
        self.assertEqual(
            build_authorization_transcript(
                first,
                key_id="bootstrap-key-001",
                authority_id="root.bootstrap-authority:01",
            ),
            build_authorization_transcript(
                second,
                key_id="bootstrap-key-001",
                authority_id="root.bootstrap-authority:01",
            ),
        )

    def test_changed_authorization_content_changes_transcript(self) -> None:
        first = bootstrap()
        second = bootstrap()
        second["authorization_sequence"] = 2
        self.assertNotEqual(
            build_authorization_transcript(
                encoded(first),
                key_id="bootstrap-key-001",
                authority_id="root.bootstrap-authority:01",
            ),
            build_authorization_transcript(
                encoded(second),
                key_id="bootstrap-key-001",
                authority_id="root.bootstrap-authority:01",
            ),
        )

    def test_changed_key_id_changes_transcript(self) -> None:
        raw = encoded(bootstrap())
        first = build_authorization_transcript(
            raw,
            key_id="bootstrap-key-001",
            authority_id="root.bootstrap-authority:01",
        )
        second = build_authorization_transcript(
            raw,
            key_id="bootstrap-key-002",
            authority_id="root.bootstrap-authority:01",
        )
        self.assertNotEqual(first, second)

    def test_authority_id_must_match_signed_object(self) -> None:
        with self.assertRaises(AuthorizationTranscriptError):
            build_authorization_transcript(
                encoded(bootstrap()),
                key_id="bootstrap-key-001",
                authority_id="root.other-authority:01",
            )

    def test_wrong_transcript_version_rejected(self) -> None:
        with self.assertRaises(AuthorizationTranscriptError):
            build_authorization_transcript(
                encoded(bootstrap()),
                key_id="bootstrap-key-001",
                authority_id="root.bootstrap-authority:01",
                transcript_version="guardian-f401:m15:authorization-transcript:v2",
            )

    def test_wrong_algorithm_rejected(self) -> None:
        with self.assertRaises(AuthorizationTranscriptError):
            build_authorization_transcript(
                encoded(bootstrap()),
                key_id="bootstrap-key-001",
                authority_id="root.bootstrap-authority:01",
                signature_algorithm="rsa",
            )

    def test_declared_version_and_algorithm_are_frozen(self) -> None:
        self.assertEqual(
            AUTHORIZATION_TRANSCRIPT_VERSION,
            "guardian-f401:m15:authorization-transcript:v1",
        )
        self.assertEqual(AUTHORIZATION_SIGNATURE_ALGORITHM, "ed25519")

    def test_invalid_key_id_rejected(self) -> None:
        with self.assertRaises(AuthorizationTranscriptError):
            build_authorization_transcript(
                encoded(bootstrap()),
                key_id="bad key",
                authority_id="root.bootstrap-authority:01",
            )

    def test_invalid_authorization_object_rejected(self) -> None:
        with self.assertRaises(AuthorizationTranscriptError):
            build_authorization_transcript(
                b"{}",
                key_id="bootstrap-key-001",
                authority_id="root.bootstrap-authority:01",
            )

    def test_framing_uses_uint16_key_and_authority_lengths(self) -> None:
        raw = encoded(bootstrap())
        key_id = "bootstrap-key-001"
        authority_id = "root.bootstrap-authority:01"
        transcript = build_authorization_transcript(
            raw,
            key_id=key_id,
            authority_id=authority_id,
        )
        offset = len(BOOTSTRAP_AUTHORIZATION_PURPOSE)
        key_length = int.from_bytes(transcript[offset:offset + 2], "big")
        self.assertEqual(key_length, len(key_id.encode("ascii")))
        offset += 2 + key_length
        authority_length = int.from_bytes(transcript[offset:offset + 2], "big")
        self.assertEqual(authority_length, len(authority_id.encode("ascii")))

    def test_framing_uses_uint32_canonical_length(self) -> None:
        raw = encoded(bootstrap())
        key_id = "bootstrap-key-001"
        authority_id = "root.bootstrap-authority:01"
        canonical = canonicalize_authorization_object(raw)
        transcript = build_authorization_transcript(
            raw,
            key_id=key_id,
            authority_id=authority_id,
        )
        offset = len(BOOTSTRAP_AUTHORIZATION_PURPOSE)
        offset += 2 + len(key_id.encode("ascii"))
        offset += 2 + len(authority_id.encode("ascii"))
        canonical_length = int.from_bytes(
            transcript[offset:offset + 4],
            "big",
        )
        self.assertEqual(canonical_length, len(canonical))
        self.assertEqual(transcript[offset + 4:], canonical)

    def test_transcript_result_is_bytes_only(self) -> None:
        result = build_authorization_transcript(
            encoded(bootstrap()),
            key_id="bootstrap-key-001",
            authority_id="root.bootstrap-authority:01",
        )
        self.assertIsInstance(result, bytes)


if __name__ == "__main__":
    unittest.main(verbosity=2)
