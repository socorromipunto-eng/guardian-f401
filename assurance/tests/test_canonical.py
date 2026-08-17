"""RFC 8785 determinism tests for validated assurance envelopes."""

import unittest

from guardian_assurance import canonicalize_envelope


class CanonicalTests(unittest.TestCase):
    def test_member_order_is_canonical(self) -> None:
        first = b'{"payload":{"result":"UNKNOWN","subject_id":"machine-01","claim":"state","schema_version":"m14.observation.v1","evidence_digest":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},"logical_time":1,"object_id":"fedcba9876543210fedcba9876543210","producer_epoch":"0123456789abcdef0123456789abcdef","producer_id":"plant-a.guardian-01","object_type":"observation","domain":"guardian-f401:m14:assurance:v1:observation"}'
        second = b'{"domain":"guardian-f401:m14:assurance:v1:observation","object_type":"observation","producer_id":"plant-a.guardian-01","producer_epoch":"0123456789abcdef0123456789abcdef","object_id":"fedcba9876543210fedcba9876543210","logical_time":1,"payload":{"claim":"state","evidence_digest":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa","result":"UNKNOWN","schema_version":"m14.observation.v1","subject_id":"machine-01"}}'
        self.assertEqual(canonicalize_envelope(first), canonicalize_envelope(second))

    def test_one_thousand_runs_are_byte_identical(self) -> None:
        raw = b'{"domain":"guardian-f401:m14:assurance:v1:witness","object_type":"witness","producer_id":"plant-b.guardian-01","producer_epoch":"0123456789abcdef0123456789abcdef","object_id":"fedcba9876543210fedcba9876543210","logical_time":9,"payload":{"schema_version":"m14.witness.v1","subject_id":"machine-01","result":"NOT_CONFIRMED","observed_object_ids":["0123456789abcdef0123456789abcdef"],"evidence_digest":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}}'
        expected = canonicalize_envelope(raw)
        self.assertTrue(all(canonicalize_envelope(raw) == expected for _ in range(1000)))

    def test_escape_equivalent_input_is_byte_identical(self) -> None:
        plain = b'{"domain":"guardian-f401:m14:assurance:v1:observation","object_type":"observation","producer_id":"plant-a.guardian-01","producer_epoch":"0123456789abcdef0123456789abcdef","object_id":"fedcba9876543210fedcba9876543210","logical_time":1,"payload":{"schema_version":"m14.observation.v1","subject_id":"machine-01","claim":"state","result":"UNKNOWN","evidence_digest":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}}'
        escaped = plain.replace(b'"state"', b'"st\\u0061te"')
        self.assertEqual(canonicalize_envelope(plain), canonicalize_envelope(escaped))


if __name__ == "__main__":
    unittest.main(verbosity=2)
