from pathlib import Path
import json
import subprocess
import sys
import tempfile
import unittest


class SemanticCliTests(unittest.TestCase):
    def test_semantic_validate_json_output(self):
        with tempfile.TemporaryDirectory() as directory:
            registry = Path(directory) / "registry.json"
            registry.write_text(json.dumps({
                "schema": "guardian.semantic.registry.v1",
                "records": [
                    {"id": "EV-1", "type": "evidence", "evidence_class": "software"},
                    {"id": "CLAIM-1", "type": "claim", "status": "SUPPORTED", "evidence": ["EV-1"]},
                ],
            }), encoding="utf-8")
            repo = Path(__file__).resolve().parents[2]
            completed = subprocess.run([
                sys.executable, "-m", "tools.governance_tooling",
                "--repo", str(repo), "--format", "json",
                "semantic-validate", "--registry", str(registry),
            ], cwd=repo, text=True, capture_output=True, shell=False)
            self.assertEqual(completed.returncode, 0)
            data = json.loads(completed.stdout)
            self.assertEqual(data["final"], "PASS")
            self.assertEqual(data["authority"]["ACTUATION"], "NOT_GRANTED")


if __name__ == "__main__":
    unittest.main()
