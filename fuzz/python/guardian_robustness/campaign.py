"""Guardian M11 deterministic defensive fuzz and fault-injection campaigns."""

# Import hmac for constant-time server-proof verification.
import hmac

# Import random for reproducible campaign selection.
import random

# Import dataclass for immutable campaign reports.
from dataclasses import dataclass

# Import shared Guardian protocol and M10 security primitives.
from guardian_protocol import (
    AuthBegin,
    AuthFinish,
    BaselineAction,
    BaselineControl,
    Command,
    ErrorCode,
    Frame,
    HealthState,
    IncrementalParser,
    MAX_FRAME_SIZE,
    MessageType,
    SecurityRole,
    compute_client_proof,
    compute_server_proof,
    decode_auth_challenge,
    decode_authenticated_session,
    decode_health_status,
    derive_session_key,
    encode_auth_begin,
    encode_auth_finish,
    encode_baseline_control,
    encode_frame,
    encode_secure_request,
)

# Import the secure-mode software device.
from guardian_sim import GuardianDevice

# Import the intentionally public simulator-only demonstration key and configuration.
from guardian_sim.config import (
    DEFAULT_SECURITY_PSK_HEX,
    SimulatorConfig,
)

# Import deterministic mutation primitives.
from .mutations import (
    MutationKind,
    chunk_stream,
    mutate,
)


# Store one immutable M11 robustness campaign report.
@dataclass(frozen=True, slots=True)
class RobustnessReport:
    """Summary of deterministic defensive robustness campaigns."""

    # Store the master reproducibility seed.
    seed: int

    # Store requested iterations per mutation campaign.
    iterations: int

    # Count parser mutation cases executed.
    parser_cases: int

    # Count parser cases that recovered a canonical PING afterward.
    parser_recoveries: int

    # Count secure-envelope tamper cases executed.
    security_tamper_cases: int

    # Count tampered secure requests rejected without privileged state mutation.
    security_tamper_rejections: int

    # Count explicit exact-request replay rejections.
    replay_rejections: int

    # Count valid-MAC counter-gap rejections.
    counter_gap_rejections: int


# Build one ordinary request frame.
def _request(
    command: Command,
    sequence: int,
    payload: bytes = b"",
) -> Frame:
    """Return one canonical Guardian REQUEST frame."""

    # Return immutable request semantics.
    return Frame(
        message_type=MessageType.REQUEST,
        command=command,
        sequence=sequence,
        payload=payload,
    )


# Query the simulator M8 health state without changing it.
def _health_status(
    device: GuardianDevice,
    sequence: int,
):
    """Return one decoded simulator health snapshot."""

    # Execute the read-only health query.
    response = device.process_frame(
        _request(
            Command.GET_HEALTH_STATUS,
            sequence,
        )
    )

    # Require successful read-only response semantics.
    if response.message_type != MessageType.RESPONSE:

        # Fail the robustness harness rather than hiding product regression.
        raise AssertionError(
            "GET_HEALTH_STATUS failed during robustness campaign"
        )

    # Decode and return the immutable M8 snapshot.
    return decode_health_status(
        response.payload
    )


# Establish one real M10 simulator session and return its identity and derived key.
def _authenticate(
    device: GuardianDevice,
    psk: bytes,
    role: SecurityRole,
    rng: random.Random,
    sequence_base: int,
) -> tuple[int, bytes]:
    """Authenticate one secure-mode simulator session."""

    # Generate one deterministic host nonce for reproducible campaigns.
    client_nonce = bytes(
        rng.randrange(256)
        for _ in range(16)
    )

    # Start challenge-response authentication.
    begin_response = device.process_frame(
        _request(
            Command.AUTH_BEGIN,
            sequence_base,
            encode_auth_begin(
                AuthBegin(
                    role=role,
                    client_nonce=client_nonce,
                )
            ),
        )
    )

    # Require the device to return a challenge.
    if begin_response.message_type != MessageType.RESPONSE:

        # Fail loudly because campaign setup itself is invalid.
        raise AssertionError(
            "AUTH_BEGIN failed during robustness campaign"
        )

    # Decode the device challenge.
    challenge = decode_auth_challenge(
        begin_response.payload
    )

    # Calculate the expected server proof.
    expected_server_proof = compute_server_proof(
        psk,
        role,
        challenge.session_id,
        client_nonce,
        challenge.device_nonce,
    )

    # Require the device to prove possession of the configured PSK.
    if not hmac.compare_digest(
        expected_server_proof,
        challenge.server_proof,
    ):

        # Fail the campaign before trusting the session transcript.
        raise AssertionError(
            "server proof mismatch during robustness campaign"
        )

    # Calculate the client proof.
    client_proof = compute_client_proof(
        psk,
        role,
        challenge.session_id,
        client_nonce,
        challenge.device_nonce,
    )

    # Finish the authentication handshake.
    finish_response = device.process_frame(
        _request(
            Command.AUTH_FINISH,
            sequence_base + 1,
            encode_auth_finish(
                AuthFinish(
                    role=role,
                    session_id=challenge.session_id,
                    client_proof=client_proof,
                )
            ),
        )
    )

    # Require an authenticated session acknowledgement.
    if finish_response.message_type != MessageType.RESPONSE:

        # Fail campaign setup explicitly.
        raise AssertionError(
            "AUTH_FINISH failed during robustness campaign"
        )

    # Decode the fixed session acknowledgement.
    authenticated = decode_authenticated_session(
        finish_response.payload
    )

    # Require exact session identity.
    if (
        authenticated.session_id != challenge.session_id
        or authenticated.role != role
    ):

        # Reject contradictory authentication setup.
        raise AssertionError(
            "authenticated session metadata mismatch"
        )

    # Derive the same full per-session HMAC key.
    session_key = derive_session_key(
        psk,
        role,
        challenge.session_id,
        client_nonce,
        challenge.device_nonce,
    )

    # Return active session identity and key.
    return (
        challenge.session_id,
        session_key,
    )


