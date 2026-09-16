"""Guardian F401 Cppcheck runner contract tests."""

from __future__ import annotations

import importlib.util
import pathlib


ROOT = pathlib.Path(__file__).resolve().parents[2]
RUNNER = ROOT / "tools" / "run_cppcheck.py"


def load_runner():
    spec = importlib.util.spec_from_file_location(
        "guardian_run_cppcheck",
        RUNNER,
    )

    assert spec is not None
    assert spec.loader is not None

    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)

    return module


def test_runner_exists() -> None:
    assert RUNNER.is_file()


def test_cppcheck_version_is_pinned() -> None:
    module = load_runner()

    assert (
        module.EXPECTED_CPPCHECK_VERSION ==
        "Cppcheck 2.21.0"
    )


def test_target_defines_are_exact() -> None:
    module = load_runner()

    assert module.REQUIRED_DEFINES == (
        "STM32F401xE",
        "USE_HAL_DRIVER",
    )


def test_vendor_driver_tree_not_in_primary_roots() -> None:
    module = load_runner()

    assert all(
        "GuardianF401/Drivers" not in root
        for root in module.FIRST_PARTY_ROOTS
    )


def test_negative_fixture_contract() -> None:
    fixture = """
    int guardian_negative_fixture(void)
    {
        int *p = 0;
        return *p;
    }
    """

    assert "int *p = 0;" in fixture
    assert "return *p;" in fixture

def test_armclang_compiler_model_contract() -> None:
    """The static-analysis compiler model is explicit and pinned."""
    source = RUNNER.read_text(encoding="utf-8")

    assert 'COMPILER_MODEL_NAME = "ARMCLANG_6_24"' in source
    assert 'COMPILER_MODEL_ARMCC_VERSION = "6240002"' in source
    assert 'f"__ARMCC_VERSION={COMPILER_MODEL_ARMCC_VERSION}"' in source
    assert 'f"-D{COMPILER_MODEL_DEFINE}"' in source
    assert '"compiler_model": COMPILER_MODEL_NAME' in source
    assert '"compiler_model_armcc_version": COMPILER_MODEL_ARMCC_VERSION' in source
    assert '"compiler_model_define": COMPILER_MODEL_DEFINE' in source
    assert '"suppression_count": 0' in source
    assert "--suppress" not in source
    assert "--inline-suppr" not in source
