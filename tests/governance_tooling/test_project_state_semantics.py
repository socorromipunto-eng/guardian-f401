from __future__ import annotations
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
VALIDATOR = REPO / "tools/validate_project_state.py"

def run_validator(repo):
    return subprocess.run(
        [sys.executable, str(VALIDATOR), "--repo", str(repo)],
        text=True, capture_output=True, shell=False
    )

def copy_fixture(dst):
    (dst / "governance").mkdir(parents=True, exist_ok=True)
    for name in (
        "project-state.json",
        "requirements-register.json",
        "capability-register.json",
        "claims-register.json",
        "evidence-register.json",
        "document-register.json",
    ):
        (dst / "governance" / name).write_bytes(
            (REPO / "governance" / name).read_bytes()
        )

class ProjectStateSemanticContractTests(unittest.TestCase):
    def test_register_hash_drift_is_detected_fail_closed(self):
        with tempfile.TemporaryDirectory() as td_raw:
            td = Path(td_raw)
            subprocess.run(["git","init"],cwd=td,check=True,capture_output=True,text=True)
            subprocess.run(["git","config","user.email","guardian@example.invalid"],cwd=td,check=True)
            subprocess.run(["git","config","user.name","Guardian Test"],cwd=td,check=True)
            copy_fixture(td)

            ps_path = td / "governance/project-state.json"
            ps = json.loads(ps_path.read_text(encoding="utf-8"))

            for item in ps["registers"]:
                p = td / item["path"]
                item["sha256"] = hashlib.sha256(p.read_bytes()).hexdigest().upper()

            (td / "seed.txt").write_text("seed\n", encoding="utf-8")
            subprocess.run(["git","add","."],cwd=td,check=True)
            subprocess.run(["git","commit","-m","seed"],cwd=td,check=True,capture_output=True,text=True)

            commit = subprocess.run(
                ["git","rev-parse","HEAD"],
                cwd=td,check=True,capture_output=True,text=True
            ).stdout.strip()

            tree = subprocess.run(
                ["git","rev-parse","HEAD^{tree}"],
                cwd=td,check=True,capture_output=True,text=True
            ).stdout.strip()

            ps["source_commit"] = commit
            ps["source_tree"] = tree
            ps_path.write_text(
                json.dumps(ps, indent=2) + "\n",
                encoding="utf-8"
            )

            req = td / "governance/requirements-register.json"
            req.write_bytes(req.read_bytes() + b"\n")

            cp = run_validator(td)
            self.assertNotEqual(cp.returncode, 0)
            self.assertIn(
                "REGISTER_HASH_MISMATCH:governance/requirements-register.json",
                cp.stdout,
            )

    def test_matching_historical_commit_tree_passes(self):
        with tempfile.TemporaryDirectory() as td_raw:
            td = Path(td_raw)
            subprocess.run(["git","init"],cwd=td,check=True,capture_output=True,text=True)
            subprocess.run(["git","config","user.email","guardian@example.invalid"],cwd=td,check=True)
            subprocess.run(["git","config","user.name","Guardian Test"],cwd=td,check=True)
            copy_fixture(td)

            ps_path = td / "governance/project-state.json"
            ps = json.loads(ps_path.read_text(encoding="utf-8"))
            for item in ps["registers"]:
                p = td / item["path"]
                item["sha256"] = hashlib.sha256(p.read_bytes()).hexdigest().upper()

            (td / "seed.txt").write_text("seed\\n", encoding="utf-8")
            subprocess.run(["git","add","."],cwd=td,check=True)
            subprocess.run(["git","commit","-m","seed"],cwd=td,check=True,capture_output=True,text=True)
            commit = subprocess.run(["git","rev-parse","HEAD"],cwd=td,check=True,capture_output=True,text=True).stdout.strip()
            tree = subprocess.run(["git","rev-parse","HEAD^{tree}"],cwd=td,check=True,capture_output=True,text=True).stdout.strip()

            ps["source_commit"] = commit
            ps["source_tree"] = tree
            ps_path.write_text(json.dumps(ps, indent=2) + "\n", encoding="utf-8")

            cp = run_validator(td)
            self.assertEqual(cp.returncode, 0, cp.stdout + cp.stderr)
            self.assertIn("SOURCE_COMMIT_SEMANTIC_BINDING=PASS", cp.stdout)
            self.assertIn("SOURCE_COMMIT_EQUALS_HEAD=NOT_REQUIRED", cp.stdout)

    def test_tree_mismatch_fails(self):
        with tempfile.TemporaryDirectory() as td_raw:
            td = Path(td_raw)
            subprocess.run(["git","init"],cwd=td,check=True,capture_output=True,text=True)
            subprocess.run(["git","config","user.email","guardian@example.invalid"],cwd=td,check=True)
            subprocess.run(["git","config","user.name","Guardian Test"],cwd=td,check=True)
            copy_fixture(td)

            ps_path = td / "governance/project-state.json"
            ps = json.loads(ps_path.read_text(encoding="utf-8"))
            for item in ps["registers"]:
                p = td / item["path"]
                item["sha256"] = hashlib.sha256(p.read_bytes()).hexdigest().upper()

            (td / "seed.txt").write_text("seed\\n", encoding="utf-8")
            subprocess.run(["git","add","."],cwd=td,check=True)
            subprocess.run(["git","commit","-m","seed"],cwd=td,check=True,capture_output=True,text=True)
            commit = subprocess.run(["git","rev-parse","HEAD"],cwd=td,check=True,capture_output=True,text=True).stdout.strip()

            ps["source_commit"] = commit
            ps["source_tree"] = "0" * 40
            ps_path.write_text(json.dumps(ps, indent=2) + "\n", encoding="utf-8")

            cp = run_validator(td)
            self.assertNotEqual(cp.returncode, 0)
            self.assertIn("SOURCE_TREE_MISMATCH", cp.stdout)

    def test_noncommit_object_fails(self):
        with tempfile.TemporaryDirectory() as td_raw:
            td = Path(td_raw)
            subprocess.run(["git","init"],cwd=td,check=True,capture_output=True,text=True)
            copy_fixture(td)
            ps_path = td / "governance/project-state.json"
            ps = json.loads(ps_path.read_text(encoding="utf-8"))
            for item in ps["registers"]:
                p = td / item["path"]
                item["sha256"] = hashlib.sha256(p.read_bytes()).hexdigest().upper()
            blob = subprocess.run(
                ["git","hash-object","-w","--stdin"],
                cwd=td,input="blob",check=True,capture_output=True,text=True
            ).stdout.strip()
            ps["source_commit"] = blob
            ps["source_tree"] = "0" * 40
            ps_path.write_text(json.dumps(ps, indent=2) + "\n", encoding="utf-8")
            cp = run_validator(td)
            self.assertNotEqual(cp.returncode, 0)
            self.assertIn("SOURCE_COMMIT_NOT_COMMIT", cp.stdout)

if __name__ == "__main__":
    unittest.main()
