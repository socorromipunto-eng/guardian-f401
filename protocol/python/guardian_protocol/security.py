"""Guardian M10 authenticated-session and secure-envelope codecs."""

# Import hashlib for SHA-256.
import hashlib

# Import hmac for HMAC-SHA-256 and constant-time comparisons.
import hmac

# Import struct for deterministic big-endian wire serialization.
import struct

# Import dataclass for immutable shared protocol models.
from dataclasses import dataclass

# Import IntEnum for wire-compatible authorization roles.
from enum import IntEnum

# Import the existing Guardian message type registry.
from .enums import MessageType


# Define the first authenticated-session schema revision.
SECURITY_SCHEMA_VERSION = 0x01

# Define the required provisioned PSK width.
SECURITY_PSK_SIZE = 32

# Define challenge nonce width.
SECURITY_NONCE_SIZE = 16

# Define transmitted truncated HMAC tag width.
SECURITY_TAG_SIZE = 16

# Define full session-key width.
SECURITY_SESSION_KEY_SIZE = 32

# Define the default authenticated-session inactivity timeout.
DEFAULT_SECURITY_SESSION_TIMEOUT_SECONDS = 300

# Define secure request and response maximum inner payload sizes.
MAX_SECURE_REQUEST_INNER_PAYLOAD = 224

# Define secure response maximum inner payload size.
MAX_SECURE_RESPONSE_INNER_PAYLOAD = 223

# Define fixed payload layouts.
_AUTH_BEGIN_REQUEST = struct.Struct(">BB16s")

# Define AUTH_BEGIN successful response layout.
_AUTH_BEGIN_RESPONSE = struct.Struct(">BBI16s16s")

# Define AUTH_FINISH request layout.
_AUTH_FINISH_REQUEST = struct.Struct(">BBI16s")

# Define AUTH_FINISH successful response layout.
_AUTH_FINISH_RESPONSE = struct.Struct(">BBIH")

# Define public security-status layout.
_SECURITY_STATUS = struct.Struct(">BBBBIQHHIIII")

# Define secure request fixed prefix.
_SECURE_REQUEST_PREFIX = struct.Struct(">BIQBH")

# Define secure response fixed prefix.
_SECURE_RESPONSE_PREFIX = struct.Struct(">BIQBBH")

# Define domain-separation labels shared exactly with portable C.
_SERVER_PROOF_LABEL = b"GF-M10-SERVER"

# Define client-proof domain separation.
_CLIENT_PROOF_LABEL = b"GF-M10-CLIENT"

# Define session-key derivation domain separation.
_SESSION_KEY_LABEL = b"GF-M10-SESSION"

# Define authenticated request domain separation.
_REQUEST_MAC_LABEL = b"GF-M10-REQUEST"

# Define authenticated response domain separation.
_RESPONSE_MAC_LABEL = b"GF-M10-RESPONSE"


# Define authenticated authorization roles.
class SecurityRole(IntEnum):
    """Guardian M10 authenticated authorization roles."""

    # Represent no authenticated role.
    NONE = 0

    # Permit authenticated monitoring-only operations.
    OBSERVER = 1

    # Permit baseline and supervisory-control operations.
    OPERATOR = 2

    # Reserve the highest role for future firmware-lifecycle operations.
    ADMIN = 3


# Store one AUTH_BEGIN request.
@dataclass(frozen=True, slots=True)
class AuthBegin:
    """Guardian M10 authentication challenge request."""

    # Store requested authorization role.
    role: SecurityRole

    # Store the unpredictable host challenge nonce.
    client_nonce: bytes


# Store one AUTH_BEGIN successful response.
@dataclass(frozen=True, slots=True)
class AuthChallenge:
    """Guardian M10 device challenge and server proof."""

    # Store the granted authorization role.
    role: SecurityRole

    # Store the random session identifier.
    session_id: int

    # Store the unpredictable device nonce.
    device_nonce: bytes

    # Store the truncated server authentication proof.
    server_proof: bytes


