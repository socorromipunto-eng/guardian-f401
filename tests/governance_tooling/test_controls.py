import unittest

from tools.governance_tooling.controls import CONTROLS


class ControlCatalogueTests(unittest.TestCase):
    def test_control_ids_are_unique_and_external_alignment_is_unadjudicated(self) -> None:
        ids = [control.control_id for control in CONTROLS]
        self.assertEqual(len(ids), len(set(ids)))
        self.assertTrue(ids)
        for control in CONTROLS:
            self.assertEqual(control.external_alignment_status, "NOT_ADJUDICATED")


if __name__ == "__main__":
    unittest.main()
