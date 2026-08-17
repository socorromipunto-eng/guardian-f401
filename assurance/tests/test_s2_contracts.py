"""M14-S2 closed-schema, structural and numeric contract vectors."""

import copy
import json
import unittest

from guardian_assurance import AssuranceError, AssuranceLimits, ErrorCode, parse_and_validate_envelope
from guardian_assurance.validation import _validate_structure


def envelope(object_type="observation"):
    payloads = {
        "observation": {"schema_version": "m14.observation.v1", "subject_id": "machine-01", "claim": "state", "result": "UNKNOWN", "evidence_digest": "a" * 64},
        "decision": {"schema_version": "m14.decision.v1", "policy_id": "policy-01", "result": "HUMAN_REVIEW_REQUIRED", "reason_codes": ["reason-01"], "evidence_ids": ["1" * 32]},
        "witness": {"schema_version": "m14.witness.v1", "subject_id": "machine-01", "result": "NOT_CONFIRMED", "observed_object_ids": ["2" * 32], "evidence_digest": "b" * 64},
    }
    return {"domain": f"guardian-f401:m14:assurance:v1:{object_type}", "object_type": object_type, "producer_id": "plant-a.guardian-01", "producer_epoch": "0" * 32, "object_id": "f" * 32, "logical_time": 1, "payload": payloads[object_type]}


def encoded(value):
    return json.dumps(value, separators=(",", ":")).encode()


class S2ContractVectors(unittest.TestCase):
    def assert_rejected(self, value, code=ErrorCode.SCHEMA):
        with self.assertRaises(AssuranceError) as caught:
            parse_and_validate_envelope(encoded(value))
        self.assertEqual(caught.exception.code, code)


def accept_case(name, value):
    def test(self):
        self.assertEqual(parse_and_validate_envelope(encoded(value)), value)
    setattr(S2ContractVectors, f"test_{name}", test)


def reject_case(name, mutate, object_type="observation", code=ErrorCode.SCHEMA):
    def test(self):
        value = envelope(object_type)
        mutate(value)
        self.assert_rejected(value, code)
    setattr(S2ContractVectors, f"test_{name}", test)


accept_case("s2_001_observation_valid", envelope("observation"))
accept_case("s2_002_decision_valid", envelope("decision"))
accept_case("s2_003_witness_valid", envelope("witness"))

for index, field in enumerate(("schema_version", "subject_id", "claim", "result", "evidence_digest"), 4):
    reject_case(f"s2_{index:03d}_observation_missing_{field}", lambda value, field=field: value["payload"].pop(field))
reject_case("s2_009_observation_extra", lambda value: value["payload"].update(extra=True))
reject_case("s2_010_observation_version", lambda value: value["payload"].update(schema_version="m14.observation.v2"))
reject_case("s2_011_observation_subject", lambda value: value["payload"].update(subject_id="bad space"))
reject_case("s2_012_observation_claim", lambda value: value["payload"].update(claim=""))
reject_case("s2_013_observation_result", lambda value: value["payload"].update(result="CONFIRMED"))
reject_case("s2_014_observation_digest_upper", lambda value: value["payload"].update(evidence_digest="A" * 64))
reject_case("s2_015_observation_digest_length", lambda value: value["payload"].update(evidence_digest="a" * 63))

for index, field in enumerate(("schema_version", "policy_id", "result", "reason_codes", "evidence_ids"), 16):
    reject_case(f"s2_{index:03d}_decision_missing_{field}", lambda value, field=field: value["payload"].pop(field), "decision")
