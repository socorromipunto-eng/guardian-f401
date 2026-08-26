"""M15 deterministic purpose-separated transcript tests."""

from __future__ import annotations

import hashlib
import json
import unittest

from guardian_assurance.transcript import (
    COMMON_DOMAIN_PREFIX,
    PURPOSE_DOMAINS,
    TRANSCRIPT_VERSION,
    TranscriptError,
    build_signed_assurance_transcript,
    purpose_domain_for_object_type,
)


def envelope(object_type: str = "observation") -> bytes:
    payloads = {
        "observation": {
            "schema_version": "m14.observation.v1",
            "subject_id": "machine-01",
            "claim": "state",
            "result": "UNKNOWN",
            "evidence_digest": "a" * 64,
        },
        "decision": {
            "schema_version": "m14.decision.v1",
            "policy_id": "policy-01",
            "result": "HUMAN_REVIEW_REQUIRED",
            "reason_codes": ["reason-01"],
            "evidence_ids": ["1" * 32],
        },
        "witness": {
            "schema_version": "m14.witness.v1",
            "subject_id": "machine-01",
            "result": "NOT_CONFIRMED",
            "observed_object_ids": ["2" * 32],
            "evidence_digest": "b" * 64,
        },
    }

    value = {
        "domain": f"guardian-f401:m14:assurance:v1:{object_type}",
        "object_type": object_type,
        "producer_id": "plant-a.guardian-01",
        "producer_epoch": "0" * 32,
        "object_id": "f" * 32,
        "logical_time": 1,
        "payload": payloads[object_type],
    }

    return json.dumps(
        value,
        separators=(",", ":"),
        ensure_ascii=False,
    ).encode("utf-8")