# Store one AUTH_FINISH request.
@dataclass(frozen=True, slots=True)
class AuthFinish:
    """Guardian M10 client proof."""

    # Store the requested authorization role.
    role: SecurityRole

    # Store the pending session identifier.
    session_id: int

    # Store the truncated client proof.
    client_proof: bytes


# Store one successful authenticated session acknowledgement.
@dataclass(frozen=True, slots=True)
class AuthenticatedSession:
    """Guardian M10 established session metadata."""

    # Store authenticated authorization role.
    role: SecurityRole

    # Store active session identifier.
    session_id: int

    # Store device-enforced inactivity timeout in seconds.
    timeout_seconds: int


# Store one public security diagnostics snapshot.
@dataclass(frozen=True, slots=True)
class SecurityStatus:
    """Guardian M10 public authentication/session diagnostics."""

    # Store whether security provisioning exists.
    configured: bool

    # Store whether one authenticated session is active.
    active: bool

    # Store active role or NONE.
    active_role: SecurityRole

    # Store active session identifier or zero.
    session_id: int

    # Store exact next secure request counter.
    next_counter: int

    # Store configured inactivity timeout.
    timeout_seconds: int

    # Store current remaining inactivity lifetime.
    remaining_seconds: int

    # Store successful authentication count.
    auth_successes: int

    # Store failed proof/tag count.
    auth_failures: int

    # Store strict anti-replay rejection count.
    replay_rejections: int

    # Store authorization rejection count.
    unauthorized_rejections: int


# Store one decoded authenticated secure request.
@dataclass(frozen=True, slots=True)
class SecureRequest:
    """Guardian M10 authenticated inner request."""

    # Store active session identifier.
    session_id: int

    # Store strict monotonic request counter.
    counter: int

    # Store authenticated inner command identifier.
    inner_command: int

    # Store authenticated inner command payload.
    inner_payload: bytes


# Store one decoded authenticated secure response.
@dataclass(frozen=True, slots=True)
class SecureResponse:
    """Guardian M10 authenticated inner response."""

    # Store active session identifier.
    session_id: int

    # Store echoed authenticated request counter.
    counter: int

    # Store authenticated inner response message type.
    inner_message_type: MessageType

    # Store authenticated inner command identifier.
    inner_command: int

    # Store authenticated inner response payload.
    inner_payload: bytes


# Require exactly one 256-bit PSK.
def validate_psk(psk: bytes) -> bytes:
    """Return immutable PSK bytes after exact-size validation."""

    # Convert arbitrary bytes-like input.
    key = bytes(psk)

    # Require exactly 32 high-entropy bytes.
    if len(key) != SECURITY_PSK_SIZE:

        # Reject ambiguous or weak-length provisioning.
        raise ValueError(
            f"security PSK must contain exactly {SECURITY_PSK_SIZE} bytes"
        )

    # Return immutable validated key.
    return key


# Require one exact challenge nonce.
def _validate_nonce(
    nonce: bytes,
    field_name: str,
) -> bytes:
    """Return immutable nonce bytes after exact-size validation."""

    # Convert arbitrary bytes-like input.
    value = bytes(nonce)

    # Require the frozen nonce width.
    if len(value) != SECURITY_NONCE_SIZE:

        # Reject malformed challenge data.
        raise ValueError(
            f"{field_name} must contain exactly {SECURITY_NONCE_SIZE} bytes"
        )

    # Return immutable validated nonce.
    return value


# Require one transmitted 128-bit tag/proof.
def _validate_tag(
    tag: bytes,
    field_name: str,
) -> bytes:
    """Return immutable tag bytes after exact-size validation."""

    # Convert arbitrary bytes-like input.
    value = bytes(tag)

    # Require the frozen truncated HMAC width.
    if len(value) != SECURITY_TAG_SIZE:

        # Reject malformed authenticity data.
        raise ValueError(
            f"{field_name} must contain exactly {SECURITY_TAG_SIZE} bytes"
        )

    # Return immutable validated tag.
    return value


