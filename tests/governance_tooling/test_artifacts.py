from pathlib import Path
import tempfile
import unittest

from tools.governance_tooling.artifacts import byte_count, sha256_file


class ArtifactTests(unittest.TestCase):
    def test_sha256_and_byte_count(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "x.bin"
            path.write_bytes(b"abc")
            self.assertEqual(byte_count(path), 3)
            self.assertEqual(
                sha256_file(path),
                "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD",
            )


if __name__ == "__main__":
    unittest.main()