# Run deterministic parser mutations and require bounded recovery.
def run_parser_campaign(
    iterations: int,
    seed: int,
) -> tuple[int, int]:
    """Mutate framed traffic and require parser recovery after bounded flush bytes."""

    # Reject nonsensical campaign sizes.
    if iterations <= 0:

        # Require at least one mutation.
        raise ValueError(
            "iterations must be positive"
        )

    # Create a local campaign RNG.
    rng = random.Random(
        seed
    )

    # Build several canonical read-only request seeds.
    seeds = (
        encode_frame(
            _request(
                Command.PING,
                1,
            )
        ),
        encode_frame(
            _request(
                Command.GET_STATUS,
                2,
            )
        ),
        encode_frame(
            _request(
                Command.GET_SECURITY_STATUS,
                3,
            )
        ),
    )

    # Build one canonical recovery PING with a distinctive sequence.
    recovery_ping = encode_frame(
        _request(
            Command.PING,
            0xA5A55A5A,
        )
    )

    # Enumerate every published mutation class.
    mutation_kinds = tuple(
        MutationKind
    )

    # Count executed mutation cases.
    cases = 0

    # Count successful bounded recoveries.
    recoveries = 0

    # Execute every requested mutation iteration.
    for iteration in range(
        iterations
    ):

        # Select one canonical seed frame.
        source = seeds[
            rng.randrange(
                len(seeds)
            )
        ]

        # Select one mutation class.
        kind = mutation_kinds[
            rng.randrange(
                len(mutation_kinds)
            )
        ]

        # Derive a per-case reproducibility seed.
        case_seed = rng.getrandbits(
            32
        )

        # Apply exactly one deterministic fault.
        case = mutate(
            source,
            kind,
            case_seed,
        )

        # Create independent parser state for this case.
        parser = IncrementalParser()

        # Feed the mutated stream through arbitrary transport fragmentation.
        for chunk in chunk_stream(
            case.data,
            case_seed ^ 0x13579BDF,
        ):

            # Parser must never raise on arbitrary transport bytes.
            parser.feed(
                chunk
            )

        # Flush any bounded incomplete candidate with bytes that cannot begin Guardian magic.
        for chunk in chunk_stream(
            bytes(MAX_FRAME_SIZE),
            case_seed ^ 0x2468ACE0,
        ):

            # Complete or discard any attacker-controlled bounded candidate.
            parser.feed(
                chunk
            )

        # Feed one canonical recovery PING after the bounded flush.
        recovered_frames = []

        # Fragment the recovery frame independently.
        for chunk in chunk_stream(
            recovery_ping,
            case_seed ^ 0x55AA55AA,
            maximum_chunk=5,
        ):

            # Collect all recovered valid frames.
            recovered_frames.extend(
                parser.feed(
                    chunk
                )
            )

        # Require the distinctive canonical PING to be recovered.
        if not any(
            frame.command == int(Command.PING)
            and frame.sequence == 0xA5A55A5A
            for frame in recovered_frames
        ):

            # Preserve exact reproduction metadata.
            raise AssertionError(
                (
                    "parser failed bounded recovery: "
                    f"iteration={iteration} "
                    f"kind={case.kind.name} "
                    f"seed={case.seed} "
                    f"description={case.description}"
                )
            )

        # Count one completed mutation case.
        cases += 1

        # Count one successful parser recovery.
        recoveries += 1

    # Return deterministic counters.
    return (
        cases,
        recoveries,
    )


