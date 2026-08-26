import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
VALIDATOR = ROOT / "tools" / "validate_authority_boundary.py"

MINIMAL_FILES = (
    "firmware/Control/Src/guardian_control.c",
    "firmware/Control/Inc/guardian_control.h",
    "firmware/Platform/Src/guardian_embedded_link.c",
    "firmware/App/Src/guardian_firmware_app.c",
    "assurance/policies/m15-authority-boundary.json",
    "tools/validate_authority_boundary.py",
)


class AuthorityBoundaryValidatorTest(unittest.TestCase):
    def run_validator(self, root: Path) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
                str(root / "tools" / "validate_authority_boundary.py"),
                "--root",
                str(root),
            ],
            text=True,
            capture_output=True,
            check=False,
        )

    def make_test_tree(self) -> tuple[tempfile.TemporaryDirectory, Path]:
        temporary = tempfile.TemporaryDirectory()
        root = Path(temporary.name)

        for relative_name in MINIMAL_FILES:
            source = ROOT / relative_name
            target = root / relative_name

            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target)

        return temporary, root

    def append_source(
        self,
        root: Path,
        relative_name: str,
        source_text: str,
    ) -> None:
        path = root / relative_name

        with path.open("a", encoding="utf-8", newline="\n") as handle:
            handle.write("\n")
            handle.write(source_text)
            handle.write("\n")

    def assert_rule_rejected(
        self,
        root: Path,
        rule_id: str,
    ) -> None:
        result = self.run_validator(root)

        self.assertEqual(
            result.returncode,
            1,
            msg=(
                f"expected violation exit code for {rule_id}\n"
                f"stdout:\n{result.stdout}\n"
                f"stderr:\n{result.stderr}"
            ),
        )

        combined = result.stdout + result.stderr

        self.assertIn(
            rule_id,
            combined,
            msg=(
                f"expected {rule_id} in validator output\n"
                f"{combined}"
            ),
        )

    def test_current_repository_baseline_passes(self) -> None:
        result = subprocess.run(
            [
                sys.executable,
                str(VALIDATOR),
                "--root",
                str(ROOT),
            ],
            text=True,
            capture_output=True,
            check=False,
        )

        self.assertEqual(
            result.returncode,
            0,
            msg=(
                f"controlled repository baseline failed\n"
                f"stdout:\n{result.stdout}\n"
                f"stderr:\n{result.stderr}"
            ),
        )

        self.assertIn(
            "PASS: G15-05 authority boundary",
            result.stdout,
        )

    def test_external_run_permit_mutation_is_rejected(self) -> None:
        temporary, root = self.make_test_tree()

        try:
            self.append_source(
                root,
                "firmware/Platform/Src/guardian_embedded_link.c",
                """
static void hostile_run_permit_bypass(
    guardian_embedded_link_t *link)
{
    link->control.status.run_permit = 1U;
}
""".strip(),
            )

            self.assert_rule_rejected(root, "AUTH-001")
        finally:
            temporary.cleanup()

    def test_external_output_invocation_is_rejected(self) -> None:
        temporary, root = self.make_test_tree()

        try:
            self.append_source(
                root,
                "firmware/Platform/Src/guardian_embedded_link.c",
                """
static void hostile_output_bypass(
    guardian_embedded_link_t *link)
{
    (void)link->control.output.apply(
        link->control.output.context,
        1U);
}
""".strip(),
            )

            self.assert_rule_rejected(root, "AUTH-002")
        finally:
            temporary.cleanup()

    def test_unauthorized_output_configuration_is_rejected(self) -> None:
        temporary, root = self.make_test_tree()

        try:
            self.append_source(
                root,
                "firmware/App/Src/guardian_firmware_app.c",
                """
static void hostile_direct_configuration(
    guardian_control_t *control,
    const guardian_control_output_t *output)
{
    (void)guardian_control_configure_output(control, output);
}
""".strip(),
            )

            self.assert_rule_rejected(root, "AUTH-003")
        finally:
            temporary.cleanup()

    def test_runtime_heap_introduction_is_rejected(self) -> None:
        temporary, root = self.make_test_tree()

        try:
            self.append_source(
                root,
                "firmware/Platform/Src/guardian_embedded_link.c",
                """
static void *hostile_heap_bypass(void)
{
    return malloc(16U);
}
""".strip(),
            )

            self.assert_rule_rejected(root, "AUTH-004")
        finally:
            temporary.cleanup()


    def test_multiline_run_permit_mutation_is_rejected(self) -> None:
        temporary, root = self.make_test_tree()

        try:
            self.append_source(
                root,
                "firmware/Platform/Src/guardian_embedded_link.c",
                """
static void hostile_multiline_run_permit(
    guardian_embedded_link_t *link)
{
    link->control.status.run_permit
        =
        1U;
}
""".strip(),
            )

            self.assert_rule_rejected(root, "AUTH-001")
        finally:
            temporary.cleanup()

    def test_callback_alias_invocation_is_rejected(self) -> None:
        temporary, root = self.make_test_tree()

        try:
            self.append_source(
                root,
                "firmware/Platform/Src/guardian_embedded_link.c",
                """
static void hostile_callback_alias(
    guardian_embedded_link_t *link)
{
    guardian_control_apply_fn fn =
        link->control.output.apply;

    (void)fn(
        link->control.output.context,
        1U);
}
""".strip(),
            )

            self.assert_rule_rejected(root, "AUTH-002")
        finally:
            temporary.cleanup()

    def test_multiline_heap_call_is_rejected(self) -> None:
        temporary, root = self.make_test_tree()

        try:
            self.append_source(
                root,
                "firmware/Platform/Src/guardian_embedded_link.c",
                """
static void *hostile_multiline_heap(void)
{
    return malloc
    (
        16U
    );
}
""".strip(),
            )

            self.assert_rule_rejected(root, "AUTH-004")
        finally:
            temporary.cleanup()

    def test_comments_and_strings_do_not_create_false_violations(
        self,
    ) -> None:
        temporary, root = self.make_test_tree()

        try:
            self.append_source(
                root,
                "firmware/Platform/Src/guardian_embedded_link.c",
                """
/*
link->control.status.run_permit = 1U;
link->control.output.apply(
    link->control.output.context,
    1U);
malloc(16U);
*/

static const char hostile_text[] =
    "guardian_control_configure_output(control, output)";
""".strip(),
            )

            result = self.run_validator(root)

            self.assertEqual(
                result.returncode,
                0,
                msg=(
                    "comments or string literals produced a false "
                    "authority violation\n"
                    f"stdout:\n{result.stdout}\n"
                    f"stderr:\n{result.stderr}"
                ),
            )

            self.assertIn(
                "PASS: G15-05 authority boundary",
                result.stdout,
            )
        finally:
            temporary.cleanup()


if __name__ == "__main__":
    unittest.main()