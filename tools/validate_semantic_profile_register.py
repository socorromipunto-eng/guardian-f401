#!/usr/bin/env python3
from __future__ import annotations

import argparse
import copy
import json
import sys
from collections import Counter
from pathlib import Path
from typing import Any

from jsonschema import Draft202012Validator
from jsonschema.exceptions import SchemaError


EXPECTED_REGISTRY_SCHEMA_VERSION = "1.0.0"
EXPECTED_REGISTRY_ID = "guardian:semantic-profile-registry:m16"
EXPECTED_REGISTRY_ROLE = "CONTROLLED_SEMANTIC_PROFILE_INDEX"

EXPECTED_DOCUMENT_REGISTER_SCHEMA = "2.0.0"
EXPECTED_DECISION_REGISTER_SCHEMA = "1.0.0"

EXPECTED_STATE_WIRE_VALUES = {
    "BOOT": 0,
    "DISCOVERING": 1,
    "ACTIVE": 2,
    "DEGRADED": 3,
    "SAFE_HOLD": 4,
    "FAULT": 5,
}

COMPATIBILITY_OUTCOMES = {
    "COMPATIBLE",
    "INCOMPATIBLE",
    "UNKNOWN",
    "UNSUPPORTED",
    "HISTORICALLY_IMPOSSIBLE",
}

FAIL_CLOSED_OUTCOMES = {
    "INCOMPATIBLE",
    "UNKNOWN",
    "UNSUPPORTED",
    "HISTORICALLY_IMPOSSIBLE",
}

AUTHORITY_FIELDS = (
    "semantic_compatibility_grants_authority",
    "freshness_grants_authority",
    "authentication_grants_authority",
    "remote_state_grants_local_transition",
    "remote_message_grants_actuation",
    "ai_advisory_grants_authority",
    "direct_actuation_authority",
)

REGISTRATION_FIELDS = (
    "registration_grants_approval",
    "registration_grants_implementation",
    "registration_grants_validation",
    "registration_grants_authority",
    "registration_grants_actuation",
)

VERSION_CONTEXT_FIELDS = (
    "hardware_revision",
    "platform_version",
    "firmware_version",
)


class DuplicateKeyError(ValueError):
    pass


def _strict_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}

    for key, value in pairs:
        if key in result:
            raise DuplicateKeyError("DUPLICATE_JSON_KEY:" + key)
        result[key] = value

    return result


def load_json_strict(path: Path) -> Any:
    text = path.read_text(encoding="utf-8")

    return json.loads(
        text,
        object_pairs_hook=_strict_object,
    )


def add_error(errors: list[str], reason: str) -> None:
    errors.append(reason)


def exact_cardinality(
    values: list[dict[str, Any]],
    field: str,
    wanted: str,
) -> int:
    return sum(
        1
        for item in values
        if isinstance(item, dict)
        and item.get(field) == wanted
    )


def exact_document_cardinality(
    documents: list[dict[str, Any]],
    document_id: str,
    document_version: str,
) -> int:
    return sum(
        1
        for item in documents
        if isinstance(item, dict)
        and item.get("id") == document_id
        and item.get("version") == document_version
    )


def profile_index(
    profiles: list[dict[str, Any]],
) -> dict[str, dict[str, Any]]:
    return {
        profile["profile_id"]: profile
        for profile in profiles
        if isinstance(profile, dict)
        and isinstance(profile.get("profile_id"), str)
    }


def has_unknown_required_context(
    profile: dict[str, Any],
) -> bool:
    context = profile.get("platform_context", {})

    for field in VERSION_CONTEXT_FIELDS:
        value = context.get(field)

        if not isinstance(value, dict):
            return True

        if value.get("state") == "UNKNOWN":
            return True

    return False


def compatibility_tuple(
    contract: dict[str, Any],
) -> tuple[str, str, str, str]:
    return (
        str(contract.get("producer_profile_id", "")),
        str(contract.get("consumer_profile_id", "")),
        str(contract.get("protocol_id", "")),
        str(contract.get("protocol_version", "")),
    )