reject_case("s2_021_decision_extra", lambda value: value["payload"].update(extra=1), "decision")
reject_case("s2_022_decision_empty_reasons", lambda value: value["payload"].update(reason_codes=[]), "decision")
reject_case("s2_023_decision_many_reasons", lambda value: value["payload"].update(reason_codes=[f"r{i}" for i in range(17)]), "decision")
reject_case("s2_024_decision_duplicate_reason", lambda value: value["payload"].update(reason_codes=["r", "r"]), "decision")
reject_case("s2_025_decision_empty_evidence", lambda value: value["payload"].update(evidence_ids=[]), "decision")
reject_case("s2_026_decision_many_evidence", lambda value: value["payload"].update(evidence_ids=[f"{i:032x}" for i in range(33)]), "decision")
reject_case("s2_027_decision_duplicate_evidence", lambda value: value["payload"].update(evidence_ids=["1" * 32, "1" * 32]), "decision")
reject_case("s2_028_decision_bad_evidence", lambda value: value["payload"].update(evidence_ids=["Z" * 32]), "decision")
reject_case("s2_029_decision_bad_result", lambda value: value["payload"].update(result="PRESENT"), "decision")

for index, field in enumerate(("schema_version", "subject_id", "result", "observed_object_ids", "evidence_digest"), 30):
    reject_case(f"s2_{index:03d}_witness_missing_{field}", lambda value, field=field: value["payload"].pop(field), "witness")
reject_case("s2_035_witness_extra", lambda value: value["payload"].update(extra=1), "witness")
reject_case("s2_036_witness_empty_ids", lambda value: value["payload"].update(observed_object_ids=[]), "witness")
reject_case("s2_037_witness_duplicate_ids", lambda value: value["payload"].update(observed_object_ids=["2" * 32, "2" * 32]), "witness")
reject_case("s2_038_witness_bad_id", lambda value: value["payload"].update(observed_object_ids=["2" * 31]), "witness")
reject_case("s2_039_witness_bad_digest", lambda value: value["payload"].update(evidence_digest="g" * 64), "witness")

reject_case("s2_040_cross_schema", lambda value: value.update(payload=envelope("decision")["payload"]))
reject_case("s2_041_float", lambda value: value["payload"].update(extra_number=1.5))
reject_case("s2_042_negative_zero", lambda value: value["payload"].update(extra_number=-0.0))
reject_case("s2_043_unsafe_positive", lambda value: value.update(logical_time=9_007_199_254_740_992))
reject_case("s2_044_unsafe_negative", lambda value: value["payload"].update(extra_number=-9_007_199_254_740_992))
reject_case("s2_045_boolean_time", lambda value: value.update(logical_time=True))


class StructuralVectors(unittest.TestCase):
    def check(self, value, limits, code=None):
        if code is None:
            _validate_structure(value, limits)
        else:
            with self.assertRaises(AssuranceError) as caught:
                _validate_structure(value, limits)
            self.assertEqual(caught.exception.code, code)


def structural_case(number, value, limits, code=None):
    def test(self):
        self.check(value(), limits(), code)
    setattr(StructuralVectors, f"test_s2_{number:03d}_structural", test)


def nested(depth):
    value = 0
    for _ in range(depth - 1):
        value = [value]
    return value