# Run secure-envelope tampering and require fail-closed privileged state.
def run_security_tamper_campaign(
    iterations: int,
    seed: int,
) -> tuple[int, int]:
    """Mutate valid-MAC command envelopes and require rejection before state mutation."""

    # Reject nonsensical campaign sizes.
    if iterations <= 0:

        # Require at least one mutation.
        raise ValueError(
            "iterations must be positive"
        )

    # Create one local campaign RNG.
    rng = random.Random(
        seed
    )

    # Decode the intentionally public simulator-only PSK.
    psk = bytes.fromhex(
        DEFAULT_SECURITY_PSK_HEX
    )

    # Enumerate every deterministic mutation class.
    mutation_kinds = tuple(
        MutationKind
    )

    # Count executed tamper cases.
    cases = 0

    # Count rejected tamper cases.
    rejections = 0

    # Execute every requested mutation on a fresh session/device.
    for iteration in range(
        iterations
    ):

        # Create a fresh secure-mode simulator so no prior state can mask a mutation.
        device = GuardianDevice(
            SimulatorConfig(
                security_enabled=True,
            )
        )

        # Establish one real OPERATOR session.
        session_id, session_key = _authenticate(
            device,
            psk,
            SecurityRole.OPERATOR,
            rng,
            1000 + (iteration * 10),
        )

        # Require a fresh device to remain untrained before the privileged request.
        before = _health_status(
            device,
            1002 + (iteration * 10),
        )

        # Guard the campaign's no-side-effect baseline assumption.
        if before.state != HealthState.UNTRAINED:

            # Fail if simulator setup unexpectedly mutates health state.
            raise AssertionError(
                "fresh secure simulator was not UNTRAINED"
            )

        # Encode one valid protected baseline mutation at counter one.
        valid_secure_payload = encode_secure_request(
            session_key=session_key,
            session_id=session_id,
            counter=1,
            outer_sequence=1003 + (iteration * 10),
            inner_command=int(Command.BASELINE_CONTROL),
            inner_payload=encode_baseline_control(
                BaselineControl(
                    action=BaselineAction.START,
                    target_samples=16,
                )
            ),
        )

        # Select one mutation class and per-case seed.
        kind = mutation_kinds[
            rng.randrange(
                len(mutation_kinds)
            )
        ]

        # Derive the exact reproducibility seed.
        case_seed = rng.getrandbits(
            32
        )

        # Mutate the authenticated envelope after its tag was calculated.
        case = mutate(
            valid_secure_payload,
            kind,
            case_seed,
        )

        # Deliver the tampered envelope under the original outer sequence.
        response = device.process_frame(
            _request(
                Command.SECURE_COMMAND,
                1003 + (iteration * 10),
                case.data,
            )
        )

        # Require a plain outer security rejection.
        if response.message_type != MessageType.ERROR:

            # Fail because invalid authenticated bytes must never reach application dispatch.
            raise AssertionError(
                (
                    "tampered secure command was not rejected: "
                    f"iteration={iteration} "
                    f"kind={case.kind.name} "
                    f"seed={case.seed} "
                    f"description={case.description}"
                )
            )

        # Require authenticity failure rather than successful application execution.
        if response.payload != bytes(
            (
                int(
                    ErrorCode.UNAUTHORIZED
                ),
            )
        ):

            # Fail on unexpected security classification.
            raise AssertionError(
                (
                    "tampered secure command returned unexpected error: "
                    f"{response.payload.hex()}"
                )
            )

        # Query health after the rejected tamper.
        after = _health_status(
            device,
            1004 + (iteration * 10),
        )

        # Require no privileged baseline state mutation.
        if after.state != HealthState.UNTRAINED:

            # Fail closed when any tampered command changes application state.
            raise AssertionError(
                (
                    "tampered secure command changed health state: "
                    f"iteration={iteration} "
                    f"kind={case.kind.name} "
                    f"seed={case.seed}"
                )
            )

        # Require no baseline samples to have been learned.
        if after.baseline_samples != 0:

            # Reject hidden partial state changes.
            raise AssertionError(
                "tampered secure command changed baseline samples"
            )

        # Count one executed case.
        cases += 1

        # Count one verified fail-closed rejection.
        rejections += 1

    # Return deterministic campaign counters.
    return (
        cases,
        rejections,
    )