def resolve_compatibility(
    registry: dict[str, Any],
    producer_profile_id: str,
    consumer_profile_id: str,
    protocol_id: str,
    protocol_version: str,
) -> dict[str, str]:
    """
    Exact-tuple resolver.

    Absence is not compatibility. Absence resolves to UNKNOWN / FAIL_CLOSED.
    More than one exact contract is ambiguous and also resolves fail closed.
    """

    matches = [
        contract
        for contract in registry.get("compatibility_contracts", [])
        if compatibility_tuple(contract)
        == (
            producer_profile_id,
            consumer_profile_id,
            protocol_id,
            protocol_version,
        )
    ]

    if len(matches) == 0:
        return {
            "resolution": "UNKNOWN",
            "authority_relevant_effect": "FAIL_CLOSED",
            "reason": "NO_EXACT_COMPATIBILITY_CONTRACT",
        }

    if len(matches) != 1:
        return {
            "resolution": "UNKNOWN",
            "authority_relevant_effect": "FAIL_CLOSED",
            "reason": "AMBIGUOUS_EXACT_COMPATIBILITY_CONTRACT",
        }

    contract = matches[0]

    pre = contract["pre_freshness_compatibility"]
    full = contract["full_message_semantic_compatibility"]

    if pre != "COMPATIBLE" or full != "COMPATIBLE":
        return {
            "resolution": (
                full
                if full in COMPATIBILITY_OUTCOMES
                else "UNKNOWN"
            ),
            "authority_relevant_effect": "FAIL_CLOSED",
            "reason": "EXPLICIT_NON_COMPATIBLE_OUTCOME",
        }

    return {
        "resolution": "COMPATIBLE",
        "authority_relevant_effect": "ELIGIBLE_FOR_LATER_POLICY_ONLY",
        "reason": "EXACT_GOVERNED_CONTRACT",
    }