class M15TranscriptTests(unittest.TestCase):
    def test_transcript_version_is_frozen(self) -> None:
        self.assertEqual(
            TRANSCRIPT_VERSION,
            "guardian-f401:m15:signed-assurance:v1",
        )

    def test_common_domain_prefix_is_frozen(self) -> None:
        self.assertEqual(
            COMMON_DOMAIN_PREFIX,
            b"GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1",
        )

    def test_purpose_domains_are_frozen(self) -> None:
        self.assertEqual(
            PURPOSE_DOMAINS["observation"],
            b"GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:OBSERVATION",
        )

        self.assertEqual(
            PURPOSE_DOMAINS["decision"],
            b"GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:DECISION",
        )

        self.assertEqual(
            PURPOSE_DOMAINS["witness"],
            b"GUARDIAN-F401:M15:SIGNED-ASSURANCE:V1:WITNESS",
        )

    def test_same_input_is_byte_reproducible(self) -> None:
        raw = envelope("observation")

        first = build_signed_assurance_transcript(
            raw,
            "node-a-key-001",
        )

        second = build_signed_assurance_transcript(
            raw,
            "node-a-key-001",
        )

        self.assertEqual(first, second)

        self.assertEqual(
            hashlib.sha256(first).digest(),
            hashlib.sha256(second).digest(),
        )

    def test_observation_uses_observation_domain(self) -> None:
        transcript = build_signed_assurance_transcript(
            envelope("observation"),
            "node-a-key-001",
        )

        self.assertTrue(
            transcript.startswith(PURPOSE_DOMAINS["observation"])
        )

    def test_decision_uses_decision_domain(self) -> None:
        transcript = build_signed_assurance_transcript(
            envelope("decision"),
            "node-a-key-001",
        )

        self.assertTrue(
            transcript.startswith(PURPOSE_DOMAINS["decision"])
        )

    def test_witness_uses_witness_domain(self) -> None:
        transcript = build_signed_assurance_transcript(
            envelope("witness"),
            "node-a-key-001",
        )

        self.assertTrue(
            transcript.startswith(PURPOSE_DOMAINS["witness"])
        )

    def test_all_purpose_domains_are_distinct(self) -> None:
        domains = tuple(PURPOSE_DOMAINS.values())

        self.assertEqual(len(domains), 3)
        self.assertEqual(len(set(domains)), 3)

    def test_cross_object_transcripts_are_distinct_with_same_key(self) -> None:
        key_id = "node-a-key-001"

        observation = build_signed_assurance_transcript(
            envelope("observation"),
            key_id,
        )

        decision = build_signed_assurance_transcript(
            envelope("decision"),
            key_id,
        )

        witness = build_signed_assurance_transcript(
            envelope("witness"),
            key_id,
        )

        self.assertNotEqual(observation, decision)
        self.assertNotEqual(observation, witness)
        self.assertNotEqual(decision, witness)

    def test_observation_does_not_use_decision_domain(self) -> None:
        transcript = build_signed_assurance_transcript(
            envelope("observation"),
            "node-a-key-001",
        )

        self.assertFalse(
            transcript.startswith(PURPOSE_DOMAINS["decision"])
        )

    def test_observation_does_not_use_witness_domain(self) -> None:
        transcript = build_signed_assurance_transcript(
            envelope("observation"),
            "node-a-key-001",
        )

        self.assertFalse(
            transcript.startswith(PURPOSE_DOMAINS["witness"])
        )

    def test_decision_does_not_use_observation_domain(self) -> None:
        transcript = build_signed_assurance_transcript(
            envelope("decision"),
            "node-a-key-001",
        )

        self.assertFalse(
            transcript.startswith(PURPOSE_DOMAINS["observation"])
        )

    def test_decision_does_not_use_witness_domain(self) -> None:
        transcript = build_signed_assurance_transcript(
            envelope("decision"),
            "node-a-key-001",
        )

        self.assertFalse(
            transcript.startswith(PURPOSE_DOMAINS["witness"])
        )

    def test_witness_does_not_use_observation_domain(self) -> None:
        transcript = build_signed_assurance_transcript(
            envelope("witness"),
            "node-a-key-001",
        )

        self.assertFalse(
            transcript.startswith(PURPOSE_DOMAINS["observation"])
        )

    def test_witness_does_not_use_decision_domain(self) -> None:
        transcript = build_signed_assurance_transcript(
            envelope("witness"),
            "node-a-key-001",
        )

        self.assertFalse(
            transcript.startswith(PURPOSE_DOMAINS["decision"])
        )

    def test_object_type_to_domain_mapping_is_closed(self) -> None:
        self.assertEqual(
            purpose_domain_for_object_type("observation"),
            PURPOSE_DOMAINS["observation"],
        )

        self.assertEqual(
            purpose_domain_for_object_type("decision"),
            PURPOSE_DOMAINS["decision"],
        )

        self.assertEqual(
            purpose_domain_for_object_type("witness"),
            PURPOSE_DOMAINS["witness"],
        )

        with self.assertRaises(TranscriptError):
            purpose_domain_for_object_type("unknown")

    def test_key_id_is_length_prefixed_big_endian(self) -> None:
        key_id = "node-a-key-001"

        transcript = build_signed_assurance_transcript(
            envelope("observation"),
            key_id,
        )

        offset = len(PURPOSE_DOMAINS["observation"])

        encoded_length = int.from_bytes(
            transcript[offset : offset + 2],
            byteorder="big",
        )

        self.assertEqual(
            encoded_length,
            len(key_id.encode("ascii")),
        )

        self.assertEqual(
            transcript[
                offset + 2 :
                offset + 2 + encoded_length
            ],
            key_id.encode("ascii"),
        )

    def test_different_key_id_changes_transcript(self) -> None:
        raw = envelope("observation")

        first = build_signed_assurance_transcript(
            raw,
            "node-a-key-001",
        )

        second = build_signed_assurance_transcript(
            raw,
            "node-a-key-002",
        )

        self.assertNotEqual(first, second)

    def test_producer_id_change_changes_transcript(self) -> None:
        raw = json.loads(envelope("observation").decode("utf-8"))

        raw["producer_id"] = "plant-a.guardian-01"

        first = build_signed_assurance_transcript(
            json.dumps(
                raw,
                separators=(",", ":"),
            ).encode("utf-8"),
            "node-a-key-001",
        )

        raw["producer_id"] = "plant-a.guardian-02"

        second = build_signed_assurance_transcript(
            json.dumps(
                raw,
                separators=(",", ":"),
            ).encode("utf-8"),
            "node-a-key-001",
        )

        self.assertNotEqual(first, second)

    def test_invalid_key_ids_are_rejected(self) -> None:
        invalid = (
            "",
            "contains space",
            "slash/not/allowed",
            "x" * 129,
            "key-é",
            "key=padding",
        )

        for key_id in invalid:
            with self.subTest(key_id=key_id):
                with self.assertRaises(TranscriptError):
                    build_signed_assurance_transcript(
                        envelope("observation"),
                        key_id,
                    )

    def test_non_bytes_input_is_rejected(self) -> None:
        with self.assertRaises(TranscriptError):
            build_signed_assurance_transcript(
                "{}",  # type: ignore[arg-type]
                "node-a-key-001",
            )

    def test_invalid_m14_object_never_reaches_transcript(self) -> None:
        with self.assertRaises(Exception):
            build_signed_assurance_transcript(
                b'{"producer_id":"plant-a.guardian-01"}',
                "node-a-key-001",
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)