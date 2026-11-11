"""AUD-002 candidate: exact reviewed wheels, then offline hash-required installation.
Run only on a GitHub Actions Linux x86_64 CPython 3.12 job. Not a local installer.
"""
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import tempfile
import urllib.parse
import urllib.request

ARTIFACTS = [{'name': 'jsonschema',
  'version': '4.26.0',
  'filename': 'jsonschema-4.26.0-py3-none-any.whl',
  'url': 'https://files.pythonhosted.org/packages/69/90/f63fb5873511e014207a475e2bb4e8b2e570d655b00ac19a9a0ca0a385ee/jsonschema-4.26.0-py3-none-any.whl',
  'size': 90630,
  'sha256': 'D489F15263B8D200F8387E64B4C3A75F06629559FB73DEB8FDFB525F2DAB50CE',
  'metadata_url': 'https://pypi.org/pypi/jsonschema/4.26.0/json',
  'scope': 'universal'},
 {'name': 'attrs',
  'version': '25.4.0',
  'filename': 'attrs-25.4.0-py3-none-any.whl',
  'url': 'https://files.pythonhosted.org/packages/3a/2a/7cc015f5b9f5db42b7d48157e23356022889fc354a2813c15934b7cb5c0e/attrs-25.4.0-py3-none-any.whl',
  'size': 67615,
  'sha256': 'ADCF7E2A1FB3B36AC48D97835BB6D8ADE15B8DCCE26ABA8BF1D14847B57A3373',
  'metadata_url': 'https://pypi.org/pypi/attrs/25.4.0/json',
  'scope': 'universal'},
 {'name': 'jsonschema-specifications',
  'version': '2025.9.1',
  'filename': 'jsonschema_specifications-2025.9.1-py3-none-any.whl',
  'url': 'https://files.pythonhosted.org/packages/41/45/1a4ed80516f02155c51f51e8cedb3c1902296743db0bbc66608a0db2814f/jsonschema_specifications-2025.9.1-py3-none-any.whl',
  'size': 18437,
  'sha256': '98802FEE3A11EE76ECACA44429FDA8A41BFF98B00A0F2838151B113F210CC6FE',
  'metadata_url': 'https://pypi.org/pypi/jsonschema-specifications/2025.9.1/json',
  'scope': 'universal'},
 {'name': 'referencing',
  'version': '0.37.0',
  'filename': 'referencing-0.37.0-py3-none-any.whl',
  'url': 'https://files.pythonhosted.org/packages/2c/58/ca301544e1fa93ed4f80d724bf5b194f6e4b945841c5bfd555878eea9fcb/referencing-0.37.0-py3-none-any.whl',
  'size': 26766,
  'sha256': '381329A9F99628C9069361716891D34AD94AF76E461DCB0335825AECC7692231',
  'metadata_url': 'https://pypi.org/pypi/referencing/0.37.0/json',
  'scope': 'universal'},
 {'name': 'rpds-py',
  'version': '0.30.0',
  'filename': 'rpds_py-0.30.0-cp312-cp312-manylinux_2_17_x86_64.manylinux2014_x86_64.whl',
  'url': 'https://files.pythonhosted.org/packages/60/1b/6f8f29f3f995c7ffdde46a626ddccd7c63aefc0efae881dc13b6e5d5bb16/rpds_py-0.30.0-cp312-cp312-manylinux_2_17_x86_64.manylinux2014_x86_64.whl',
  'size': 394080,
  'sha256': '47F236970BCCB2233267D89173D3AD2703CD36A0E2A6E92D0560D333871A3D23',
  'metadata_url': 'https://pypi.org/pypi/rpds-py/0.30.0/json',
  'scope': 'linux_cp312'},
 {'name': 'typing-extensions',
  'version': '4.15.0',
  'filename': 'typing_extensions-4.15.0-py3-none-any.whl',
  'url': 'https://files.pythonhosted.org/packages/18/67/36e9267722cc04a6b9f15c7f3441c2363321a3ea07da7ae0c0707beb2a9c/typing_extensions-4.15.0-py3-none-any.whl',
  'size': 44614,
  'sha256': 'F0FA19C6845758AB08074A0CFA8B7AECB71C999CA73D62883BC25CC018C4E548',
  'metadata_url': 'https://pypi.org/pypi/typing-extensions/4.15.0/json',
  'scope': 'universal'},
 {'name': 'pip',
  'version': '25.2',
  'filename': 'pip-25.2-py3-none-any.whl',
  'url': 'https://files.pythonhosted.org/packages/b7/3f/945ef7ab14dc4f9d7f40288d2df998d1837ee0888ec3659c813487572faa/pip-25.2-py3-none-any.whl',
  'size': 1752557,
  'sha256': '6D67A2B4E7F14D8B31B8B52648866FA717F45A1EB70E83002F4331D07E953717',
  'metadata_url': 'https://pypi.org/pypi/pip/25.2/json',
  'scope': 'pip_tool'}]