def validate_registry_data(
    schema: dict[str, Any],
    registry: dict[str, Any],
    document_register: dict[str, Any],
    decision_register: dict[str, Any],
) -> list[str]:

    errors: list[str] = []

    # ------------------------------------------------------------------
    # R-33: schema/version/canonical container discovery
    # ------------------------------------------------------------------

    if registry.get("schema_version") != EXPECTED_REGISTRY_SCHEMA_VERSION:
        add_error(
            errors,
            "UNSUPPORTED_SEMANTIC_PROFILE_REGISTRY_SCHEMA_VERSION:"
            + str(registry.get("schema_version")),
        )
        return errors

    if registry.get("registry_id") != EXPECTED_REGISTRY_ID:
        add_error(errors, "REGISTRY_ID_MISMATCH")

    if registry.get("registry_role") != EXPECTED_REGISTRY_ROLE:
        add_error(errors, "REGISTRY_ROLE_MISMATCH")

    if (
        document_register.get("schema_version")
        != EXPECTED_DOCUMENT_REGISTER_SCHEMA
    ):
        add_error(
            errors,
            "UNSUPPORTED_DOCUMENT_REGISTER_SCHEMA_VERSION:"
            + str(document_register.get("schema_version")),
        )
        return errors

    documents = document_register.get("documents")

    if not isinstance(documents, list):
        add_error(
            errors,
            "DOCUMENT_REGISTER_CANONICAL_CONTAINER_MISSING:documents",
        )
        return errors

    if (
        decision_register.get("schema_version")
        != EXPECTED_DECISION_REGISTER_SCHEMA
    ):
        add_error(
            errors,
            "UNSUPPORTED_DECISION_REGISTER_SCHEMA_VERSION:"
            + str(decision_register.get("schema_version")),
        )
        return errors

    decisions = decision_register.get("decisions")

    if not isinstance(decisions, list):
        add_error(
            errors,
            "DECISION_REGISTER_CANONICAL_CONTAINER_MISSING:decisions",
        )
        return errors

    # ------------------------------------------------------------------
    # JSON Schema
    # ------------------------------------------------------------------

    try:
        Draft202012Validator.check_schema(schema)
    except SchemaError as exc:
        add_error(errors, "SCHEMA_META_VALIDATION:" + str(exc))
        return errors

    schema_errors = sorted(
        Draft202012Validator(schema).iter_errors(registry),
        key=lambda error: [
            str(item)
            for item in error.absolute_path
        ],
    )

    for error in schema_errors:
        location = "/".join(
            str(item)
            for item in error.absolute_path
        )

        add_error(
            errors,
            "SCHEMA_VALIDATION:"
            + location
            + ":"
            + error.message,
        )

    if schema_errors:
        return errors

    # ------------------------------------------------------------------
    # Root governance non-escalation
    # ------------------------------------------------------------------

    governance = registry["governance"]

    for field in REGISTRATION_FIELDS:
        if governance.get(field) is not False:
            add_error(
                errors,
                "REGISTRATION_ESCALATION:" + field,
            )

    if governance.get("unknown_semantic_context") != "FAIL_CLOSED":
        add_error(
            errors,
            "UNKNOWN_SEMANTIC_CONTEXT_NOT_FAIL_CLOSED",
        )

    # ------------------------------------------------------------------
    # Profiles
    # ------------------------------------------------------------------

    profiles = registry["profiles"]

    profile_ids = [
        profile["profile_id"]
        for profile in profiles
    ]

    counts = Counter(profile_ids)

    for profile_id, count in sorted(counts.items()):
        if count != 1:
            add_error(
                errors,
                "PROFILE_ID_CARDINALITY:"
                + profile_id
                + ":"
                + str(count),
            )

    profiles_by_id = profile_index(profiles)

    for profile in profiles:
        profile_id = profile["profile_id"]

        # Exact wire mapping: schema alone currently permits 0..255.
        semantics = profile["state_semantics"]

        for state_name, expected_wire_value in (
            EXPECTED_STATE_WIRE_VALUES.items()
        ):
            actual = semantics[state_name]["wire_value"]

            if actual != expected_wire_value:
                add_error(
                    errors,
                    "STATE_WIRE_VALUE_MISMATCH:"
                    + profile_id
                    + ":"
                    + state_name
                    + ":"
                    + str(actual)
                    + "!="
                    + str(expected_wire_value),
                )

            if semantics[state_name]["authority_effect"] != "NONE":
                add_error(
                    errors,
                    "STATE_AUTHORITY_EFFECT_NOT_NONE:"
                    + profile_id
                    + ":"
                    + state_name,
                )

            if semantics[state_name]["actuation_effect"] != "NONE":
                add_error(
                    errors,
                    "STATE_ACTUATION_EFFECT_NOT_NONE:"
                    + profile_id
                    + ":"
                    + state_name,
                )

        authority = profile["authority_semantics"]

        for field in AUTHORITY_FIELDS:
            if authority.get(field) is not False:
                add_error(
                    errors,
                    "PROFILE_AUTHORITY_ESCALATION:"
                    + profile_id
                    + ":"
                    + field,
                )

        # R-33/R-34 source-document exact cardinality.
        provenance = profile["provenance"]

        for document_ref in provenance["source_document_refs"]:
            document_id = document_ref["id"]
            document_version = document_ref["version"]

            cardinality = exact_document_cardinality(
                documents,
                document_id,
                document_version,
            )

            if cardinality != 1:
                add_error(
                    errors,
                    "SOURCE_DOCUMENT_REF_CARDINALITY:"
                    + document_id
                    + "@"
                    + document_version
                    + ":"
                    + str(cardinality),
                )

        for decision_ref in provenance["decision_refs"]:
            cardinality = exact_cardinality(
                decisions,
                "id",
                decision_ref,
            )

            if cardinality != 1:
                add_error(
                    errors,
                    "DECISION_REF_CARDINALITY:"
                    + decision_ref
                    + ":"
                    + str(cardinality),
                )

    # ------------------------------------------------------------------
    # Compatibility contracts
    # ------------------------------------------------------------------

    contracts = registry["compatibility_contracts"]

    contract_ids = [
        contract["compatibility_contract_id"]
        for contract in contracts
    ]

    for contract_id, count in sorted(
        Counter(contract_ids).items()
    ):
        if count != 1:
            add_error(
                errors,
                "COMPATIBILITY_CONTRACT_ID_CARDINALITY:"
                + contract_id
                + ":"
                + str(count),
            )

    tuples = [
        compatibility_tuple(contract)
        for contract in contracts
    ]

    tuple_counts = Counter(tuples)

    for key, count in sorted(tuple_counts.items()):
        if count != 1:
            add_error(
                errors,
                "COMPATIBILITY_TUPLE_CARDINALITY:"
                + "|".join(key)
                + ":"
                + str(count),
            )

    for contract in contracts:
        contract_id = contract["compatibility_contract_id"]

        producer_id = contract["producer_profile_id"]
        consumer_id = contract["consumer_profile_id"]

        producer_cardinality = counts.get(producer_id, 0)
        consumer_cardinality = counts.get(consumer_id, 0)

        if producer_cardinality != 1:
            add_error(
                errors,
                "PRODUCER_PROFILE_REF_CARDINALITY:"
                + contract_id
                + ":"
                + producer_id
                + ":"
                + str(producer_cardinality),
            )

        if consumer_cardinality != 1:
            add_error(
                errors,
                "CONSUMER_PROFILE_REF_CARDINALITY:"
                + contract_id
                + ":"
                + consumer_id
                + ":"
                + str(consumer_cardinality),
            )

        if (
            producer_cardinality != 1
            or consumer_cardinality != 1
        ):
            continue

        producer = profiles_by_id[producer_id]
        consumer = profiles_by_id[consumer_id]

        for role, profile in (
            ("PRODUCER", producer),
            ("CONSUMER", consumer),
        ):
            context = profile["platform_context"]

            if contract["protocol_id"] != context["protocol_id"]:
                add_error(
                    errors,
                    "COMPATIBILITY_PROTOCOL_ID_MISMATCH:"
                    + contract_id
                    + ":"
                    + role,
                )

            if (
                contract["protocol_version"]
                != context["protocol_version"]
            ):
                add_error(
                    errors,
                    "COMPATIBILITY_PROTOCOL_VERSION_MISMATCH:"
                    + contract_id
                    + ":"
                    + role,
                )

        if contract["authority_effect"] != "NONE":
            add_error(
                errors,
                "COMPATIBILITY_AUTHORITY_EFFECT_NOT_NONE:"
                + contract_id,
            )

        if contract["actuation_effect"] != "NONE":
            add_error(
                errors,
                "COMPATIBILITY_ACTUATION_EFFECT_NOT_NONE:"
                + contract_id,
            )

        pre = contract["pre_freshness_compatibility"]
        full = contract["full_message_semantic_compatibility"]

        if (
            pre == "COMPATIBLE"
            or full == "COMPATIBLE"
        ):
            unknown_roles = []

            if has_unknown_required_context(producer):
                unknown_roles.append("PRODUCER")

            if has_unknown_required_context(consumer):
                unknown_roles.append("CONSUMER")

            if unknown_roles:
                add_error(
                    errors,
                    "COMPATIBLE_WITH_UNKNOWN_REQUIRED_CONTEXT:"
                    + contract_id
                    + ":"
                    + ",".join(unknown_roles),
                )

    # ------------------------------------------------------------------
    # Semantic change boundaries
    # ------------------------------------------------------------------

    boundaries = registry["semantic_change_boundaries"]

    boundary_ids = [
        boundary["boundary_id"]
        for boundary in boundaries
    ]

    for boundary_id, count in sorted(
        Counter(boundary_ids).items()
    ):
        if count != 1:
            add_error(
                errors,
                "SEMANTIC_CHANGE_BOUNDARY_ID_CARDINALITY:"
                + boundary_id
                + ":"
                + str(count),
            )

    for boundary in boundaries:
        boundary_id = boundary["boundary_id"]

        predecessor = boundary["predecessor_profile_id"]
        successor = boundary["successor_profile_id"]

        predecessor_cardinality = counts.get(predecessor, 0)

        successor_cardinality = counts.get(successor, 0)

        if predecessor_cardinality != 1:
            add_error(
                errors,
                "SEMANTIC_CHANGE_PREDECESSOR_CARDINALITY:"
                + boundary_id
                + ":"
                + predecessor
                + ":"
                + str(predecessor_cardinality),
            )

        if successor_cardinality != 1:
            add_error(
                errors,
                "SEMANTIC_CHANGE_SUCCESSOR_CARDINALITY:"
                + boundary_id
                + ":"
                + successor
                + ":"
                + str(successor_cardinality),
            )

        if predecessor == successor:
            add_error(
                errors,
                "SEMANTIC_CHANGE_SELF_LOOP:"
                + boundary_id,
            )

        if boundary["historical_reinterpretation_allowed"] is not False:
            add_error(
                errors,
                "HISTORICAL_REINTERPRETATION_ALLOWED:"
                + boundary_id,
            )

        decision_ref = boundary["decision_ref"]

        decision_cardinality = exact_cardinality(
            decisions,
            "id",
            decision_ref,
        )

        if decision_cardinality != 1:
            add_error(
                errors,
                "SEMANTIC_CHANGE_DECISION_REF_CARDINALITY:"
                + boundary_id
                + ":"
                + decision_ref
                + ":"
                + str(decision_cardinality),
            )

    return errors


