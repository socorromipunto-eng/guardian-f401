from pathlib import Path
import subprocess,tempfile,unittest
from tools.governance_tooling.repository import GitRepository

def git(repo,*args): return subprocess.run(["git","-C",str(repo),*args],text=True,capture_output=True,shell=False,check=True).stdout.strip()

class RepositoryIndexBlobTests(unittest.TestCase):
    def test_index_blob_returns_exact_staged_bytes(self):
        with tempfile.TemporaryDirectory() as d:
            r=Path(d)/"repo"; r.mkdir(); git(r,"init"); git(r,"config","user.email","test@example.invalid"); git(r,"config","user.name","Guardian Test"); p=r/"x.bin"; expected=b"abc\x00def\n"; p.write_bytes(expected); git(r,"add","x.bin"); self.assertEqual(GitRepository(r).index_blob("x.bin"),expected)
