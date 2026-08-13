"""Guardian M10 authenticated-session state for the software simulator."""

# Import secrets for cryptographically strong simulator nonces.
import secrets

# Import dataclass for compact pending and active internal state.
from dataclasses import dataclass

# Import shared protocol models and cryptographic helpers.
from guardian_protocol import (
    AuthChallenge,
    AuthenticatedSession,
    Command,
    SecureRequest,
    SecurityRole,
    SecurityStatus,
    compute_client_proof,
    compute_server_proof,
    decode_auth_begin,
    decode_auth_finish,
    decode_secure_request,
    derive_session_key,
    encode_auth_challenge,
    encode_authenticated_session,
    encode_secure_response,
)


# Define pending-handshake timeout matching portable firmware.
_PENDING_TIMEOUT_SECONDS = 30.0

# Define default session inactivity timeout matching portable firmware.
_SESSION_TIMEOUT_SECONDS = 300


# Represent authentication/tag failures distinctly from malformed payloads.
class SimulatorAuthenticationError(Exception):
    """Raised when M10 proof or message authentication fails."""


# Represent strict anti-replay rejection distinctly.
class SimulatorReplayError(Exception):
    """Raised when a secure request counter is not exactly the next value."""


# Represent role-based authorization rejection distinctly.
class SimulatorAuthorizationError(Exception):
    """Raised when an authenticated role cannot execute an inner command."""


# Store one pending challenge without disturbing the current active session.
@dataclass(slots=True)
class _Pending:
    """One M10 pending challenge transcript."""

    # Store requested role.
    role: SecurityRole

    # Store random session identifier.
    session_id: int

    # Store host nonce.
    client_nonce: bytes

    # Store device nonce.
    device_nonce: bytes

    # Store monotonic creation time.
    started_at: float


# Store one active authenticated session.
@dataclass(slots=True)
class _Active:
    """One M10 active authenticated session."""

    # Store authenticated role.
    role: SecurityRole

    # Store random session identifier.
    session_id: int

    # Store derived full session key.
    session_key: bytes

    # Store exact next accepted strict anti-replay counter.
    next_counter: int

    # Store monotonic last authenticated activity.
    last_activity: float


