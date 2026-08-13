"""Integration tests for Guardian simulator M10 security enforcement."""

# Import hmac for constant-time proof verification.
import hmac

# Import unittest from the Python standard library.
import unittest

# Import shared protocol security models and codecs.
from guardian_protocol import (
    AuthBegin,
    AuthFinish,
    BaselineAction,
    BaselineControl,
    Command,
    ErrorCode,
    Frame,
    MessageType,
    SecurityRole,
    compute_client_proof,
    compute_server_proof,
    decode_auth_challenge,
    decode_authenticated_session,
    decode_secure_response,
    derive_session_key,
    encode_auth_begin,
    encode_auth_finish,
    encode_baseline_control,
    encode_secure_request,
)

# Import simulator configuration and device.
from guardian_sim import GuardianDevice

# Import the public simulator demo PSK.
from guardian_sim.config import (
    DEFAULT_SECURITY_PSK_HEX,
    SimulatorConfig,
)


# Verify secure-mode simulator policy without TCP packetization.
class GuardianSecureDeviceTests(unittest.TestCase):
    """Exercise authentication, authorization and replay protection."""

    # Create one secure-mode simulator before every test.
    def setUp(self) -> None:

        # Build explicit M10 secure-mode configuration.
        self.config = SimulatorConfig(
            security_enabled=True,
        )

        # Create fresh device state.
        self.device = GuardianDevice(
            self.config
        )

        # Preserve the intentionally public test/demo key.
        self.psk = bytes.fromhex(
            DEFAULT_SECURITY_PSK_HEX
        )

    # Authenticate one role and return session key/id.
    def _authenticate(
        self,
        role: SecurityRole,
    ) -> tuple[int, bytes]:

        # Use one deterministic test nonce.
        client_nonce = bytes(range(16))

        # Start authentication.
        begin_response = self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.AUTH_BEGIN,
                sequence=1,
                payload=encode_auth_begin(
                    AuthBegin(
                        role=role,
                        client_nonce=client_nonce,
                    )
                ),
            )
        )

        # Decode device challenge.
        challenge = decode_auth_challenge(
            begin_response.payload
        )

        # Verify device possession of the PSK.
        expected_server_proof = compute_server_proof(
            self.psk,
            role,
            challenge.session_id,
            client_nonce,
            challenge.device_nonce,
        )

        # Require exact constant-time proof equality.
        self.assertTrue(
            hmac.compare_digest(
                expected_server_proof,
                challenge.server_proof,
            )
        )

        # Calculate client proof.
        client_proof = compute_client_proof(
            self.psk,
            role,
            challenge.session_id,
            client_nonce,
            challenge.device_nonce,
        )

        # Finish authentication.
        finish_response = self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.AUTH_FINISH,
                sequence=2,
                payload=encode_auth_finish(
                    AuthFinish(
                        role=role,
                        session_id=challenge.session_id,
                        client_proof=client_proof,
                    )
                ),
            )
        )

        # Require successful session acknowledgement.
        session = decode_authenticated_session(
            finish_response.payload
        )

        # Require role preservation.
        self.assertEqual(
            session.role,
            role,
        )

        # Derive the same session key.
        key = derive_session_key(
            self.psk,
            role,
            challenge.session_id,
            client_nonce,
            challenge.device_nonce,
        )

        # Return session identity and key.
        return challenge.session_id, key

    # Verify direct privileged commands are rejected in secure mode.
    def test_direct_baseline_control_is_unauthorized(self) -> None:

        # Build one valid legacy direct baseline request.
        response = self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.BASELINE_CONTROL,
                sequence=10,
                payload=encode_baseline_control(
                    BaselineControl(
                        action=BaselineAction.START,
                        target_samples=16,
                    )
                ),
            )
        )

        # Require explicit plain outer authorization failure.
        self.assertEqual(
            response.message_type,
            MessageType.ERROR,
        )

        # Require the M1-reserved authorization error now activated by M10.
        self.assertEqual(
            response.payload,
            bytes((int(ErrorCode.UNAUTHORIZED),)),
        )

    # Verify OPERATOR can execute protected baseline control.
    def test_operator_secure_baseline_succeeds(self) -> None:

        # Authenticate OPERATOR.
        session_id, key = self._authenticate(
            SecurityRole.OPERATOR
        )

        # Encode protected baseline command.
        secure_payload = encode_secure_request(
            session_key=key,
            session_id=session_id,
            counter=1,
            outer_sequence=20,
            inner_command=int(Command.BASELINE_CONTROL),
            inner_payload=encode_baseline_control(
                BaselineControl(
                    action=BaselineAction.START,
                    target_samples=16,
                )
            ),
        )

        # Execute SECURE_COMMAND.
        response = self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.SECURE_COMMAND,
                sequence=20,
                payload=secure_payload,
            )
        )

        # Require successful outer response.
        self.assertEqual(
            response.message_type,
            MessageType.RESPONSE,
        )

        # Authenticate the inner result.
        inner = decode_secure_response(
            response.payload,
            key,
            20,
        )

        # Require successful protected baseline response.
        self.assertEqual(
            inner.inner_message_type,
            MessageType.RESPONSE,
        )

        # Require exact protected command identity.
        self.assertEqual(
            inner.inner_command,
            int(Command.BASELINE_CONTROL),
        )

    # Verify replaying an already consumed secure request is rejected.
    def test_replayed_secure_request_is_rejected(self) -> None:

        # Authenticate OPERATOR.
        session_id, key = self._authenticate(
            SecurityRole.OPERATOR
        )

        # Build one valid protected request at counter one.
        secure_payload = encode_secure_request(
            session_key=key,
            session_id=session_id,
            counter=1,
            outer_sequence=30,
            inner_command=int(Command.BASELINE_CONTROL),
            inner_payload=encode_baseline_control(
                BaselineControl(
                    action=BaselineAction.START,
                    target_samples=16,
                )
            ),
        )

        # Consume the valid request.
        self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.SECURE_COMMAND,
                sequence=30,
                payload=secure_payload,
            )
        )

        # Replay the exact same authenticated frame.
        replay = self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.SECURE_COMMAND,
                sequence=30,
                payload=secure_payload,
            )
        )

        # Require explicit replay error.
        self.assertEqual(
            replay.message_type,
            MessageType.ERROR,
        )

        # Require the reserved replay code now activated by M10.
        self.assertEqual(
            replay.payload,
            bytes((int(ErrorCode.REPLAY_DETECTED),)),
        )

    # Verify OBSERVER authenticates but cannot mutate baseline.
    def test_observer_is_authenticated_but_not_authorized(self) -> None:

        # Authenticate an observer session.
        session_id, key = self._authenticate(
            SecurityRole.OBSERVER
        )

        # Build protected baseline mutation.
        secure_payload = encode_secure_request(
            session_key=key,
            session_id=session_id,
            counter=1,
            outer_sequence=40,
            inner_command=int(Command.BASELINE_CONTROL),
            inner_payload=encode_baseline_control(
                BaselineControl(
                    action=BaselineAction.START,
                    target_samples=16,
                )
            ),
        )

        # Execute protected request.
        response = self.device.process_frame(
            Frame(
                message_type=MessageType.REQUEST,
                command=Command.SECURE_COMMAND,
                sequence=40,
                payload=secure_payload,
            )
        )

        # Authenticate the wrapped authorization result.
        inner = decode_secure_response(
            response.payload,
            key,
            40,
        )

        # Require authenticated inner ERROR semantics.
        self.assertEqual(
            inner.inner_message_type,
            MessageType.ERROR,
        )

        # Require explicit authorization failure.
        self.assertEqual(
            inner.inner_payload,
            bytes((int(ErrorCode.UNAUTHORIZED),)),
        )


# Execute tests when run directly.
if __name__ == "__main__":

    # Start unittest.
    unittest.main(verbosity=2)
