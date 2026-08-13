"""Unit and integration tests for Guardian M11 robustness campaigns."""

# Import unittest from the Python standard library.
import unittest

# Import deterministic M11 campaign entry points.
from guardian_robustness import (
    run_counter_fault_campaign,
    run_parser_campaign,
    run_security_tamper_campaign,
)


# Verify the defensive fuzz/fault campaigns themselves.
class RobustnessCampaignTests(unittest.TestCase):
    """Run bounded deterministic M11 campaigns during ordinary test discovery."""

    # Verify parser mutations always recover a later canonical frame.
    def test_parser_campaign_recovers(self) -> None:

        # Execute a compact deterministic parser campaign.
        cases, recoveries = run_parser_campaign(
            iterations=64,
            seed=0x50415253,
        )

        # Require every requested case to execute.
        self.assertEqual(
            cases,
            64,
        )

        # Require every case to recover.
        self.assertEqual(
            recoveries,
            cases,
        )

    # Verify authenticated-envelope tampering never mutates privileged state.
    def test_security_tamper_campaign_fails_closed(self) -> None:

        # Execute a compact deterministic secure-envelope mutation campaign.
        cases, rejections = run_security_tamper_campaign(
            iterations=32,
            seed=0x53454355,
        )

        # Require every requested case to execute.
        self.assertEqual(
            cases,
            32,
        )

        # Require every tampered request to be rejected without state change.
        self.assertEqual(
            rejections,
            cases,
        )

    # Verify exact replay and valid-MAC skipped counter are both rejected.
    def test_counter_fault_campaign(self) -> None:

        # Execute explicit anti-replay fault injection.
        replay_rejections, gap_rejections = run_counter_fault_campaign(
            seed=0x434F554E,
        )

        # Require exact replay rejection.
        self.assertEqual(
            replay_rejections,
            1,
        )

        # Require valid-MAC counter-gap rejection.
        self.assertEqual(
            gap_rejections,
            1,
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
