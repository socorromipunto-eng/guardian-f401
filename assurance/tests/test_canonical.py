"""RFC 8785 determinism tests for validated assurance envelopes."""

import unittest

from guardian_assurance import canonicalize_envelope


class CanonicalTests(unittest.TestCase):
    def test_member_order_is_canonical(self) -> None:
        first = b'{"payload":{"z":2,"a":1},"logical_time":1,"object_id":"fedcba9876543210fedcba9876543210","producer_epoch":"0123456789abcdef0123456789abcdef","producer_id":"plant-a.guardian-01","object_type":"observation","domain":"guardian-f401:m14:assurance:v1:observation"}'
        second = b'{"domain":"guardian-f401:m14:assurance:v1:observation","object_type":"observation","producer_id":"plant-a.guardian-01","producer_epoch":"0123456789abcdef0123456789abcdef","object_id":"fedcba9876543210fedcba9876543210","logical_time":1,"payload":{"a":1,"z":2}}'
        self.assertEqual(canonicalize_envelope(first), canonicalize_envelope(second))

    def test_one_thousand_runs_are_byte_identical(self) -> None:
        raw = b'{"domain":"guardian-f401:m14:assurance:v1:witness","object_type":"witness","producer_id":"plant-b.guardian-01","producer_epoch":"0123456789abcdef0123456789abcdef","object_id":"fedcba9876543210fedcba9876543210","logical_time":9,"payload":{"result":"NOT_CONFIRMED"}}'
        expected = canonicalize_envelope(raw)
        self.assertTrue(all(canonicalize_envelope(raw) == expected for _ in range(1000)))


if __name__ == "__main__":
    unittest.main(verbosity=2)

