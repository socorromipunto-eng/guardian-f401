"""Internal control catalogue for Guardian governance tooling.

External-framework mappings are intentionally NOT adjudicated in Batch 2.
This module must not be used to claim NASA, NIST, CIS, or other certification.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class Control:
    control_id: str
    name: str
    intent: str
    external_alignment_status: str = "NOT_ADJUDICATED"


CONTROLS = (
    Control("GTF-CTRL-001", "Read-only by default", "Tooling must not mutate repository state."),
    Control("GTF-CTRL-002", "Shell disabled", "Critical subprocess execution uses shell=False."),
    Control("GTF-CTRL-003", "Human mutation authority", "Technical PASS must not grant mutation authority."),
    Control("GTF-CTRL-004", "Evidence boundary", "PROVEN and NOT_PROVEN remain explicit."),
    Control("GTF-CTRL-005", "Failure classification", "Product, harness, environment, and authority failures remain distinct."),
    Control("GTF-CTRL-006", "Source-of-truth observation", "Repository identity is mechanically observed."),
    Control("GTF-CTRL-007", "Adversarial verification", "Negative and hostile conditions are tested."),
)