# Build the canonical handshake transcript.
def _handshake_transcript(
    label: bytes,
    role: SecurityRole,
    session_id: int,
    client_nonce: bytes,
    device_nonce: bytes,
) -> bytes:
    """Return canonical M10 handshake transcript bytes."""

    # Normalize the role.
    role_value = SecurityRole(role)

    # Require a non-zero unsigned 32-bit session identifier.
    if not 1 <= session_id <= 0xFFFFFFFF:

        # Reject invalid session identity.
        raise ValueError(
            "session_id must be between 1 and 0xFFFFFFFF"
        )

    # Validate both challenge nonces.
    client = _validate_nonce(
        client_nonce,
        "client_nonce",
    )

    # Validate device nonce.
    device = _validate_nonce(
        device_nonce,
        "device_nonce",
    )

    # Return exact label + schema + role + session + nonces.
    return (
        label
        + bytes(
            (
                SECURITY_SCHEMA_VERSION,
                int(role_value),
            )
        )
        + session_id.to_bytes(4, "big")
        + client
        + device
    )


# Calculate the truncated server authentication proof.
def compute_server_proof(
    psk: bytes,
    role: SecurityRole,
    session_id: int,
    client_nonce: bytes,
    device_nonce: bytes,
) -> bytes:
    """Return the 128-bit M10 server proof."""

    # Validate the provisioned key.
    key = validate_psk(psk)

    # Build the canonical domain-separated transcript.
    transcript = _handshake_transcript(
        _SERVER_PROOF_LABEL,
        role,
        session_id,
        client_nonce,
        device_nonce,
    )

    # Return the first 128 bits of HMAC-SHA-256.
    return hmac.new(
        key,
        transcript,
        hashlib.sha256,
    ).digest()[:SECURITY_TAG_SIZE]


# Calculate the truncated client authentication proof.
def compute_client_proof(
    psk: bytes,
    role: SecurityRole,
    session_id: int,
    client_nonce: bytes,
    device_nonce: bytes,
) -> bytes:
    """Return the 128-bit M10 client proof."""

    # Validate the provisioned key.
    key = validate_psk(psk)

    # Build the canonical domain-separated transcript.
    transcript = _handshake_transcript(
        _CLIENT_PROOF_LABEL,
        role,
        session_id,
        client_nonce,
        device_nonce,
    )

    # Return the first 128 bits of HMAC-SHA-256.
    return hmac.new(
        key,
        transcript,
        hashlib.sha256,
    ).digest()[:SECURITY_TAG_SIZE]


# Derive one unique full per-session HMAC key.
def derive_session_key(
    psk: bytes,
    role: SecurityRole,
    session_id: int,
    client_nonce: bytes,
    device_nonce: bytes,
) -> bytes:
    """Return the full 256-bit M10 session key."""

    # Validate the provisioned key.
    key = validate_psk(psk)

    # Build the canonical domain-separated transcript.
    transcript = _handshake_transcript(
        _SESSION_KEY_LABEL,
        role,
        session_id,
        client_nonce,
        device_nonce,
    )

    # Return the full HMAC-SHA-256 result.
    return hmac.new(
        key,
        transcript,
        hashlib.sha256,
    ).digest()


# Encode one AUTH_BEGIN request.
def encode_auth_begin(
    request: AuthBegin,
) -> bytes:
    """Encode one fixed Guardian M10 AUTH_BEGIN request."""

    # Normalize and validate requested role.
    role = SecurityRole(request.role)

    # Require an actual authentication role.
    if role == SecurityRole.NONE:

        # Reject meaningless authentication.
        raise ValueError(
            "AUTH_BEGIN role cannot be NONE"
        )

    # Validate client nonce.
    client_nonce = _validate_nonce(
        request.client_nonce,
        "client_nonce",
    )

    # Pack exact fixed request bytes.
    return _AUTH_BEGIN_REQUEST.pack(
        SECURITY_SCHEMA_VERSION,
        int(role),
        client_nonce,
    )


