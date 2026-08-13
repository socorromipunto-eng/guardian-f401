/* Include the complete embedded middleware under integration test. */
#include "guardian_embedded_link.h"

/* Include assertion support. */
#include <assert.h>

/* Include standard output for one concise CI success line. */
#include <stdio.h>

/* Include memory utilities for deterministic fake transport queues. */
#include <string.h>

/* Define bounded fake transport storage. */
#define TEST_QUEUE_CAPACITY ((size_t)1024U)

/* Store deterministic fake transport and output state. */
typedef struct
{
    /* Store host-to-device bytes. */
    uint8_t rx[TEST_QUEUE_CAPACITY];

    /* Store valid RX byte count. */
    size_t rx_size;

    /* Store next RX byte index. */
    size_t rx_index;

    /* Store device-to-host bytes. */
    uint8_t tx[TEST_QUEUE_CAPACITY];

    /* Store valid TX byte count. */
    size_t tx_size;

    /* Store fake monotonic uptime. */
    uint32_t uptime_seconds;

    /* Store the last logical run permit applied by M9. */
    uint8_t run_permit;
} test_context_t;

/* Return one fake RX byte without blocking. */
static int test_read_byte(
    void *context,
    uint8_t *byte)
{
    /* Recover fake transport state. */
    test_context_t *test =
        (test_context_t *)context;

    /* Reject invalid storage. */
    if ((test == NULL) ||
        (byte == NULL) ||
        (test->rx_index >= test->rx_size))
    {
        /* Report no available byte. */
        return 0;
    }

    /* Copy the next queued byte. */
    *byte =
        test->rx[
            test->rx_index
        ];

    /* Advance queue ownership. */
    test->rx_index += 1U;

    /* Report one byte. */
    return 1;
}

/* Capture one complete device TX block atomically. */
static size_t test_write(
    void *context,
    const uint8_t *data,
    size_t length)
{
    /* Recover fake transport state. */
    test_context_t *test =
        (test_context_t *)context;

    /* Reject invalid storage or overflow. */
    if ((test == NULL) ||
        ((data == NULL) && (length != 0U)) ||
        ((test->tx_size + length) >
         sizeof(test->tx)))
    {
        /* Reject the complete write. */
        return 0U;
    }

    /* Copy accepted bytes into the fake TX queue. */
    (void)memcpy(
        &test->tx[
            test->tx_size
        ],
        data,
        length);

    /* Publish the new TX byte count. */
    test->tx_size +=
        length;

    /* Report full atomic acceptance. */
    return length;
}

/* Return deterministic fake uptime. */
static uint32_t test_uptime(
    void *context)
{
    /* Recover fake state. */
    test_context_t *test =
        (test_context_t *)context;

    /* Return zero for missing state. */
    if (test == NULL)
    {
        /* Publish deterministic zero uptime. */
        return 0U;
    }

    /* Return configured uptime. */
    return test->uptime_seconds;
}

/* Apply one logical M9 output into the fake application boundary. */
static int test_apply_output(
    void *context,
    uint8_t run_permit)
{
    /* Recover fake state. */
    test_context_t *test =
        (test_context_t *)context;

    /* Reject missing output storage. */
    if (test == NULL)
    {
        /* Report output failure. */
        return 0;
    }

    /* Normalize and store the logical permit. */
    test->run_permit =
        (run_permit != 0U)
        ? 1U
        : 0U;

    /* Report successful application. */
    return 1;
}

/* Initialize the complete link with fake transport and output boundaries. */
static guardian_embedded_link_t test_link_init(
    test_context_t *test)
{
    /* Create link storage. */
    guardian_embedded_link_t link = {0};

    /* Create transport callback configuration. */
    guardian_embedded_io_t io = {0};

    /* Create immutable public identity. */
    guardian_device_identity_t identity = {0};

    /* Create M9 output configuration. */
    guardian_control_output_t output = {0};

    /* Connect fake RX. */
    io.read_byte =
        test_read_byte;

    /* Connect fake TX. */
    io.write =
        test_write;

    /* Connect fake uptime. */
    io.uptime_seconds =
        test_uptime;

    /* Share the fake test context. */
    io.context =
        test;

    /* Publish one deterministic model name. */
    identity.model =
        "Guardian-F401-M9-TEST";

    /* Publish test firmware version. */
    identity.firmware_major = 0U;

    /* Publish M9 minor version. */
    identity.firmware_minor = 9U;

    /* Publish test patch version. */
    identity.firmware_patch = 0U;

    /* Publish deterministic device ID. */
    identity.device_id =
        0xAABBCCDDUL;

    /* Initialize the complete transport-independent link. */
    assert(
        guardian_embedded_link_init(
            &link,
            &io,
            &identity) ==
        GUARDIAN_PROTOCOL_OK);

    /* Connect the fake M9 output callback. */
    output.apply =
        test_apply_output;

    /* Share the same fake context. */
    output.context =
        test;

    /* Require immediate safe-off output configuration. */
    assert(
        guardian_embedded_link_configure_control_output(
            &link,
            &output) ==
        GUARDIAN_CONTROL_OK);

    /* Prove the local interlock closed for this integration test. */
    guardian_embedded_link_set_interlock(
        &link,
        1U);

    /* Return the initialized middleware by value. */
    return link;
}

