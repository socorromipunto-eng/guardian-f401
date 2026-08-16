"""Stable error taxonomy for assurance input rejection."""

from enum import Enum


class ErrorCode(str, Enum):
    RAW_LIMIT = "RAW_LIMIT"
    INVALID_UTF8 = "INVALID_UTF8"
    INVALID_JSON = "INVALID_JSON"
    DUPLICATE_KEY = "DUPLICATE_KEY"
    STRUCTURAL_LIMIT = "STRUCTURAL_LIMIT"
    SCHEMA = "SCHEMA"
    DOMAIN = "DOMAIN"
    CANONICALIZATION = "CANONICALIZATION"


class AssuranceError(ValueError):
    """A deterministic, externally classifiable validation failure."""

    def __init__(self, code: ErrorCode, detail: str) -> None:
        super().__init__(f"{code.value}: {detail}")
        self.code = code
        self.detail = detail