# Decode one AUTH_BEGIN request.
def decode_auth_begin(
    payload: bytes,
) -> AuthBegin:
    """Decode one fixed Guardian M10 AUTH_BEGIN request."""

    # Convert bytes-like input.
    encoded = bytes(payload)

    # Require exact fixed payload size.
    if len(encoded) != _AUTH_BEGIN_REQUEST.size:

        # Reject truncated or trailing bytes.
        raise ValueError(
            (
                f"AUTH_BEGIN expected {_AUTH_BEGIN_REQUEST.size} bytes, "
                f"received {len(encoded)}"
            )
        )

    # Decode schema, role and nonce.
    schema, role_value, client_nonce = (
        _AUTH_BEGIN_REQUEST.unpack(encoded)
    )

    # Require schema revision one.
    if schema != SECURITY_SCHEMA_VERSION:

        # Reject unknown handshake semantics.
        raise ValueError(
            f"unsupported security schema version: {schema}"
        )

    # Normalize published role.
    role = SecurityRole(role_value)

    # Reject NONE.
    if role == SecurityRole.NONE:

        # Reject meaningless authentication.
        raise ValueError(
            "AUTH_BEGIN role cannot be NONE"
        )

    # Return immutable request model.
    return AuthBegin(
        role=role,
        client_nonce=client_nonce,
    )


# Encode one AUTH_BEGIN successful response.
def encode_auth_challenge(
    challenge: AuthChallenge,
) -> bytes:
    """Encode one fixed Guardian M10 AUTH_BEGIN response."""

    # Normalize role.
    role = SecurityRole(challenge.role)

    # Validate device nonce.
    device_nonce = _validate_nonce(
        challenge.device_nonce,
        "device_nonce",
    )

    # Validate server proof.
    server_proof = _validate_tag(
        challenge.server_proof,
        "server_proof",
    )

    # Pack exact fixed response.
    return _AUTH_BEGIN_RESPONSE.pack(
        SECURITY_SCHEMA_VERSION,
        int(role),
        challenge.session_id,
        device_nonce,
        server_proof,
    )


# Decode one AUTH_BEGIN successful response.
def decode_auth_challenge(
    payload: bytes,
) -> AuthChallenge:
    """Decode one fixed Guardian M10 AUTH_BEGIN response."""

    # Convert bytes-like input.
    encoded = bytes(payload)

    # Require exact fixed response size.
    if len(encoded) != _AUTH_BEGIN_RESPONSE.size:

        # Reject truncated or trailing bytes.
        raise ValueError(
            (
                f"AUTH_BEGIN response expected {_AUTH_BEGIN_RESPONSE.size} bytes, "
                f"received {len(encoded)}"
            )
        )

    # Decode the complete response.
    schema, role_value, session_id, device_nonce, server_proof = (
        _AUTH_BEGIN_RESPONSE.unpack(encoded)
    )

    # Require schema revision one.
    if schema != SECURITY_SCHEMA_VERSION:

        # Reject unknown semantics.
        raise ValueError(
            f"unsupported security schema version: {schema}"
        )

    # Return typed challenge.
    return AuthChallenge(
        role=SecurityRole(role_value),
        session_id=session_id,
        device_nonce=device_nonce,
        server_proof=server_proof,
    )


# Encode one AUTH_FINISH request.
def encode_auth_finish(
    request: AuthFinish,
) -> bytes:
    """Encode one fixed Guardian M10 AUTH_FINISH request."""

    # Normalize role.
    role = SecurityRole(request.role)

    # Validate client proof.
    proof = _validate_tag(
        request.client_proof,
        "client_proof",
    )

    # Pack exact fixed request.
    return _AUTH_FINISH_REQUEST.pack(
        SECURITY_SCHEMA_VERSION,
        int(role),
        request.session_id,
        proof,
    )


