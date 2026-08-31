from pathlib import Path
import subprocess,tempfile,unittest
from tools.governance_tooling.candidate import FORBIDDEN_PROVEN_CLAIMS,evidence_boundary_is_coherent,run_candidate
from tools.governance_tooling.repository import GitRepository
from tools.governance_tooling.result import Result

def git(repo,*args):
    return subprocess.run(["git","-C",str(repo),*args],text=True,capture_output=True,shell=False,check=True).stdout.strip()

class CandidateTests(unittest.TestCase):
    def make_repo(self):
        holder=tempfile.TemporaryDirectory(); repo=Path(holder.name)/"repo"; repo.mkdir(); git(repo,"init"); git(repo,"config","user.email","test@example.invalid"); git(repo,"config","user.name","Guardian Test"); (repo/"base.txt").write_text("base\n",encoding="utf-8"); git(repo,"add","base.txt"); git(repo,"commit","-m","base"); git(repo,"switch","-c","feature/test"); return holder,repo
    def test_exact_untracked_candidate_passes(self):
        h,r=self.make_repo();
        try:(r/"candidate.txt").write_text("candidate\n",encoding="utf-8"); self.assertEqual(run_candidate(GitRepository(r),("candidate.txt",)).final,"PASS")
        finally:h.cleanup()
    def test_unexpected_candidate_fails(self):
        h,r=self.make_repo();
        try:(r/"candidate.txt").write_text("candidate\n",encoding="utf-8"); (r/"extra.txt").write_text("extra\n",encoding="utf-8"); self.assertEqual(run_candidate(GitRepository(r),("candidate.txt",)).final,"FAIL")
        finally:h.cleanup()
    def test_staged_index_worktree_mismatch_fails(self):
        h,r=self.make_repo();
        try:
            p=r/"candidate.txt"; p.write_text("approved\n",encoding="utf-8"); git(r,"add","candidate.txt"); p.write_text("changed\n",encoding="utf-8"); self.assertEqual(run_candidate(GitRepository(r),("candidate.txt",)).final,"FAIL")
        finally:h.cleanup()
    def test_forbidden_scope_fails(self):
        h,r=self.make_repo();
        try:(r/"candidate.txt").write_text("candidate\n",encoding="utf-8"); self.assertEqual(run_candidate(GitRepository(r),("candidate.txt",),("candidate.txt",)).final,"FAIL")
        finally:h.cleanup()
    def test_forbidden_claim_overlap_rejected(self):
        x=Result(command="x",repository="r"); c=sorted(FORBIDDEN_PROVEN_CLAIMS)[0]; x.proven.append(c); x.not_proven.append(c); self.assertFalse(evidence_boundary_is_coherent(x))
    def test_authority_not_granted(self):
        h,r=self.make_repo();
        try:(r/"candidate.txt").write_text("candidate\n",encoding="utf-8"); self.assertTrue(all(v=="NOT_GRANTED" for v in run_candidate(GitRepository(r),("candidate.txt",)).authority.values()))
        finally:h.cleanup()
