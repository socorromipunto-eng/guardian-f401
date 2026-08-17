"""Explicit software-only resource limits for the first M14 slice."""

from dataclasses import dataclass


@dataclass(frozen=True)
class AssuranceLimits:
    max_raw_bytes: int = 65_536
    max_depth: int = 16
    max_nodes: int = 2_048
    max_object_members: int = 128
    max_array_items: int = 256
    max_string_utf8_bytes: int = 4_096
