#ifndef GUARDIAN_MULTINODE_H
#define GUARDIAN_MULTINODE_H

#include <stddef.h>
#include <stdint.h>

#define GUARDIAN_MULTINODE_MAX_NODES ((size_t)5U)

#define GUARDIAN_CAP_OBSERVATION        ((uint32_t)1U << 0)
#define GUARDIAN_CAP_SUPERVISION        ((uint32_t)1U << 1)
#define GUARDIAN_CAP_EVIDENCE_WITNESS   ((uint32_t)1U << 2)
#define GUARDIAN_CAP_RECOVERY_ASSIST    ((uint32_t)1U << 3)
#define GUARDIAN_CAP_SIGNAL_PROCESSING  ((uint32_t)1U << 4)
#define GUARDIAN_CAP_POLICY_REFERENCE   ((uint32_t)1U << 5)
#define GUARDIAN_CAP_ACTUATION_GATE     ((uint32_t)1U << 6)
#define GUARDIAN_CAP_SIGNATURE_VERIFY   ((uint32_t)1U << 7)
#define GUARDIAN_CAP_SENSOR_ACQUISITION ((uint32_t)1U << 8)

#define GUARDIAN_CAP_ALL ((uint32_t)0x000001FFU)

typedef enum
{
    GUARDIAN_NODE_ROLE_INVALID = 0,

    GUARDIAN_NODE_ROLE_F401_REFERENCE_CONTROL,

    GUARDIAN_NODE_ROLE_MCXN947_SUPERVISORY_PEER,

    GUARDIAN_NODE_ROLE_ST_MEASUREMENT_REFERENCE,

    GUARDIAN_NODE_ROLE_SECURITY_WITNESS,

    GUARDIAN_NODE_ROLE_HIGH_COMPUTE_PROCESSING

} guardian_multinode_role_t;

typedef enum
{
    GUARDIAN_AUTHORITY_CLASS_NONE = 0,

    GUARDIAN_AUTHORITY_CLASS_OBSERVE,

    GUARDIAN_AUTHORITY_CLASS_WITNESS,

    GUARDIAN_AUTHORITY_CLASS_REFERENCE_CONTROL

} guardian_multinode_authority_class_t;

typedef enum
{
    GUARDIAN_MULTINODE_OK = 0,

    GUARDIAN_MULTINODE_ERROR_NULL_ARGUMENT,

    GUARDIAN_MULTINODE_ERROR_INVALID_IDENTITY,

    GUARDIAN_MULTINODE_ERROR_INVALID_ROLE,

    GUARDIAN_MULTINODE_ERROR_INVALID_AUTHORITY_CLASS,

    GUARDIAN_MULTINODE_ERROR_INVALID_CAPABILITIES,

    GUARDIAN_MULTINODE_ERROR_AUTHORITY_ESCALATION,

    GUARDIAN_MULTINODE_ERROR_REGISTRY_FULL,

    GUARDIAN_MULTINODE_ERROR_DUPLICATE_NODE_ID,

    GUARDIAN_MULTINODE_ERROR_DUPLICATE_PRODUCER_ID,

    GUARDIAN_MULTINODE_ERROR_DUPLICATE_ROLE,

    GUARDIAN_MULTINODE_ERROR_TOPOLOGY_INCOMPLETE,

    GUARDIAN_MULTINODE_ERROR_NOT_REFERENCE_CONTROL

} guardian_multinode_result_t;

typedef struct
{
    uint32_t node_id;

    uint32_t producer_id;

    uint32_t key_id;

    guardian_multinode_role_t role;

    guardian_multinode_authority_class_t authority_class;

    uint32_t capabilities;

} guardian_multinode_node_t;

typedef struct
{
    guardian_multinode_node_t
        nodes[GUARDIAN_MULTINODE_MAX_NODES];

    size_t node_count;

} guardian_multinode_registry_t;

void guardian_multinode_registry_init(
    guardian_multinode_registry_t *registry);

guardian_multinode_result_t
guardian_multinode_validate_node(
    const guardian_multinode_node_t *node);

guardian_multinode_result_t
guardian_multinode_register_node(
    guardian_multinode_registry_t *registry,
    const guardian_multinode_node_t *node);

guardian_multinode_result_t
guardian_multinode_validate_complete_topology(
    const guardian_multinode_registry_t *registry);

guardian_multinode_result_t
guardian_multinode_can_request_actuation(
    const guardian_multinode_registry_t *registry,
    uint32_t requesting_node_id,
    uint8_t *allowed);

#endif
