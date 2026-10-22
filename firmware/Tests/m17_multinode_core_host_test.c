#include "guardian_multinode.h"

#include <stdio.h>

#define TEST_CHECK(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            return __LINE__; \
        } \
    } \
    while (0)

static guardian_multinode_node_t make_node(
    uint32_t node_id,
    uint32_t producer_id,
    uint32_t key_id,
    guardian_multinode_role_t role,
    guardian_multinode_authority_class_t authority,
    uint32_t capabilities)
{
    guardian_multinode_node_t node;

    node.node_id =
        node_id;

    node.producer_id =
        producer_id;

    node.key_id =
        key_id;

    node.role =
        role;

    node.authority_class =
        authority;

    node.capabilities =
        capabilities;

    return node;
}

int main(void)
{
    guardian_multinode_registry_t registry;

    uint8_t allowed = 0U;

    guardian_multinode_node_t f401 =
        make_node(
            1U,
            101U,
            1001U,
            GUARDIAN_NODE_ROLE_F401_REFERENCE_CONTROL,
            GUARDIAN_AUTHORITY_CLASS_REFERENCE_CONTROL,
            GUARDIAN_CAP_OBSERVATION |
            GUARDIAN_CAP_POLICY_REFERENCE |
            GUARDIAN_CAP_ACTUATION_GATE);

    guardian_multinode_node_t nxp =
        make_node(
            2U,
            102U,
            1002U,
            GUARDIAN_NODE_ROLE_MCXN947_SUPERVISORY_PEER,
            GUARDIAN_AUTHORITY_CLASS_OBSERVE,
            GUARDIAN_CAP_OBSERVATION |
            GUARDIAN_CAP_SUPERVISION |
            GUARDIAN_CAP_EVIDENCE_WITNESS |
            GUARDIAN_CAP_RECOVERY_ASSIST |
            GUARDIAN_CAP_SIGNATURE_VERIFY);

    guardian_multinode_node_t st2 =
        make_node(
            3U,
            103U,
            1003U,
            GUARDIAN_NODE_ROLE_ST_MEASUREMENT_REFERENCE,
            GUARDIAN_AUTHORITY_CLASS_OBSERVE,
            GUARDIAN_CAP_OBSERVATION |
            GUARDIAN_CAP_SENSOR_ACQUISITION);

    guardian_multinode_node_t witness =
        make_node(
            4U,
            104U,
            1004U,
            GUARDIAN_NODE_ROLE_SECURITY_WITNESS,
            GUARDIAN_AUTHORITY_CLASS_WITNESS,
            GUARDIAN_CAP_OBSERVATION |
            GUARDIAN_CAP_EVIDENCE_WITNESS |
            GUARDIAN_CAP_SIGNATURE_VERIFY);

    guardian_multinode_node_t compute =
        make_node(
            5U,
            105U,
            1005U,
            GUARDIAN_NODE_ROLE_HIGH_COMPUTE_PROCESSING,
            GUARDIAN_AUTHORITY_CLASS_OBSERVE,
            GUARDIAN_CAP_OBSERVATION |
            GUARDIAN_CAP_SIGNAL_PROCESSING);

    guardian_multinode_node_t malicious_compute =
        compute;

    guardian_multinode_registry_init(
        &registry);

    /*
     * PT01-PT05:
     * register the complete five-node topology.
     */
    TEST_CHECK(
        guardian_multinode_register_node(
            &registry,
            &f401) ==
        GUARDIAN_MULTINODE_OK);

    TEST_CHECK(
        guardian_multinode_register_node(
            &registry,
            &nxp) ==
        GUARDIAN_MULTINODE_OK);

    TEST_CHECK(
        guardian_multinode_register_node(
            &registry,
            &st2) ==
        GUARDIAN_MULTINODE_OK);

    TEST_CHECK(
        guardian_multinode_register_node(
            &registry,
            &witness) ==
        GUARDIAN_MULTINODE_OK);

    TEST_CHECK(
        guardian_multinode_register_node(
            &registry,
            &compute) ==
        GUARDIAN_MULTINODE_OK);

    /*
     * PT06:
     * all five required roles must exist.
     */
    TEST_CHECK(
        guardian_multinode_validate_complete_topology(
            &registry) ==
        GUARDIAN_MULTINODE_OK);

    /*
     * PT07:
     * F401 reference-control node may reach
     * the actuation-request boundary.
     */
    TEST_CHECK(
        guardian_multinode_can_request_actuation(
            &registry,
            1U,
            &allowed) ==
        GUARDIAN_MULTINODE_OK);

    TEST_CHECK(
        allowed == 1U);

    /*
     * NT01:
     * MCXN947 supervision never implies
     * physical actuation authority.
     */
    allowed = 1U;

    TEST_CHECK(
        guardian_multinode_can_request_actuation(
            &registry,
            2U,
            &allowed) ==
        GUARDIAN_MULTINODE_ERROR_NOT_REFERENCE_CONTROL);

    TEST_CHECK(
        allowed == 0U);

    /*
     * NT02:
     * witness status never implies
     * physical actuation authority.
     */
    allowed = 1U;

    TEST_CHECK(
        guardian_multinode_can_request_actuation(
            &registry,
            4U,
            &allowed) ==
        GUARDIAN_MULTINODE_ERROR_NOT_REFERENCE_CONTROL);

    TEST_CHECK(
        allowed == 0U);

    /*
     * NT03:
     * high-compute node cannot self-promote
     * into reference-control authority.
     */
    malicious_compute.authority_class =
        GUARDIAN_AUTHORITY_CLASS_REFERENCE_CONTROL;

    malicious_compute.capabilities |=
        GUARDIAN_CAP_ACTUATION_GATE;

    TEST_CHECK(
        guardian_multinode_validate_node(
            &malicious_compute) !=
        GUARDIAN_MULTINODE_OK);

    /*
     * NT04:
     * an incomplete topology cannot be
     * represented as the complete M17 fabric.
     */
    guardian_multinode_registry_init(
        &registry);

    TEST_CHECK(
        guardian_multinode_register_node(
            &registry,
            &f401) ==
        GUARDIAN_MULTINODE_OK);

    TEST_CHECK(
        guardian_multinode_register_node(
            &registry,
            &nxp) ==
        GUARDIAN_MULTINODE_OK);

    TEST_CHECK(
        guardian_multinode_validate_complete_topology(
            &registry) ==
        GUARDIAN_MULTINODE_ERROR_TOPOLOGY_INCOMPLETE);

    puts(
        "M17_MULTINODE_CORE_HOST_TEST=PASS");

    return 0;
}