# Decode one AUTH_FINISH request.
def decode_auth_finish(
    payload: bytes,
) -> AuthFinish:
    """Decode one fixed Guardian M10 AUTH_FINISH request."""

    # Convert bytes-like input.
    encoded = bytes(payload)

    # Require exact fixed request size.
    if len(encoded) != _AUTH_FINISH_REQUEST.size:

        # Reject malformed request.
        raise ValueError(
            (
                f"AUTH_FINISH expected {_AUTH_FINISH_REQUEST.size} bytes, "
                f"received {len(encoded)}"
            )
        )

    # Decode all fields.
    schema, role_value, session_id, client_proof = (
        _AUTH_FINISH_REQUEST.unpack(encoded)
    )

    # Require schema revision one.
    if schema != SECURITY_SCHEMA_VERSION:

        # Reject unknown semantics.
        raise ValueError(
            f"unsupported security schema version: {schema}"
        )

    # Return immutable proof request.
    return AuthFinish(
        role=SecurityRole(role_value),
        session_id=session_id,
        client_proof=client_proof,
    )


# Encode one successful AUTH_FINISH response.
def encode_authenticated_session(
    session: AuthenticatedSession,
) -> bytes:
    """Encode one fixed Guardian M10 session acknowledgement."""

    # Normalize role.
    role = SecurityRole(session.role)

    # Validate timeout field width.
    if not 1 <= session.timeout_seconds <= 0xFFFF:

        # Reject impossible session policy.
        raise ValueError(
            "timeout_seconds must be between 1 and 65535"
        )

    # Pack exact fixed response.
    return _AUTH_FINISH_RESPONSE.pack(
        SECURITY_SCHEMA_VERSION,
        int(role),
        session.session_id,
        session.timeout_seconds,
    )


# Decode one successful AUTH_FINISH response.
def decode_authenticated_session(
    payload: bytes,
) -> AuthenticatedSession:
    """Decode one fixed Guardian M10 session acknowledgement."""

    # Convert bytes-like input.
    encoded = bytes(payload)

    # Require exact fixed response size.
    if len(encoded) != _AUTH_FINISH_RESPONSE.size:

        # Reject malformed response.
        raise ValueError(
            (
                f"AUTH_FINISH response expected {_AUTH_FINISH_RESPONSE.size} bytes, "
                f"received {len(encoded)}"
            )
        )

    # Decode all fields.
    schema, role_value, session_id, timeout_seconds = (
        _AUTH_FINISH_RESPONSE.unpack(encoded)
    )

    # Require schema revision one.
    if schema != SECURITY_SCHEMA_VERSION:

        # Reject unknown semantics.
        raise ValueError(
            f"unsupported security schema version: {schema}"
        )

    # Return immutable session metadata.
    return AuthenticatedSession(
        role=SecurityRole(role_value),
        session_id=session_id,
        timeout_seconds=timeout_seconds,
    )


# Encode one public security-status payload.
def encode_security_status(
    status: SecurityStatus,
) -> bytes:
    """Encode one fixed Guardian M10 security-status payload."""

    # Normalize role.
    role = SecurityRole(status.active_role)

    # Pack the exact fixed public diagnostics.
    return _SECURITY_STATUS.pack(
        SECURITY_SCHEMA_VERSION,
        int(bool(status.configured)),
        int(bool(status.active)),
        int(role),
        status.session_id,
        status.next_counter,
        status.timeout_seconds,
        status.remaining_seconds,
        status.auth_successes,
        status.auth_failures,
        status.replay_rejections,
        status.unauthorized_rejections,
    )