/* Return one valid stable M7 feature vector for baseline learning. */
static guardian_dsp_features_t test_features(
    uint32_t sequence)
{
    /* Create deterministic feature storage. */
    guardian_dsp_features_t features = {0};

    /* Publish the block sequence. */
    features.block_sequence =
        sequence;

    /* Publish the M6 reference sample rate. */
    features.sample_rate_hz =
        4000U;

    /* Publish stable healthy vibration RMS. */
    features.rms_mg =
        40U;

    /* Publish stable peak. */
    features.peak_mg =
        60U;

    /* Publish stable crest factor. */
    features.crest_factor_milli =
        1500U;

    /* Publish stable dominant frequency. */
    features.dominant_frequency_centi_hz =
        25000UL;

    /* Publish stable dominant peak. */
    features.dominant_peak_mg =
        58U;

    /* Publish stable spectral centroid. */
    features.spectral_centroid_centi_hz =
        40000UL;

    /* Publish stable spectral bands. */
    features.low_band_permille =
        800U;

    /* Publish stable spectral bands. */
    features.mid_band_permille =
        150U;

    /* Publish stable spectral bands. */
    features.high_band_permille =
        50U;

    /* Mark acquisition valid with reference calibration. */
    features.acquisition_status_flags =
        GUARDIAN_ACQUISITION_STATUS_VALID |
        GUARDIAN_ACQUISITION_STATUS_DEFAULT_CALIBRATION;

    /* Mark DSP analysis valid. */
    features.dsp_status_flags =
        GUARDIAN_DSP_STATUS_VALID |
        GUARDIAN_DSP_STATUS_DC_REMOVED |
        GUARDIAN_DSP_STATUS_HANN_WINDOW |
        GUARDIAN_DSP_STATUS_DOMINANT_VALID;

    /* Return the complete feature vector. */
    return features;
}

/* Queue one CONTROL_COMMAND request into fake RX storage. */
static void test_queue_control_action(
    test_context_t *test,
    guardian_control_action_t action,
    uint32_t sequence)
{
    /* Create deterministic request storage. */
    guardian_frame_t request = {0};

    /* Store encoded byte count. */
    size_t encoded_size = 0U;

    /* Configure host request message class. */
    request.message_type =
        GUARDIAN_MESSAGE_REQUEST;

    /* Select the M9 command. */
    request.command =
        (uint8_t)GUARDIAN_COMMAND_CONTROL_COMMAND;

    /* Use protocol v0.1 flags. */
    request.flags =
        GUARDIAN_SUPPORTED_FLAGS;

    /* Configure correlation sequence. */
    request.sequence =
        sequence;

    /* Publish the exact M9 request payload length. */
    request.payload_length =
        GUARDIAN_CONTROL_COMMAND_REQUEST_SIZE;

    /* Publish schema revision one. */
    request.payload[0] =
        GUARDIAN_CONTROL_SCHEMA_VERSION;

    /* Publish the selected action. */
    request.payload[1] =
        (uint8_t)action;

    /* Encode the complete request into fake RX storage. */
    assert(
        guardian_protocol_encode(
            &request,
            test->rx,
            sizeof(test->rx),
            &encoded_size) ==
        GUARDIAN_PROTOCOL_OK);

    /* Publish queued RX size. */
    test->rx_size =
        encoded_size;

    /* Reset RX ownership to the first byte. */
    test->rx_index =
        0U;
}

/* Decode the single captured response. */
static guardian_frame_t test_decode_response(
    test_context_t *test)
{
    /* Create response storage. */
    guardian_frame_t response = {0};

    /* Decode the complete fake TX frame. */
    assert(
        guardian_protocol_decode(
            test->tx,
            test->tx_size,
            &response) ==
        GUARDIAN_PROTOCOL_OK);

    /* Return the decoded response by value. */
    return response;
}

