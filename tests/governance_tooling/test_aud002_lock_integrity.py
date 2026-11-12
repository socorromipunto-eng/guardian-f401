"""AUD-002 lock byte-contract regressions; no network or installation."""
import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location('aud002_installer', ROOT / 'tools/install_governance_dependencies.py')
INSTALLER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(INSTALLER)
LF = (ROOT / 'governance/requirements.lock').read_bytes().replace(b'\r\n', b'\n')
EXPECTED = '67FE8ED47D02BD26C81319A22A28917704B0FFE755EF8A60FB4109B47348CAAF'


class LockIntegrityTests(unittest.TestCase):
    def test_reference_blob(self):
        self.assertEqual(len(LF), 731)
        self.assertEqual(hashlib.sha256(LF).hexdigest().upper(), EXPECTED)
        self.assertEqual(INSTALLER.LOCK_SHA256, EXPECTED)

    def test_lf_and_crlf(self):
        for raw in (LF, LF.replace(b'\n', b'\r\n')):
            with self.subTest(length=len(raw)):
                self.assertEqual(INSTALLER.verify_lock(raw), LF)

    def test_every_single_byte_mutation_rejected(self):
        for index in range(len(LF)):
            bad = LF[:index] + bytes([LF[index] ^ 1]) + LF[index + 1:]
            with self.subTest(index=index), self.assertRaises(RuntimeError):
                INSTALLER.verify_lock(bad)

    def test_malformed_and_structural_changes_rejected(self):
        cases = [b'', LF[:-1], LF + b'\n', b'\xef\xbb\xbf' + LF,
                 LF.replace(b'\n', b'\r\n', 1), LF.replace(b'\n', b'\r'),
                 LF.replace(b'\n', b'\r\r\n'), LF + b' --extra-index-url=https://bad.invalid',
                 LF.replace(b'4.26.0', b'4.26.1'), LF.replace(b'    --hash', b'   --hash', 1)]
        for index, bad in enumerate(cases):
            with self.subTest(index=index), self.assertRaises(RuntimeError):
                INSTALLER.verify_lock(bad)

    def test_invalid_lock_stops_before_network_or_subprocess(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / 'governance').mkdir()
            (root / 'governance/requirements.lock').write_bytes(LF + b'X')
            with mock.patch.object(INSTALLER, '__file__', str(root / 'tools/installer.py')), \
                 mock.patch.dict(INSTALLER.os.environ, {'GITHUB_ACTIONS': 'true', 'RUNNER_OS': 'Linux', 'RUNNER_TEMP': folder}), \
                 mock.patch.object(INSTALLER.sys, 'version_info', (3, 12)), \
                 mock.patch.object(INSTALLER.platform, 'machine', return_value='x86_64'), \
                 mock.patch.object(INSTALLER.urllib.request, 'build_opener') as network, \
                 mock.patch.object(INSTALLER.subprocess, 'run') as process:
                with self.assertRaisesRegex(RuntimeError, 'CANDIDATE_LOCK_CHANGED'):
                    INSTALLER.main()
                network.assert_not_called()
                process.assert_not_called()

    def test_pipeline_uses_verified_snapshot_for_both_endings(self):
        for raw in (LF, LF.replace(b'\n', b'\r\n')):
            with self.subTest(length=len(raw)), tempfile.TemporaryDirectory() as folder:
                root = Path(folder)
                (root / 'governance').mkdir()
                source = root / 'governance/requirements.lock'
                source.write_bytes(raw)
                payload = b'synthetic wheel, never executed'
                artifact = {'url': 'https://files.pythonhosted.org/synthetic.whl', 'filename': 'synthetic.whl',
                            'size': len(payload), 'sha256': hashlib.sha256(payload).hexdigest().upper(), 'scope': 'pip_tool'}
                response = mock.MagicMock()
                response.__enter__.return_value = response
                response.geturl.return_value = artifact['url']
                def read_bytes(limit):
                    source.write_bytes(b'changed after verification')
                    return payload
                response.read.side_effect = read_bytes
                opener = mock.Mock()
                opener.open.return_value = response
                def check_command(command, **kwargs):
                    lock = Path(command[command.index('-r') + 1])
                    self.assertNotEqual(lock, source)
                    self.assertEqual(lock.read_bytes(), LF)
                    self.assertIn('--require-hashes', command)
                    self.assertIn('--no-index', command)
                    self.assertIn('--only-binary=:all:', command)
                    self.assertTrue(kwargs['check'])
                with mock.patch.object(INSTALLER, '__file__', str(root / 'tools/installer.py')), \
                     mock.patch.dict(INSTALLER.os.environ, {'GITHUB_ACTIONS': 'true', 'RUNNER_OS': 'Linux', 'RUNNER_TEMP': folder}), \
                     mock.patch.object(INSTALLER.sys, 'version_info', (3, 12)), \
                     mock.patch.object(INSTALLER.platform, 'machine', return_value='x86_64'), \
                     mock.patch.object(INSTALLER, 'ARTIFACTS', [artifact]), \
                     mock.patch.object(INSTALLER.urllib.request, 'build_opener', return_value=opener), \
                     mock.patch.object(INSTALLER.subprocess, 'run', side_effect=check_command) as process:
                    INSTALLER.main()
                    self.assertEqual(process.call_count, 1)

    def test_wheel_integrity_guards(self):
        raw = b'wheel'
        artifact = {'size': len(raw), 'sha256': hashlib.sha256(raw).hexdigest().upper()}
        INSTALLER.verify_wheel(raw, artifact)
        for bad in (raw + b'X', b'WHEEL'):
            with self.assertRaises(RuntimeError):
                INSTALLER.verify_wheel(bad, artifact)

    def test_download_host_guard(self):
        INSTALLER.check_url('https://files.pythonhosted.org/pkg.whl')
        for bad in ('http://files.pythonhosted.org/pkg.whl', 'https://other.invalid/pkg.whl',
                    'https://user@files.pythonhosted.org/pkg.whl', 'https://files.pythonhosted.org:444/pkg.whl'):
            with self.assertRaises(RuntimeError):
                INSTALLER.check_url(bad)


if __name__ == '__main__':
    unittest.main(verbosity=2)
