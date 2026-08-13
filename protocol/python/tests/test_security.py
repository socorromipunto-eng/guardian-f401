"""Unit tests for Guardian M10 security codecs and HMAC domain separation."""

# Import unittest from the Python standard library.
import unittest

# Import the public M10 protocol API.
from guardian_protocol import (
    AuthBegin,
    AuthFinish,
    MessageType,
    SecurityRole,
    compute_client_proof,
    compute_server_proof,
    decode_auth_begin,
    decode_auth_finish,
    decode_secure_request,
    decode_secure_response,
    derive_session_key,
    encode_auth_begin,
    encode_auth_finish,
    encode_secure_request,
    encode_secure_response,
)


# Verify deterministic authentication and secure-envelope behavior.
class SecurityCodecTests(unittest.TestCase):
    """Exercise M10 proof, session-key and message-tag codecs."""

    # Define one deterministic 256-bit test PSK.
    PSK = bytes(range(32))

    # Define deterministic host nonce.
    CLIENT_NONCE = bytes(range(0x10, 0x20))

    # Define deterministic device nonce.
    DEVICE_NONCE = bytes(range(0x20, 0x30))

    # Define deterministic session identifier.
    SESSION_ID = 0xA1B2C3D4

    # Verify AUTH_BEGIN and AUTH_FINISH request codecs.
    def test_auth_request_round_trips(self) -> None:

        # Build one AUTH_BEGIN request.
        begin = AuthBegin(
            role=SecurityRole.OPERATOR,
            client_nonce=self.CLIENT_NONCE,
        )

        # Require complete AUTH_BEGIN preservation.
        self.assertEqual(
            decode_auth_begin(
                encode_auth_begin(begin)
            ),
            begin,
        )

        # Build one client proof.
        proof = compute_client_proof(
            self.PSK,
            SecurityRole.OPERATOR,
            self.SESSION_ID,
            self.CLIENT_NONCE,
            self.DEVICE_NONCE,
        )

        # Build AUTH_FINISH.
        finish = AuthFinish(
            role=SecurityRole.OPERATOR,
            session_id=self.SESSION_ID,
            client_proof=proof,
        )

        # Require complete AUTH_FINISH preservation.
        self.assertEqual(
            decode_auth_finish(
                encode_auth_finish(finish)
            ),
            finish,
        )

    # Verify domain separation produces distinct proof/key material.
    def test_domain_separation(self) -> None:

        # Calculate server proof.
        server = compute_server_proof(
            self.PSK,
            SecurityRole.OPERATOR,
            self.SESSION_ID,
            self.CLIENT_NONCE,
            self.DEVICE_NONCE,
        )

        # Calculate client proof.
        client = compute_client_proof(
            self.PSK,
            SecurityRole.OPERATOR,
            self.SESSION_ID,
            self.CLIENT_NONCE,
            self.DEVICE_NONCE,
        )

        # Derive session key.
        session_key = derive_session_key(
            self.PSK,
            SecurityRole.OPERATOR,
            self.SESSION_ID,
            self.CLIENT_NONCE,
            self.DEVICE_NONCE,
        )

        # Require different domains not to reuse identical authentication material.
        self.assertNotEqual(server, client)

        # Require the session key to be full SHA-256 width.
        self.assertEqual(
            len(session_key),
            32,
        )

        # Require the session-key prefix not to equal either proof.
        self.assertNotEqual(
            session_key[:16],
            server,
        )

        # Require the session-key prefix not to equal client proof.
        self.assertNotEqual(
            session_key[:16],
            client,
        )

    # Verify request and response envelopes authenticate every correlation field.
    def test_secure_request_and_response_round_trip(self) -> None:

        # Derive deterministic session key.
        key = derive_session_key(
            self.PSK,
            SecurityRole.OPERATOR,
            self.SESSION_ID,
            self.CLIENT_NONCE,
            self.DEVICE_NONCE,
        )

        # Encode one protected baseline command.
        request_payload = encode_secure_request(
            session_key=key,
            session_id=self.SESSION_ID,
            counter=1,
            outer_sequence=77,
            inner_command=0x13,
            inner_payload=b"\x01\x01\x00\x10",
        )

        # Decode and authenticate it.
        request = decode_secure_request(
            request_payload,
            key,
            77,
        )

        # Require exact counter and inner payload.
        self.assertEqual(request.counter, 1)
        self.assertEqual(request.inner_command, 0x13)
        self.assertEqual(
            request.inner_payload,
            b"\x01\x01\x00\x10",
        )

        # Encode one authenticated inner response.
        response_payload = encode_secure_response(
            session_key=key,
            session_id=self.SESSION_ID,
            counter=1,
            outer_sequence=77,
            inner_message_type=MessageType.RESPONSE,
            inner_command=0x13,
            inner_payload=b"\x01\x01\x00\x10",
        )

        # Decode and authenticate the response.
        response = decode_secure_response(
            response_payload,
            key,
            77,
        )

        # Require complete correlation.
        self.assertEqual(response.counter, 1)
        self.assertEqual(
            response.inner_message_type,
            MessageType.RESPONSE,
        )
        self.assertEqual(response.inner_command, 0x13)

    # Verify authenticated payload tampering fails closed.
    def test_secure_request_rejects_tampered_tag(self) -> None:

        # Derive deterministic session key.
        key = derive_session_key(
            self.PSK,
            SecurityRole.OPERATOR,
            self.SESSION_ID,
            self.CLIENT_NONCE,
            self.DEVICE_NONCE,
        )

        # Encode one valid secure request.
        encoded = bytearray(
            encode_secure_request(
                session_key=key,
                session_id=self.SESSION_ID,
                counter=1,
                outer_sequence=88,
                inner_command=0x15,
                inner_payload=b"\x01\x01",
            )
        )

        # Corrupt one transmitted tag byte.
        encoded[-1] ^= 0x01

        # Require authentication failure.
        with self.assertRaises(ValueError):

            # Attempt to verify the corrupted envelope.
            decode_secure_request(
                encoded,
                key,
                88,
            )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