/* Verify M8 baseline, host ARM, local run, state ownership and interlock fault. */
static void test_m9_embedded_link_flow(void)
{
    /* Create fake transport and output state. */
    test_context_t test = {0};

    /* Initialize complete embedded middleware. */
    guardian_embedded_link_t link =
        test_link_init(
            &test);

    /* Start the minimum explicit M8 baseline directly for focused M9 integration. */
    assert(
        guardian_health_start_baseline(
            &link.health,
            GUARDIAN_HEALTH_BASELINE_MIN_SAMPLES) == 1);

    /* Feed exactly sixteen healthy M7 vectors through the public embedded-link DSP boundary. */
    uint16_t sample = 0U;

    /* Learn the complete baseline. */
    for (sample = 0U;
         sample < GUARDIAN_HEALTH_BASELINE_MIN_SAMPLES;
         ++sample)
    {
        /* Build one healthy feature snapshot. */
        guardian_dsp_features_t features =
            test_features(
                (uint32_t)sample + 1U);

        /* Update DSP, M8 health and M9 policy together. */
        guardian_embedded_link_update_dsp(
            &link,
            &features);
    }

    /* Require trained M8 readiness. */
    assert(
        guardian_embedded_link_health_status(
            &link
        ).state ==
        GUARDIAN_HEALTH_STATE_READY);

    /* Queue host ARM. */
    test_queue_control_action(
        &test,
        GUARDIAN_CONTROL_ACTION_ARM,
        1000U);

    /* Process the complete request. */
    guardian_embedded_link_poll(
        &link,
        TEST_QUEUE_CAPACITY);

    /* Decode the ARM response. */
    guardian_frame_t response =
        test_decode_response(
            &test);

    /* Require successful ARM response. */
    assert(
        response.message_type ==
        GUARDIAN_MESSAGE_RESPONSE);

    /* Require resulting ARMED state. */
    assert(
        response.payload[2] ==
        (uint8_t)GUARDIAN_CONTROL_STATE_ARMED);

    /* Require host ARM to remain safe-off. */
    assert(
        response.payload[3] ==
        0U);

    /* Require legacy device state to remain IDLE while merely armed. */
    assert(
        link.state ==
        GUARDIAN_DEVICE_STATE_IDLE);

    /* Attempt a legacy manual RUNNING override while M9 is armed. */
    guardian_embedded_link_set_state(
        &link,
        GUARDIAN_DEVICE_STATE_RUNNING);

    /* Require M9 authority to reject the legacy override. */
    assert(
        link.state ==
        GUARDIAN_DEVICE_STATE_IDLE);

    /* Assert the local-only machine run request. */
    guardian_embedded_link_set_local_run_request(
        &link,
        1U);

    /* Require active logical permit. */
    assert(
        guardian_embedded_link_run_permit(
            &link) ==
        1U);

    /* Require fake application output to receive the permit. */
    assert(
        test.run_permit ==
        1U);

    /* Require legacy device state to map to RUNNING. */
    assert(
        link.state ==
        GUARDIAN_DEVICE_STATE_RUNNING);

    /* Attempt to overwrite active M9 state manually. */
    guardian_embedded_link_set_state(
        &link,
        GUARDIAN_DEVICE_STATE_IDLE);

    /* Require the manual override to be ignored. */
    assert(
        link.state ==
        GUARDIAN_DEVICE_STATE_RUNNING);

    /* Open the local interlock. */
    guardian_embedded_link_set_interlock(
        &link,
        0U);

    /* Require immediate safe-off. */
    assert(
        guardian_embedded_link_run_permit(
            &link) ==
        0U);

    /* Require fake output safe-off. */
    assert(
        test.run_permit ==
        0U);

    /* Require legacy device state to map to FAULT. */
    assert(
        link.state ==
        GUARDIAN_DEVICE_STATE_FAULT);

    /* Require explicit M9 fault-latched state. */
    assert(
        guardian_embedded_link_control_status(
            &link
        ).state ==
        GUARDIAN_CONTROL_STATE_FAULT_LATCHED);
}

/* Execute the M9 embedded-link integration suite. */
int main(void)
{
    /* Verify the complete supervisory integration flow. */
    test_m9_embedded_link_flow();

    /* Print one concise success line for local and CI logs. */
    (void)printf("Guardian M9 control link host tests: PASS\n");

    /* Return conventional successful process status. */
    return 0;
}