# Decode one public security-status payload.
def decode_security_status(
    payload: bytes,
) -> SecurityStatus:
    """Decode one fixed Guardian M10 security-status payload."""

    # Convert bytes-like input.
    encoded = bytes(payload)

    # Require exact fixed payload size.
    if len(encoded) != _SECURITY_STATUS.size:

        # Reject malformed status.
        raise ValueError(
            (
                f"GET_SECURITY_STATUS expected {_SECURITY_STATUS.size} bytes, "
                f"received {len(encoded)}"
            )
        )

    # Decode the complete status.
    (
        schema,
        configured,
        active,
        role_value,
        session_id,
        next_counter,
        timeout_seconds,
        remaining_seconds,
        auth_successes,
        auth_failures,
        replay_rejections,
        unauthorized_rejections,
    ) = _SECURITY_STATUS.unpack(encoded)

    # Require schema revision one.
    if schema != SECURITY_SCHEMA_VERSION:

        # Reject unknown semantics.
        raise ValueError(
            f"unsupported security schema version: {schema}"
        )

    # Require canonical booleans.
    if configured not in (0, 1) or active not in (0, 1):

        # Reject ambiguous status semantics.
        raise ValueError(
            "security status booleans must be encoded as 0 or 1"
        )

    # Return immutable status.
    return SecurityStatus(
        configured=bool(configured),
        active=bool(active),
        active_role=SecurityRole(role_value),
        session_id=session_id,
        next_counter=next_counter,
        timeout_seconds=timeout_seconds,
        remaining_seconds=remaining_seconds,
        auth_successes=auth_successes,
        auth_failures=auth_failures,
        replay_rejections=replay_rejections,
        unauthorized_rejections=unauthorized_rejections,
    )


# Build canonical secure request MAC bytes.
def _secure_request_mac_input(
    session_id: int,
    counter: int,
    outer_sequence: int,
    inner_command: int,
    inner_payload: bytes,
) -> bytes:
    """Return canonical request bytes covered by HMAC."""

    # Convert inner payload to immutable bytes.
    payload = bytes(inner_payload)

    # Require the bounded inner request size.
    if len(payload) > MAX_SECURE_REQUEST_INNER_PAYLOAD:

        # Reject data that cannot fit in one Guardian frame.
        raise ValueError(
            (
                "secure inner request exceeds "
                f"{MAX_SECURE_REQUEST_INNER_PAYLOAD} bytes"
            )
        )

    # Return domain-separated exact canonical bytes.
    return (
        _REQUEST_MAC_LABEL
        + bytes((SECURITY_SCHEMA_VERSION,))
        + session_id.to_bytes(4, "big")
        + counter.to_bytes(8, "big")
        + outer_sequence.to_bytes(4, "big")
        + bytes((inner_command,))
        + len(payload).to_bytes(2, "big")
        + payload
    )


# Encode one authenticated secure request.
def encode_secure_request(
    session_key: bytes,
    session_id: int,
    counter: int,
    outer_sequence: int,
    inner_command: int,
    inner_payload: bytes = b"",
) -> bytes:
    """Encode one authenticated M10 SECURE_COMMAND request payload."""

    # Validate full session-key width.
    key = bytes(session_key)

    # Require exact full derived session key.
    if len(key) != SECURITY_SESSION_KEY_SIZE:

        # Reject invalid session-key state.
        raise ValueError(
            "session_key must contain exactly 32 bytes"
        )

    # Convert payload to immutable bytes.
    payload = bytes(inner_payload)

    # Build canonical authenticated bytes.
    mac_input = _secure_request_mac_input(
        session_id,
        counter,
        outer_sequence,
        inner_command,
        payload,
    )

    # Calculate and truncate HMAC-SHA-256.
    tag = hmac.new(
        key,
        mac_input,
        hashlib.sha256,
    ).digest()[:SECURITY_TAG_SIZE]

    # Pack the transmitted prefix.
    prefix = _SECURE_REQUEST_PREFIX.pack(
        SECURITY_SCHEMA_VERSION,
        session_id,
        counter,
        inner_command,
        len(payload),
    )

    # Return prefix + inner payload + tag.
    return prefix + payload + tag


