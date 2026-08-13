"""Host-side Guardian M10 authenticated-session manager."""

# Import hmac for constant-time server-proof verification.
import hmac

# Import secrets for unpredictable client nonces.
import secrets

# Import shared Guardian security protocol models and codecs.
from guardian_protocol import (
    AuthBegin,
    AuthFinish,
    AuthenticatedSession,
    Command,
    Frame,
    MessageType,
    compute_client_proof,
    compute_server_proof,
    decode_auth_challenge,
    decode_authenticated_session,
    decode_secure_response,
    derive_session_key,
    encode_auth_begin,
    encode_auth_finish,
    encode_secure_request,
)

# Import immutable host security configuration.
from .config import SecurityClientConfig

# Import normalized host exceptions.
from .errors import (
    GuardianCtlError,
    ProtocolClientError,
    RemoteDeviceError,
)

# Import request sequence allocation.
from .sequence import SequenceManager

# Import synchronous transport contract.
from .transport import ExchangeTransport


# Maintain one authenticated M10 host session.
class GuardianSecuritySession:
    """Authenticate lazily and exchange protected Guardian commands."""

    # Create one session manager from shared transport and request sequence state.
    def __init__(
        self,
        transport: ExchangeTransport,
        sequence_manager: SequenceManager,
        config: SecurityClientConfig,
    ) -> None:

        # Preserve synchronous transport.
        self._transport = transport

        # Preserve the parent client's sequence allocator.
        self._sequence_manager = sequence_manager

        # Preserve immutable PSK and role configuration.
        self._config = config

        # Start without an authenticated session identifier.
        self._session_id = 0

        # Start without a derived session key.
        self._session_key: bytes | None = None

        # Start strict anti-replay counter at one after authentication.
        self._next_counter = 1

        # Start without session acknowledgement metadata.
        self._authenticated: AuthenticatedSession | None = None

    # Return whether a usable local authenticated session exists.
    @property
    def active(self) -> bool:
        """Return whether local session-key state exists."""

        # Require both metadata and key material.
        return (
            self._authenticated is not None
            and self._session_key is not None
        )

    # Erase local session metadata after ambiguous transport/authentication failures.
    def invalidate(self) -> None:
        """Forget local session identity, key and counter."""

        # Remove active session identifier.
        self._session_id = 0

        # Drop derived key bytes.
        self._session_key = None

        # Reset local strict counter.
        self._next_counter = 1

        # Remove session acknowledgement.
        self._authenticated = None

    # Perform a complete challenge-response handshake.
    def authenticate(self) -> AuthenticatedSession:
        """Establish a fresh authenticated M10 session."""

        # Generate an unpredictable host challenge nonce.
        client_nonce = secrets.token_bytes(16)

        # Allocate outer AUTH_BEGIN correlation sequence.
        begin_sequence = self._sequence_manager.next()

        # Build the fixed authentication challenge request.
        begin_request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.AUTH_BEGIN,
            sequence=begin_sequence,
            payload=encode_auth_begin(
                AuthBegin(
                    role=self._config.role,
                    client_nonce=client_nonce,
                )
            ),
        )

        # Execute AUTH_BEGIN.
        begin_response = self._transport.exchange(
            begin_request
        )

        # Decode the device challenge.
        try:

            # Parse the fixed challenge response.
            challenge = decode_auth_challenge(
                begin_response.payload
            )
        except ValueError as exc:

            # Forget any previous local session state.
            self.invalidate()

            # Raise a stable host protocol failure.
            raise ProtocolClientError(
                f"invalid AUTH_BEGIN response: {exc}"
            ) from exc

        # Require the device to preserve requested role.
        if challenge.role != self._config.role:

            # Reject unexpected authorization downgrade/upgrade.
            self.invalidate()

            # Raise a stable protocol failure.
            raise ProtocolClientError(
                "AUTH_BEGIN returned an unexpected authorization role"
            )

        # Calculate the expected device-authentication proof.
        expected_server_proof = compute_server_proof(
            self._config.psk,
            challenge.role,
            challenge.session_id,
            client_nonce,
            challenge.device_nonce,
        )

        # Verify the device proof in constant time before sending client proof.
        if not hmac.compare_digest(
            expected_server_proof,
            challenge.server_proof,
        ):

            # Forget any previous local session state.
            self.invalidate()

            # Reject a device that cannot prove possession of the PSK.
            raise ProtocolClientError(
                "AUTH_BEGIN server proof verification failed"
            )

        # Derive the same full per-session HMAC key.
        session_key = derive_session_key(
            self._config.psk,
            challenge.role,
            challenge.session_id,
            client_nonce,
            challenge.device_nonce,
        )

        # Calculate client proof only after server authentication succeeds.
        client_proof = compute_client_proof(
            self._config.psk,
            challenge.role,
            challenge.session_id,
            client_nonce,
            challenge.device_nonce,
        )

        # Allocate AUTH_FINISH correlation sequence.
        finish_sequence = self._sequence_manager.next()

        # Build the fixed client-proof request.
        finish_request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.AUTH_FINISH,
            sequence=finish_sequence,
            payload=encode_auth_finish(
                AuthFinish(
                    role=challenge.role,
                    session_id=challenge.session_id,
                    client_proof=client_proof,
                )
            ),
        )

        # Execute AUTH_FINISH.
        finish_response = self._transport.exchange(
            finish_request
        )

        # Decode authenticated-session acknowledgement.
        try:

            # Parse fixed session metadata.
            authenticated = decode_authenticated_session(
                finish_response.payload
            )
        except ValueError as exc:

            # Forget untrusted local key state.
            self.invalidate()

            # Raise stable protocol failure.
            raise ProtocolClientError(
                f"invalid AUTH_FINISH response: {exc}"
            ) from exc

        # Require session identity and role to match the challenge transcript.
        if (
            authenticated.session_id != challenge.session_id
            or authenticated.role != challenge.role
        ):

            # Reject contradictory session establishment.
            self.invalidate()

            # Raise stable protocol failure.
            raise ProtocolClientError(
                "AUTH_FINISH returned inconsistent session metadata"
            )

        # Publish verified session identifier.
        self._session_id = authenticated.session_id

        # Publish derived full session key.
        self._session_key = session_key

        # Start strict request counter at exactly one.
        self._next_counter = 1

        # Preserve immutable session acknowledgement.
        self._authenticated = authenticated

        # Return successful session metadata.
        return authenticated

    # Ensure one authenticated session exists.
    def _ensure_authenticated(self) -> None:
        """Authenticate lazily before the first protected command."""

        # Reuse existing local session when available.
        if self.active:

            # Return without another handshake.
            return

        # Establish a fresh authenticated session.
        self.authenticate()

    # Execute one authenticated privileged command.
    def exchange(
        self,
        inner_command: int,
        inner_payload: bytes = b"",
    ) -> Frame:
        """Return one authenticated inner RESPONSE or raise its inner ERROR."""

        # Authenticate lazily.
        self._ensure_authenticated()

        # Preserve key after active check for type narrowing.
        session_key = self._session_key

        # Reject impossible local session state defensively.
        if session_key is None:

            # Raise a stable host contract failure.
            raise ProtocolClientError(
                "authenticated session key is unavailable"
            )

        # Allocate one outer request sequence.
        outer_sequence = self._sequence_manager.next()

        # Preserve the exact current strict request counter.
        counter = self._next_counter

        # Build the authenticated SECURE_COMMAND payload.
        secure_payload = encode_secure_request(
            session_key=session_key,
            session_id=self._session_id,
            counter=counter,
            outer_sequence=outer_sequence,
            inner_command=int(inner_command),
            inner_payload=inner_payload,
        )

        # Build the ordinary Guardian outer request.
        request = Frame(
            message_type=MessageType.REQUEST,
            command=Command.SECURE_COMMAND,
            sequence=outer_sequence,
            payload=secure_payload,
        )

        # Ambiguous transport or outer security errors invalidate local session state.
        try:

            # Execute one synchronous secure exchange.
            outer_response = self._transport.exchange(
                request
            )
        except GuardianCtlError:

            # Force a new handshake on the next protected operation.
            self.invalidate()

            # Re-raise the original normalized host/transport exception.
            raise

        # Decode and verify the authenticated inner response.
        try:

            # Verify response HMAC and parse inner semantics.
            secure_response = decode_secure_response(
                outer_response.payload,
                session_key,
                outer_sequence,
            )
        except ValueError as exc:

            # Forget local key state after authenticity ambiguity.
            self.invalidate()

            # Raise stable host protocol failure.
            raise ProtocolClientError(
                f"invalid secure response: {exc}"
            ) from exc

        # Require exact session identity.
        if secure_response.session_id != self._session_id:

            # Reject cross-session response substitution.
            self.invalidate()

            # Raise stable protocol failure.
            raise ProtocolClientError(
                "secure response session identifier mismatch"
            )

        # Require exact request counter echo.
        if secure_response.counter != counter:

            # Reject replayed or mismatched response.
            self.invalidate()

            # Raise stable protocol failure.
            raise ProtocolClientError(
                "secure response counter mismatch"
            )

        # Require exact inner command correlation.
        if secure_response.inner_command != int(inner_command):

            # Reject authenticated response to another command.
            self.invalidate()

            # Raise stable protocol failure.
            raise ProtocolClientError(
                "secure response inner command mismatch"
            )

        # Advance local counter only after a valid authenticated response.
        self._next_counter += 1

        # Convert authenticated inner ERROR into the existing host exception contract.
        if secure_response.inner_message_type == MessageType.ERROR:

            # Require the canonical one-byte inner error payload.
            if len(secure_response.inner_payload) != 1:

                # Reject malformed authenticated error semantics.
                raise ProtocolClientError(
                    "secure inner ERROR must contain exactly one error byte"
                )

            # Raise the same structured remote error used by ordinary commands.
            raise RemoteDeviceError(
                command=secure_response.inner_command,
                error_code=secure_response.inner_payload[0],
            )

        # Require successful inner response semantics.
        if secure_response.inner_message_type != MessageType.RESPONSE:

            # Reject impossible synchronous message class.
            raise ProtocolClientError(
                "secure inner result must be RESPONSE or ERROR"
            )

        # Return one ordinary inner response to the high-level client.
        return Frame(
            message_type=MessageType.RESPONSE,
            command=secure_response.inner_command,
            sequence=outer_sequence,
            payload=secure_response.inner_payload,
        )