structural_case(46, lambda: nested(15), lambda: AssuranceLimits(max_depth=16))
structural_case(47, lambda: nested(16), lambda: AssuranceLimits(max_depth=16))
structural_case(48, lambda: nested(17), lambda: AssuranceLimits(max_depth=16), ErrorCode.STRUCTURAL_LIMIT)
structural_case(49, lambda: [0] * 2047, lambda: AssuranceLimits(max_nodes=2048, max_array_items=2047))
structural_case(50, lambda: [0] * 2048, lambda: AssuranceLimits(max_nodes=2049, max_array_items=2048))
structural_case(51, lambda: [0] * 2049, lambda: AssuranceLimits(max_nodes=2048, max_array_items=2049), ErrorCode.STRUCTURAL_LIMIT)
structural_case(52, lambda: {str(i): 0 for i in range(127)}, lambda: AssuranceLimits(max_object_members=128))
structural_case(53, lambda: {str(i): 0 for i in range(128)}, lambda: AssuranceLimits(max_object_members=128))
structural_case(54, lambda: {str(i): 0 for i in range(129)}, lambda: AssuranceLimits(max_object_members=128), ErrorCode.STRUCTURAL_LIMIT)
structural_case(55, lambda: [0] * 255, lambda: AssuranceLimits(max_array_items=256))
structural_case(56, lambda: [0] * 256, lambda: AssuranceLimits(max_array_items=256))
structural_case(57, lambda: [0] * 257, lambda: AssuranceLimits(max_array_items=256), ErrorCode.STRUCTURAL_LIMIT)
structural_case(58, lambda: "a" * 4095, lambda: AssuranceLimits(max_string_utf8_bytes=4096))
structural_case(59, lambda: "a" * 4096, lambda: AssuranceLimits(max_string_utf8_bytes=4096))
structural_case(60, lambda: "a" * 4097, lambda: AssuranceLimits(max_string_utf8_bytes=4096), ErrorCode.STRUCTURAL_LIMIT)
structural_case(61, lambda: {"a" * 4095: 0}, lambda: AssuranceLimits(max_string_utf8_bytes=4096))
structural_case(62, lambda: {"a" * 4096: 0}, lambda: AssuranceLimits(max_string_utf8_bytes=4096))
structural_case(63, lambda: {"a" * 4097: 0}, lambda: AssuranceLimits(max_string_utf8_bytes=4096), ErrorCode.STRUCTURAL_LIMIT)
structural_case(64, lambda: 9_007_199_254_740_991, AssuranceLimits)
structural_case(65, lambda: -9_007_199_254_740_991, AssuranceLimits)
structural_case(66, lambda: 9_007_199_254_740_992, AssuranceLimits, ErrorCode.SCHEMA)


class CoverageCorrectionVectors(unittest.TestCase):
    def rejected(self, value, code=ErrorCode.SCHEMA):
        with self.assertRaises(AssuranceError) as caught:
            parse_and_validate_envelope(encoded(value))
        self.assertEqual(caught.exception.code, code)

    def test_s2_067_unknown_dispatch(self):
        value = envelope()
        value["object_type"] = "unknown"
        value["domain"] = "guardian-f401:m14:assurance:v1:unknown"
        self.rejected(value)

    def test_s2_068_observation_rejects_decision_payload(self):
        value = envelope("observation")
        value["payload"] = envelope("decision")["payload"]
        self.rejected(value)

    def test_s2_069_decision_rejects_witness_payload(self):
        value = envelope("decision")
        value["payload"] = envelope("witness")["payload"]
        self.rejected(value)

    def test_s2_070_witness_rejects_observation_payload(self):
        value = envelope("witness")
        value["payload"] = envelope("observation")["payload"]
        self.rejected(value)

    def padded(self, size):
        base = encoded(envelope())
        self.assertLess(len(base), size)
        return base + (b" " * (size - len(base)))

    def test_s2_071_raw_65535(self):
        self.assertEqual(parse_and_validate_envelope(self.padded(65_535))["object_type"], "observation")

    def test_s2_072_raw_65536(self):
        self.assertEqual(parse_and_validate_envelope(self.padded(65_536))["object_type"], "observation")

    def test_s2_073_raw_65537(self):
        with self.assertRaises(AssuranceError) as caught:
            parse_and_validate_envelope(self.padded(65_537))
        self.assertEqual(caught.exception.code, ErrorCode.RAW_LIMIT)

    def test_s2_074_exponent_is_rejected(self):
        raw = encoded(envelope()).replace(b'"logical_time":1', b'"logical_time":1e0')
        with self.assertRaises(AssuranceError) as caught:
            parse_and_validate_envelope(raw)
        self.assertEqual(caught.exception.code, ErrorCode.SCHEMA)

    def test_s2_075_fraction_is_rejected(self):
        raw = encoded(envelope()).replace(b'"logical_time":1', b'"logical_time":1.5')
        with self.assertRaises(AssuranceError) as caught:
            parse_and_validate_envelope(raw)
        self.assertEqual(caught.exception.code, ErrorCode.SCHEMA)


if __name__ == "__main__":
    unittest.main(verbosity=2)