LOCK_SHA256 = "67FE8ED47D02BD26C81319A22A28917704B0FFE755EF8A60FB4109B47348CAAF"


def require(condition, reason):
    if not condition:
        raise RuntimeError(reason)


def verify_lock(raw):
    """Accept the reviewed LF blob or its uniform CRLF checkout only."""
    canonical = raw.replace(b"\r\n", b"\n")
    require(b"\r" not in canonical, "LOCK_INVALID_LINE_ENDINGS")
    require(raw == canonical or raw == canonical.replace(b"\n", b"\r\n"),
            "LOCK_MIXED_LINE_ENDINGS")
    require(hashlib.sha256(canonical).hexdigest().upper() == LOCK_SHA256,
            "CANDIDATE_LOCK_CHANGED")
    return canonical


def check_url(url):
    parsed = urllib.parse.urlparse(url)
    require(parsed.scheme == "https" and parsed.hostname == "files.pythonhosted.org"
            and not parsed.username and not parsed.password and parsed.port in (None, 443),
            "DOWNLOAD_OUTSIDE_EXACT_HOST")


class ReviewedRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, fp, code, message, headers, newurl):
        check_url(newurl)
        return super().redirect_request(request, fp, code, message, headers, newurl)


def verify_wheel(raw, artifact):
    require(len(raw) == artifact["size"], "WHEEL_LENGTH_MISMATCH")
    require(hashlib.sha256(raw).hexdigest().upper() == artifact["sha256"],
            "WHEEL_SHA256_MISMATCH")


def main():
    require(os.environ.get("GITHUB_ACTIONS") == "true" and os.environ.get("RUNNER_OS") == "Linux",
            "GITHUB_LINUX_JOB_REQUIRED")
    require(sys.version_info[:2] == (3, 12) and platform.machine().lower() in ("x86_64", "amd64"),
            "CPYTHON312_X64_REQUIRED")
    root = Path(__file__).resolve().parents[1]
    lock = root / "governance/requirements.lock"
    canonical_lock = verify_lock(lock.read_bytes())
    temporary_root = Path(os.environ["RUNNER_TEMP"]).resolve()
    require(temporary_root.is_dir(), "RUNNER_TEMP_ABSENT")
    opener = urllib.request.build_opener(ReviewedRedirect())
    with tempfile.TemporaryDirectory(prefix="guardian-aud002-", dir=temporary_root) as folder:
        house = Path(folder)
        verified_lock = house / "requirements.lock"
        with verified_lock.open("xb") as output:
            output.write(canonical_lock)
        for artifact in ARTIFACTS:
            check_url(artifact["url"])
            with opener.open(artifact["url"], timeout=30) as response:
                check_url(response.geturl())
                raw = response.read(artifact["size"] + 1)
            verify_wheel(raw, artifact)
            with (house / artifact["filename"]).open("xb") as output:
                output.write(raw)
        pip_artifact = next(row for row in ARTIFACTS if row["scope"] == "pip_tool")
        pip_wheel = house / pip_artifact["filename"]
        entry = "import sys,runpy;sys.path.insert(0,sys.argv.pop(1));sys.argv[0]='pip';runpy.run_module('pip',run_name='__main__')"
        environment = {key: value for key, value in os.environ.items()
                       if not key.startswith(("PIP_", "PYTHON"))}
        environment["PIP_CONFIG_FILE"] = os.devnull
        command = [sys.executable, "-I", "-B", "-c", entry, str(pip_wheel),
                   "--isolated", "--disable-pip-version-check", "--no-cache-dir", "--no-input",
                   "install", "--no-index", "--find-links", str(house), "--only-binary=:all:",
                   "--require-hashes", "--ignore-installed", "--no-compile", "-r", str(verified_lock)]
        subprocess.run(command, cwd=root, env=environment, check=True, timeout=180)
    print("AUD002_HASH_LOCKED_DEPENDENCY_INSTALL=PASS")


if __name__ == "__main__":
    main()
