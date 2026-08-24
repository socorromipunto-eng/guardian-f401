"""Guardian M15 host/test Ed25519 provider tests."""

from __future__ import annotations

import unittest

from guardian_assurance.crypto_provider import (
    ED25519_ALGORITHM,
    ED25519_PRIVATE_KEY_BYTES,
    ED25519_PUBLIC_KEY_BYTES,
    ED25519_SIGNATURE_BYTES,
    CryptoResultCode,
    HostEd25519Provider,
    generate_test_keypair,
)


class M15CryptoProviderTests(unittest.TestCase):
    def setUp(self) -> None:
        self.provider = HostEd25519Provider()
        self.private_key, self.public_key = generate_test_keypair()
        self.transcript = (
            b"GUARDIAN-F401:M15:TEST:TRANSCRIPT"
        )

    def test_key_sizes_are_exact(self) -> None:
        self.assertEqual(
            len(self.private_key),
            ED25519_PRIVATE_KEY_BYTES,
        )
        self.assertEqual(
            len(self.public_key),
            ED25519_PUBLIC_KEY_BYTES,
        )

    def test_host_test_sign_and_verify(self) -> None:
        signing = self.provider.sign_for_test(
            algorithm=ED25519_ALGORITHM,
            private_key=self.private_key,
            transcript=self.transcript,
        )

        self.assertEqual(
            signing.code,
            CryptoResultCode.VERIFIED,
        )
        self.assertTrue(signing.signed)
        self.assertIsNotNone(signing.signature)
        self.assertEqual(
            len(signing.signature or b""),
            ED25519_SIGNATURE_BYTES,
        )

        verification = self.provider.verify(
            algorithm=ED25519_ALGORITHM,
            public_key=self.public_key,
            transcript=self.transcript,
            signature=signing.signature or b"",
        )

        self.assertEqual(
            verification.code,
            CryptoResultCode.VERIFIED,
        )
        self.assertTrue(verification.verified)

    def test_changed_transcript_is_signature_invalid(self) -> None:
        signing = self.provider.sign_for_test(
            algorithm=ED25519_ALGORITHM,
            private_key=self.private_key,
            transcript=self.transcript,
        )

        verification = self.provider.verify(
            algorithm=ED25519_ALGORITHM,
            public_key=self.public_key,
            transcript=self.transcript + b"x",
            signature=signing.signature or b"",
        )

        self.assertEqual(
            verification.code,
            CryptoResultCode.SIGNATURE_INVALID,
        )
        self.assertFalse(verification.verified)

    def test_changed_signature_is_signature_invalid(self) -> None:
        signing = self.provider.sign_for_test(
            algorithm=ED25519_ALGORITHM,
            private_key=self.private_key,
            transcript=self.transcript,
        )

        signature = bytearray(signing.signature or b"")
        signature[0] ^= 1

        verification = self.provider.verify(
            algorithm=ED25519_ALGORITHM,
            public_key=self.public_key,
            transcript=self.transcript,
            signature=bytes(signature),
        )

        self.assertEqual(
            verification.code,
            CryptoResultCode.SIGNATURE_INVALID,
        )

    def test_wrong_public_key_is_signature_invalid(self) -> None:
        signing = self.provider.sign_for_test(
            algorithm=ED25519_ALGORITHM,
            private_key=self.private_key,
            transcript=self.transcript,
        )

        _, other_public = generate_test_keypair()

        verification = self.provider.verify(
            algorithm=ED25519_ALGORITHM,
            public_key=other_public,
            transcript=self.transcript,
            signature=signing.signature or b"",
        )

        self.assertEqual(
            verification.code,
            CryptoResultCode.SIGNATURE_INVALID,
        )

    def test_short_signature_is_signature_invalid(self) -> None:
        verification = self.provider.verify(
            algorithm=ED25519_ALGORITHM,
            public_key=self.public_key,
            transcript=self.transcript,
            signature=b"x" * 63,
        )

        self.assertEqual(
            verification.code,
            CryptoResultCode.SIGNATURE_INVALID,
        )

    def test_unsupported_algorithm_fails_closed(self) -> None:
        verification = self.provider.verify(
            algorithm="rsa",
            public_key=self.public_key,
            transcript=self.transcript,
            signature=b"x" * 64,
        )

        self.assertEqual(
            verification.code,
            CryptoResultCode.ALGORITHM_UNSUPPORTED,
        )

        signing = self.provider.sign_for_test(
            algorithm="rsa",
            private_key=self.private_key,
            transcript=self.transcript,
        )

        self.assertEqual(
            signing.code,
            CryptoResultCode.ALGORITHM_UNSUPPORTED,
        )

    def test_public_key_wrong_length_is_provider_failure(self) -> None:
        verification = self.provider.verify(
            algorithm=ED25519_ALGORITHM,
            public_key=b"x" * 31,
            transcript=self.transcript,
            signature=b"x" * 64,
        )

        self.assertEqual(
            verification.code,
            CryptoResultCode.CRYPTO_PROVIDER_FAILURE,
        )

    def test_private_key_wrong_length_is_provider_failure(self) -> None:
        signing = self.provider.sign_for_test(
            algorithm=ED25519_ALGORITHM,
            private_key=b"x" * 31,
            transcript=self.transcript,
        )

        self.assertEqual(
            signing.code,
            CryptoResultCode.CRYPTO_PROVIDER_FAILURE,
        )

    def test_non_bytes_transcript_fails_closed(self) -> None:
        verification = self.provider.verify(
            algorithm=ED25519_ALGORITHM,
            public_key=self.public_key,
            transcript="not-bytes",  # type: ignore[arg-type]
            signature=b"x" * 64,
        )

        self.assertEqual(
            verification.code,
            CryptoResultCode.CRYPTO_PROVIDER_FAILURE,
        )

    def test_result_does_not_claim_authenticated(self) -> None:
        signing = self.provider.sign_for_test(
            algorithm=ED25519_ALGORITHM,
            private_key=self.private_key,
            transcript=self.transcript,
        )

        verification = self.provider.verify(
            algorithm=ED25519_ALGORITHM,
            public_key=self.public_key,
            transcript=self.transcript,
            signature=signing.signature or b"",
        )

        self.assertEqual(
            verification.code,
            CryptoResultCode.VERIFIED,
        )

        self.assertFalse(
            hasattr(verification, "authenticated")
        )

        self.assertFalse(
            hasattr(verification, "authorized")
        )

        self.assertFalse(
            hasattr(verification, "fresh")
        )

    def test_provider_metadata_is_present(self) -> None:
        result = self.provider.verify(
            algorithm="unsupported",
            public_key=self.public_key,
            transcript=self.transcript,
            signature=b"x" * 64,
        )

        self.assertEqual(
            result.provider_identifier,
            "python-cryptography-ed25519",
        )

        self.assertEqual(
            result.provider_version,
            "50.0.0",
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)