# Run explicit replay and valid-MAC counter-gap fault injection.
def run_counter_fault_campaign(
    seed: int,
) -> tuple[int, int]:
    """Require exact replay and out-of-order counters to fail without re-execution."""

    # Create one local deterministic RNG.
    rng = random.Random(
        seed
    )

    # Decode the simulator-only demonstration key.
    psk = bytes.fromhex(
        DEFAULT_SECURITY_PSK_HEX
    )

    # Create one fresh secure-mode simulator.
    device = GuardianDevice(
        SimulatorConfig(
            security_enabled=True,
        )
    )

    # Establish one real OPERATOR session.
    session_id, session_key = _authenticate(
        device,
        psk,
        SecurityRole.OPERATOR,
        rng,
        5000,
    )

    # Define one valid baseline-start inner payload.
    baseline_payload = encode_baseline_control(
        BaselineControl(
            action=BaselineAction.START,
            target_samples=16,
        )
    )

    # Build the exact counter-one protected request.
    first_payload = encode_secure_request(
        session_key=session_key,
        session_id=session_id,
        counter=1,
        outer_sequence=5002,
        inner_command=int(Command.BASELINE_CONTROL),
        inner_payload=baseline_payload,
    )

    # Build the ordinary outer request once so exact bytes can be replayed.
    first_request = _request(
        Command.SECURE_COMMAND,
        5002,
        first_payload,
    )

    # Execute the valid protected request once.
    first_response = device.process_frame(
        first_request
    )

    # Require successful authenticated outer response.
    if first_response.message_type != MessageType.RESPONSE:

        # Fail campaign setup.
        raise AssertionError(
            "valid secure baseline request failed"
        )

    # Snapshot state after the one intended execution.
    after_first = _health_status(
        device,
        5003,
    )

    # Require the intended baseline to complete in the software simulator.
    if after_first.state != HealthState.READY:

        # Fail if the valid control path did not execute.
        raise AssertionError(
            "valid secure baseline did not reach READY"
        )

    # Replay the exact same authenticated outer request.
    replay_response = device.process_frame(
        first_request
    )

    # Require explicit replay detection.
    if (
        replay_response.message_type != MessageType.ERROR
        or replay_response.payload != bytes(
            (
                int(
                    ErrorCode.REPLAY_DETECTED
                ),
            )
        )
    ):

        # Reject any replay ambiguity.
        raise AssertionError(
            "exact secure request replay was not rejected"
        )

    # Verify replay did not execute baseline a second time.
    after_replay = _health_status(
        device,
        5004,
    )

    # Require exact health-state stability.
    if after_replay != after_first:

        # Reject hidden replay side effects.
        raise AssertionError(
            "replayed secure request changed device health state"
        )

    # Build a valid-HMAC request that skips expected counter two and uses three.
    gap_payload = encode_secure_request(
        session_key=session_key,
        session_id=session_id,
        counter=3,
        outer_sequence=5005,
        inner_command=int(Command.BASELINE_CONTROL),
        inner_payload=encode_baseline_control(
            BaselineControl(
                action=BaselineAction.RESET,
                target_samples=0,
            )
        ),
    )

    # Deliver the valid-MAC out-of-order request.
    gap_response = device.process_frame(
        _request(
            Command.SECURE_COMMAND,
            5005,
            gap_payload,
        )
    )

    # Require strict counter-gap rejection.
    if (
        gap_response.message_type != MessageType.ERROR
        or gap_response.payload != bytes(
            (
                int(
                    ErrorCode.REPLAY_DETECTED
                ),
            )
        )
    ):

        # Fail if the strict anti-replay policy accepts a skipped counter.
        raise AssertionError(
            "valid-MAC counter gap was not rejected"
        )

    # Verify the skipped-counter reset did not execute.
    after_gap = _health_status(
        device,
        5006,
    )

    # Require state to remain exactly unchanged.
    if after_gap != after_first:

        # Reject hidden out-of-order side effects.
        raise AssertionError(
            "counter-gap secure request changed device health state"
        )

    # Report one replay and one counter-gap rejection.
    return (
        1,
        1,
    )


# Run the complete deterministic M11 robustness campaign.
def run_all_campaigns(
    iterations: int,
    seed: int,
) -> RobustnessReport:
    """Execute parser, secure-tamper, replay and counter-gap campaigns."""

    # Derive independent parser campaign seed.
    parser_seed = (
        seed
        ^ 0x50415253
    )

    # Derive independent secure-tamper campaign seed.
    security_seed = (
        seed
        ^ 0x53454355
    )

    # Derive independent counter fault seed.
    counter_seed = (
        seed
        ^ 0x434F554E
    )

    # Execute parser mutation/recovery campaign.
    parser_cases, parser_recoveries = run_parser_campaign(
        iterations,
        parser_seed,
    )

    # Execute secure-envelope tamper campaign.
    security_cases, security_rejections = (
        run_security_tamper_campaign(
            iterations,
            security_seed,
        )
    )

    # Execute explicit replay and counter-gap fault injection.
    replay_rejections, counter_gap_rejections = (
        run_counter_fault_campaign(
            counter_seed
        )
    )

    # Return one immutable complete report.
    return RobustnessReport(
        seed=seed,
        iterations=iterations,
        parser_cases=parser_cases,
        parser_recoveries=parser_recoveries,
        security_tamper_cases=security_cases,
        security_tamper_rejections=security_rejections,
        replay_rejections=replay_rejections,
        counter_gap_rejections=counter_gap_rejections,
    )
