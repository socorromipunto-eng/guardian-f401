"""Typed failure classes for governance tooling."""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum


class FailureClass(str, Enum):
    PRODUCT = "PRODUCT"
    HARNESS = "HARNESS"
    ENVIRONMENT = "ENVIRONMENT"
    AUTHORITY = "AUTHORITY"


@dataclass(slots=True)
class GovernanceToolError(RuntimeError):
    failure_class: FailureClass
    code: str
    message: str

    def __str__(self) -> str:
        return f"{self.failure_class.value}:{self.code}: {self.message}"