# Decode and authenticate one secure request.
def decode_secure_request(
    payload: bytes,
    session_key: bytes,
    outer_sequence: int,
) -> SecureRequest:
    """Decode and verify one authenticated M10 secure request."""

    # Convert inputs to immutable bytes.
    encoded = bytes(payload)

    # Validate full session key.
    key = bytes(session_key)

    # Require exact full session key.
    if len(key) != SECURITY_SESSION_KEY_SIZE:

        # Reject invalid key state.
        raise ValueError(
            "session_key must contain exactly 32 bytes"
        )

    # Require fixed prefix plus tag.
    if len(encoded) < (
        _SECURE_REQUEST_PREFIX.size
        + SECURITY_TAG_SIZE
    ):

        # Reject truncated secure envelope.
        raise ValueError(
            "secure request payload is truncated"
        )

    # Decode fixed prefix.
    schema, session_id, counter, inner_command, inner_length = (
        _SECURE_REQUEST_PREFIX.unpack(
            encoded[:_SECURE_REQUEST_PREFIX.size]
        )
    )

    # Require schema revision one.
    if schema != SECURITY_SCHEMA_VERSION:

        # Reject unknown secure semantics.
        raise ValueError(
            f"unsupported security schema version: {schema}"
        )

    # Require exact variable envelope length.
    expected_length = (
        _SECURE_REQUEST_PREFIX.size
        + inner_length
        + SECURITY_TAG_SIZE
    )

    # Reject length mismatch.
    if len(encoded) != expected_length:

        # Reject trailing or truncated authenticated data.
        raise ValueError(
            "secure request inner length does not match payload size"
        )

    # Slice exact authenticated inner payload.
    inner_payload = encoded[
        _SECURE_REQUEST_PREFIX.size:
        _SECURE_REQUEST_PREFIX.size + inner_length
    ]

    # Slice transmitted tag.
    received_tag = encoded[-SECURITY_TAG_SIZE:]

    # Rebuild canonical HMAC input.
    mac_input = _secure_request_mac_input(
        session_id,
        counter,
        outer_sequence,
        inner_command,
        inner_payload,
    )

    # Calculate expected tag.
    expected_tag = hmac.new(
        key,
        mac_input,
        hashlib.sha256,
    ).digest()[:SECURITY_TAG_SIZE]

    # Compare tags in constant time.
    if not hmac.compare_digest(
        expected_tag,
        received_tag,
    ):

        # Reject message-authenticity failure.
        raise ValueError(
            "secure request authentication tag mismatch"
        )

    # Return the authenticated inner request.
    return SecureRequest(
        session_id=session_id,
        counter=counter,
        inner_command=inner_command,
        inner_payload=inner_payload,
    )


# Build canonical secure response MAC bytes.
def _secure_response_mac_input(
    session_id: int,
    counter: int,
    outer_sequence: int,
    inner_message_type: MessageType,
    inner_command: int,
    inner_payload: bytes,
) -> bytes:
    """Return canonical response bytes covered by HMAC."""

    # Convert payload to immutable bytes.
    payload = bytes(inner_payload)

    # Require the bounded inner response size.
    if len(payload) > MAX_SECURE_RESPONSE_INNER_PAYLOAD:

        # Reject data that cannot fit in one Guardian frame.
        raise ValueError(
            (
                "secure inner response exceeds "
                f"{MAX_SECURE_RESPONSE_INNER_PAYLOAD} bytes"
            )
        )

    # Normalize message type.
    message_type = MessageType(inner_message_type)

    # Return exact canonical domain-separated bytes.
    return (
        _RESPONSE_MAC_LABEL
        + bytes((SECURITY_SCHEMA_VERSION,))
        + session_id.to_bytes(4, "big")
        + counter.to_bytes(8, "big")
        + outer_sequence.to_bytes(4, "big")
        + bytes(
            (
                int(message_type),
                inner_command,
            )
        )
        + len(payload).to_bytes(2, "big")
        + payload
    )


