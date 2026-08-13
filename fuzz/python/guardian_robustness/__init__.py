"""Guardian M11 defensive robustness and fault-injection utilities."""

# Re-export deterministic robustness campaign API.
from .campaign import (
    RobustnessReport,
    run_all_campaigns,
    run_counter_fault_campaign,
    run_parser_campaign,
    run_security_tamper_campaign,
)

# Re-export deterministic mutation primitives.
from .mutations import (
    MutationCase,
    MutationKind,
    chunk_stream,
    mutate,
)

# Publish the supported public defensive test helpers.
__all__ = [
    "MutationCase",
    "RobustnessReport",
    "MutationKind",
    "chunk_stream",
    "mutate",
    "run_all_campaigns",
    "run_counter_fault_campaign",
    "run_parser_campaign",
    "run_security_tamper_campaign",
]