def validate_paths(
    schema_path: Path,
    registry_path: Path,
    document_register_path: Path,
    decision_register_path: Path,
) -> list[str]:

    try:
        schema = load_json_strict(schema_path)
        registry = load_json_strict(registry_path)
        document_register = load_json_strict(
            document_register_path
        )
        decision_register = load_json_strict(
            decision_register_path
        )
    except Exception as exc:
        return [
            "STRICT_JSON_PARSE:"
            + type(exc).__name__
            + ":"
            + str(exc)
        ]

    return validate_registry_data(
        schema,
        registry,
        document_register,
        decision_register,
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Validate Guardian M16 semantic profile "
            "R-33/R-34 coherence."
        )
    )

    parser.add_argument("--repo", default=".")

    parser.add_argument(
        "--schema",
        default=(
            "governance/schemas/"
            "semantic-profile-register-v1.schema.json"
        ),
    )

    parser.add_argument(
        "--registry",
        default=(
            "governance/semantic/"
            "m16-semantic-profile-register.json"
        ),
    )

    parser.add_argument(
        "--document-register",
        default="governance/document-register.json",
    )

    parser.add_argument(
        "--decision-register",
        default="governance/decision-register.json",
    )

    args = parser.parse_args()

    repo = Path(args.repo).resolve()

    schema_path = repo / args.schema
    registry_path = repo / args.registry
    document_register_path = (
        repo / args.document_register
    )
    decision_register_path = (
        repo / args.decision_register
    )

    errors = validate_paths(
        schema_path,
        registry_path,
        document_register_path,
        decision_register_path,
    )

    if errors:
        print(
            "SEMANTIC_PROFILE_R33_R34_VALIDATION=FAIL"
        )

        for reason in errors:
            print("REASON=" + reason)

        print("AUTHORITY_GRANTED=NO")
        print("ACTUATION_AUTHORIZED=NO")

        return 2

    print("SEMANTIC_PROFILE_REGISTRY_SCHEMA=1.0.0")
    print("PROFILE_IDENTIFIER_COHERENCE=PASS")
    print("STATE_WIRE_VALUE_BINDING=PASS")
    print("SOURCE_DOCUMENT_REFERENCE_COHERENCE=PASS")
    print("DECISION_REFERENCE_COHERENCE=PASS")
    print("COMPATIBILITY_REFERENCE_COHERENCE=PASS")
    print("COMPATIBILITY_TUPLE_CARDINALITY=PASS")
    print("SEMANTIC_CHANGE_REFERENCE_COHERENCE=PASS")
    print("REGISTRATION_NON_ESCALATION=PASS")
    print("AUTHORITY_GRANTED=NO")
    print("ACTUATION_AUTHORIZED=NO")
    print("SEMANTIC_PROFILE_R33_R34_VALIDATION=PASS")

    return 0


if __name__ == "__main__":
    sys.exit(main())