# Encode one authenticated secure response.
def encode_secure_response(
    session_key: bytes,
    session_id: int,
    counter: int,
    outer_sequence: int,
    inner_message_type: MessageType,
    inner_command: int,
    inner_payload: bytes = b"",
) -> bytes:
    """Encode one authenticated M10 secure response payload."""

    # Validate full session-key width.
    key = bytes(session_key)

    # Require exact full session key.
    if len(key) != SECURITY_SESSION_KEY_SIZE:

        # Reject invalid key state.
        raise ValueError(
            "session_key must contain exactly 32 bytes"
        )

    # Normalize message type.
    message_type = MessageType(inner_message_type)

    # Convert inner payload.
    payload = bytes(inner_payload)

    # Build canonical authenticated bytes.
    mac_input = _secure_response_mac_input(
        session_id,
        counter,
        outer_sequence,
        message_type,
        inner_command,
        payload,
    )

    # Calculate truncated response tag.
    tag = hmac.new(
        key,
        mac_input,
        hashlib.sha256,
    ).digest()[:SECURITY_TAG_SIZE]

    # Pack the transmitted response prefix.
    prefix = _SECURE_RESPONSE_PREFIX.pack(
        SECURITY_SCHEMA_VERSION,
        session_id,
        counter,
        int(message_type),
        inner_command,
        len(payload),
    )

    # Return prefix + inner payload + tag.
    return prefix + payload + tag


# Decode and authenticate one secure response.
def decode_secure_response(
    payload: bytes,
    session_key: bytes,
    outer_sequence: int,
) -> SecureResponse:
    """Decode and verify one authenticated M10 secure response."""

    # Convert inputs to immutable bytes.
    encoded = bytes(payload)

    # Validate full session key.
    key = bytes(session_key)

    # Require exact full session key.
    if len(key) != SECURITY_SESSION_KEY_SIZE:

        # Reject invalid key state.
        raise ValueError(
            "session_key must contain exactly 32 bytes"
        )

    # Require fixed prefix plus tag.
    if len(encoded) < (
        _SECURE_RESPONSE_PREFIX.size
        + SECURITY_TAG_SIZE
    ):

        # Reject truncated secure response.
        raise ValueError(
            "secure response payload is truncated"
        )

    # Decode fixed prefix.
    (
        schema,
        session_id,
        counter,
        message_type_value,
        inner_command,
        inner_length,
    ) = _SECURE_RESPONSE_PREFIX.unpack(
        encoded[:_SECURE_RESPONSE_PREFIX.size]
    )

    # Require schema revision one.
    if schema != SECURITY_SCHEMA_VERSION:

        # Reject unknown semantics.
        raise ValueError(
            f"unsupported security schema version: {schema}"
        )

    # Require exact total envelope length.
    expected_length = (
        _SECURE_RESPONSE_PREFIX.size
        + inner_length
        + SECURITY_TAG_SIZE
    )

    # Reject length mismatch.
    if len(encoded) != expected_length:

        # Reject trailing or truncated authenticated data.
        raise ValueError(
            "secure response inner length does not match payload size"
        )

    # Normalize inner message type.
    message_type = MessageType(
        message_type_value
    )

    # Allow only normal synchronous inner results.
    if message_type not in (
        MessageType.RESPONSE,
        MessageType.ERROR,
    ):

        # Reject impossible secure synchronous semantics.
        raise ValueError(
            "secure inner response must be RESPONSE or ERROR"
        )

    # Slice exact inner payload.
    inner_payload = encoded[
        _SECURE_RESPONSE_PREFIX.size:
        _SECURE_RESPONSE_PREFIX.size + inner_length
    ]

    # Slice transmitted tag.
    received_tag = encoded[-SECURITY_TAG_SIZE:]

    # Rebuild canonical response MAC input.
    mac_input = _secure_response_mac_input(
        session_id,
        counter,
        outer_sequence,
        message_type,
        inner_command,
        inner_payload,
    )

    # Calculate expected tag.
    expected_tag = hmac.new(
        key,
        mac_input,
        hashlib.sha256,
    ).digest()[:SECURITY_TAG_SIZE]

    # Compare tags in constant time.
    if not hmac.compare_digest(
        expected_tag,
        received_tag,
    ):

        # Reject response authenticity failure.
        raise ValueError(
            "secure response authentication tag mismatch"
        )

    # Return authenticated inner response.
    return SecureResponse(
        session_id=session_id,
        counter=counter,
        inner_message_type=message_type,
        inner_command=inner_command,
        inner_payload=inner_payload,
    )