# Implement M10 simulator authentication without transport dependencies.
class SimulatorSecurity:
    """Mirror portable M10 challenge-response and secure-envelope policy."""

    # Create one configured or compatibility-mode security model.
    def __init__(
        self,
        psk: bytes,
        max_role: SecurityRole,
        enabled: bool,
    ) -> None:

        # Copy the exact simulator PSK.
        self._psk = bytes(psk)

        # Require the frozen PSK width.
        if len(self._psk) != 32:

            # Reject invalid simulator provisioning.
            raise ValueError(
                "M10 simulator PSK must contain exactly 32 bytes"
            )

        # Preserve authorization ceiling.
        self._max_role = SecurityRole(max_role)

        # Preserve whether privileged commands require secure wrapping.
        self.enabled = bool(enabled)

        # Start without a pending challenge.
        self._pending: _Pending | None = None

        # Start without an authenticated session.
        self._active: _Active | None = None

        # Start without successful authentications.
        self._auth_successes = 0

        # Start without failed proofs or tags.
        self._auth_failures = 0

        # Start without replay rejections.
        self._replay_rejections = 0

        # Start without authorization rejections.
        self._unauthorized_rejections = 0

    # Expire stale pending or active state.
    def _expire(
        self,
        now: float,
    ) -> None:
        """Apply pending and active inactivity timeouts."""

        # Erase stale pending challenges.
        if (
            self._pending is not None
            and now - self._pending.started_at
            >= _PENDING_TIMEOUT_SECONDS
        ):

            # Remove stale unauthenticated state.
            self._pending = None

        # Erase inactive authenticated sessions.
        if (
            self._active is not None
            and now - self._active.last_activity
            >= _SESSION_TIMEOUT_SECONDS
        ):

            # Remove the expired session key and counters.
            self._active = None

    # Process one AUTH_BEGIN request.
    def begin(
        self,
        payload: bytes,
        now: float,
    ) -> bytes:
        """Return one device nonce and server proof."""

        # Expire stale state first.
        self._expire(now)

        # Decode and validate fixed request semantics.
        try:

            # Decode shared request.
            request = decode_auth_begin(payload)
        except (ValueError, TypeError) as exc:

            # Count malformed authentication traffic.
            self._auth_failures = min(
                0xFFFFFFFF,
                self._auth_failures + 1,
            )

            # Preserve a clear caller-visible failure.
            raise ValueError(
                "invalid AUTH_BEGIN payload"
            ) from exc

        # Deny role escalation beyond this PSK's ceiling.
        if request.role > self._max_role:

            # Count authorization rejection.
            self._unauthorized_rejections = min(
                0xFFFFFFFF,
                self._unauthorized_rejections + 1,
            )

            # Reject unauthorized role request.
            raise SimulatorAuthorizationError(
                "requested role exceeds provisioned maximum"
            )

        # Generate one random non-zero 32-bit session identifier.
        session_id = int.from_bytes(
            secrets.token_bytes(4),
            "big",
        ) or 1

        # Generate one unpredictable device challenge nonce.
        device_nonce = secrets.token_bytes(16)

        # Calculate the device-authentication proof.
        server_proof = compute_server_proof(
            self._psk,
            request.role,
            session_id,
            request.client_nonce,
            device_nonce,
        )

        # Replace only pending challenge state.
        self._pending = _Pending(
            role=request.role,
            session_id=session_id,
            client_nonce=request.client_nonce,
            device_nonce=device_nonce,
            started_at=now,
        )

        # Encode the fixed challenge response.
        return encode_auth_challenge(
            AuthChallenge(
                role=request.role,
                session_id=session_id,
                device_nonce=device_nonce,
                server_proof=server_proof,
            )
        )

    # Process one AUTH_FINISH request.
    def finish(
        self,
        payload: bytes,
        now: float,
    ) -> bytes:
        """Verify client proof and activate a new session."""

        # Expire stale state first.
        self._expire(now)

        # Require one pending challenge.
        if self._pending is None:

            # Count invalid authentication sequence.
            self._auth_failures = min(
                0xFFFFFFFF,
                self._auth_failures + 1,
            )

            # Reject finish without begin.
            raise SimulatorAuthenticationError(
                "AUTH_FINISH has no pending challenge"
            )

        # Decode fixed request semantics.
        try:

            # Decode shared client proof.
            request = decode_auth_finish(payload)
        except (ValueError, TypeError) as exc:

            # Count malformed authentication traffic.
            self._auth_failures = min(
                0xFFFFFFFF,
                self._auth_failures + 1,
            )

            # Erase failed pending state.
            self._pending = None

            # Reject malformed finish.
            raise ValueError(
                "invalid AUTH_FINISH payload"
            ) from exc

        # Preserve current pending transcript locally.
        pending = self._pending

        # Require exact role and session binding.
        if (
            request.role != pending.role
            or request.session_id != pending.session_id
        ):

            # Count transcript substitution.
            self._auth_failures = min(
                0xFFFFFFFF,
                self._auth_failures + 1,
            )

            # Erase failed pending state.
            self._pending = None

            # Reject transcript mismatch.
            raise SimulatorAuthenticationError(
                "AUTH_FINISH transcript mismatch"
            )

        # Calculate expected client proof.
        expected = compute_client_proof(
            self._psk,
            pending.role,
            pending.session_id,
            pending.client_nonce,
            pending.device_nonce,
        )

        # Compare proof in constant time.
        if not secrets.compare_digest(
            expected,
            request.client_proof,
        ):

            # Count invalid client proof.
            self._auth_failures = min(
                0xFFFFFFFF,
                self._auth_failures + 1,
            )

            # Erase failed pending state.
            self._pending = None

            # Reject authentication.
            raise SimulatorAuthenticationError(
                "AUTH_FINISH client proof mismatch"
            )

        # Derive one unique per-session key.
        session_key = derive_session_key(
            self._psk,
            pending.role,
            pending.session_id,
            pending.client_nonce,
            pending.device_nonce,
        )

        # Replace the active session only after successful client proof.
        self._active = _Active(
            role=pending.role,
            session_id=pending.session_id,
            session_key=session_key,
            next_counter=1,
            last_activity=now,
        )

        # Clear completed pending state.
        self._pending = None

        # Count successful authentication.
        self._auth_successes = min(
            0xFFFFFFFF,
            self._auth_successes + 1,
        )

        # Return fixed session acknowledgement.
        return encode_authenticated_session(
            AuthenticatedSession(
                role=self._active.role,
                session_id=self._active.session_id,
                timeout_seconds=_SESSION_TIMEOUT_SECONDS,
            )
        )

    # Authenticate and anti-replay-check one SECURE_COMMAND.
    def unwrap(
        self,
        payload: bytes,
        outer_sequence: int,
        now: float,
    ) -> SecureRequest:
        """Return one verified inner request."""

        # Expire inactive sessions.
        self._expire(now)

        # Require one active session.
        if self._active is None:

            # Count unauthorized secure traffic.
            self._unauthorized_rejections = min(
                0xFFFFFFFF,
                self._unauthorized_rejections + 1,
            )

            # Reject traffic without session key.
            raise SimulatorAuthenticationError(
                "no active authenticated session"
            )

        # Decode and authenticate the complete request tag.
        try:

            # Verify the shared secure envelope.
            secure_request = decode_secure_request(
                payload,
                self._active.session_key,
                outer_sequence,
            )
        except ValueError as exc:

            # Count authenticity/malformed secure failure.
            self._auth_failures = min(
                0xFFFFFFFF,
                self._auth_failures + 1,
            )

            # Reject the secure envelope.
            raise SimulatorAuthenticationError(
                "secure request authentication failed"
            ) from exc

        # Require exact active session identity.
        if secure_request.session_id != self._active.session_id:

            # Count unauthorized session substitution.
            self._unauthorized_rejections = min(
                0xFFFFFFFF,
                self._unauthorized_rejections + 1,
            )

            # Reject wrong-session traffic.
            raise SimulatorAuthenticationError(
                "secure request session mismatch"
            )

        # Require exact next counter with no replay window.
        if secure_request.counter != self._active.next_counter:

            # Count replay/out-of-order rejection.
            self._replay_rejections = min(
                0xFFFFFFFF,
                self._replay_rejections + 1,
            )

            # Reject duplicate or skipped counters.
            raise SimulatorReplayError(
                "secure request counter rejected"
            )

        # Consume the verified counter before authorization/dispatch.
        self._active.next_counter += 1

        # Refresh authenticated activity.
        self._active.last_activity = now

        # Return the verified inner request.
        return secure_request

    # Require sufficient role for one privileged command.
    def authorize(
        self,
        command: int,
    ) -> None:
        """Raise when active role cannot execute one inner command."""

        # Require one active authenticated session.
        if self._active is None:

            # Reject absent session.
            raise SimulatorAuthenticationError(
                "no active authenticated session"
            )

        # Default unclassified secure commands to ADMIN.
        required_role = SecurityRole.ADMIN

        # Permit baseline mutation to OPERATOR.
        if command == int(Command.BASELINE_CONTROL):

            # Require OPERATOR or ADMIN.
            required_role = SecurityRole.OPERATOR
        elif command == int(Command.CONTROL_COMMAND):

            # Require OPERATOR or ADMIN.
            required_role = SecurityRole.OPERATOR

        # Enforce the role floor.
        if self._active.role < required_role:

            # Count authorization denial.
            self._unauthorized_rejections = min(
                0xFFFFFFFF,
                self._unauthorized_rejections + 1,
            )

            # Reject insufficient role.
            raise SimulatorAuthorizationError(
                "authenticated role is insufficient"
            )

    # Wrap one inner response with an authenticated response tag.
    def wrap(
        self,
        counter: int,
        outer_sequence: int,
        inner_message_type,
        inner_command: int,
        inner_payload: bytes,
    ) -> bytes:
        """Return one authenticated secure response payload."""

        # Require active session key.
        if self._active is None:

            # Reject impossible response state.
            raise SimulatorAuthenticationError(
                "active session disappeared before response"
            )

        # Encode and authenticate the complete inner result.
        return encode_secure_response(
            self._active.session_key,
            self._active.session_id,
            counter,
            outer_sequence,
            inner_message_type,
            inner_command,
            inner_payload,
        )

    # Return public session diagnostics.
    def status(
        self,
        now: float,
    ) -> SecurityStatus:
        """Return one public M10 security snapshot."""

        # Apply inactivity expiry.
        self._expire(now)

        # Publish no-session defaults.
        active = self._active is not None

        # Calculate remaining session lifetime.
        remaining = 0

        # Publish active fields only when a session exists.
        if self._active is not None:

            # Calculate bounded remaining inactivity seconds.
            remaining = max(
                0,
                _SESSION_TIMEOUT_SECONDS
                - int(now - self._active.last_activity),
            )

        # Return public diagnostics without secret material.
        return SecurityStatus(
            configured=True,
            active=active,
            active_role=(
                self._active.role
                if self._active is not None
                else SecurityRole.NONE
            ),
            session_id=(
                self._active.session_id
                if self._active is not None
                else 0
            ),
            next_counter=(
                self._active.next_counter
                if self._active is not None
                else 0
            ),
            timeout_seconds=_SESSION_TIMEOUT_SECONDS,
            remaining_seconds=remaining,
            auth_successes=self._auth_successes,
            auth_failures=self._auth_failures,
            replay_rejections=self._replay_rejections,
            unauthorized_rejections=self._unauthorized_rejections,
        )
