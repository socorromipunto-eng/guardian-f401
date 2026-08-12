/* Include the M5 transport-independent embedded middleware under test. */
#include "guardian_embedded_link.h"

/* Include assertion support for deterministic host-side verification. */
#include <assert.h>

/* Include fixed-width integer types used by fake transport state. */
#include <stdint.h>

/* Include standard output for one concise CI success message. */
#include <stdio.h>

/* Include memory utilities for fake byte queues. */
#include <string.h>

/* Define bounded fake transport storage. */
#define TEST_QUEUE_CAPACITY ((size_t)1024U)

/* Store deterministic fake byte transport state. */
typedef struct
{
    /* Store host-to-device request bytes. */
    uint8_t rx[TEST_QUEUE_CAPACITY];

    /* Store the number of valid RX bytes. */
    size_t rx_size;

    /* Store the next RX byte returned to middleware. */
    size_t rx_index;

    /* Store device-to-host output bytes. */
    uint8_t tx[TEST_QUEUE_CAPACITY];

    /* Store the number of valid captured TX bytes. */
    size_t tx_size;

    /* Store deterministic whole-second uptime. */
    uint32_t uptime_seconds;
} test_transport_t;

/* Return one fake RX byte without blocking. */
static int test_read_byte(
    void *context,
    uint8_t *byte)
{
    /* Recover fake transport state. */
    test_transport_t *transport =
        (test_transport_t *)context;

    /* Reject invalid callback storage. */
    if ((transport == NULL) || (byte == NULL))
    {
        /* Report no byte. */
        return 0;
    }

    /* Report an empty RX queue. */
    if (transport->rx_index >= transport->rx_size)
    {
        /* Return without blocking. */
        return 0;
    }

    /* Copy the next queued request byte. */
    *byte = transport->rx[transport->rx_index];

    /* Advance the fake RX consumer index. */
    transport->rx_index += 1U;

    /* Report exactly one produced byte. */
    return 1;
}

/* Capture one complete encoded output frame atomically. */
static size_t test_write(
    void *context,
    const uint8_t *data,
    size_t length)
{
    /* Recover fake transport state. */
    test_transport_t *transport =
        (test_transport_t *)context;

    /* Reject invalid callback storage. */
    if ((transport == NULL) ||
        ((data == NULL) && (length != 0U)))
    {
        /* Report complete rejection. */
        return 0U;
    }

    /* Reject output that would exceed bounded fake storage. */
    if ((transport->tx_size + length) >
        sizeof(transport->tx))
    {
        /* Report complete rejection. */
        return 0U;
    }

    /* Copy every accepted output byte. */
    (void)memcpy(
        &transport->tx[transport->tx_size],
        data,
        length);

    /* Publish the increased captured byte count. */
    transport->tx_size += length;

    /* Report atomic acceptance of the complete frame. */
    return length;
}

/* Return deterministic fake uptime. */
static uint32_t test_uptime_seconds(
    void *context)
{
    /* Recover fake transport state. */
    test_transport_t *transport =
        (test_transport_t *)context;

    /* Reject missing state defensively. */
    if (transport == NULL)
    {
        /* Return deterministic zero uptime. */
        return 0U;
    }

    /* Return configured fake uptime. */
    return transport->uptime_seconds;
}

/* Reset fake transport byte queues without changing device middleware state. */
static void test_clear_transport(
    test_transport_t *transport)
{
    /* Reset queued host request bytes. */
    transport->rx_size = 0U;

    /* Reset host request consumer position. */
    transport->rx_index = 0U;

    /* Reset captured device output bytes. */
    transport->tx_size = 0U;
}

/* Initialize one embedded link using the deterministic fake transport. */
static guardian_embedded_link_t test_link_init(
    test_transport_t *transport)
{
    /* Create deterministic middleware storage. */
    guardian_embedded_link_t link = {0};

    /* Create deterministic callback configuration. */
    guardian_embedded_io_t io = {0};

    /* Create immutable test identity. */
    guardian_device_identity_t identity = {0};

    /* Connect fake RX byte callback. */
    io.read_byte = test_read_byte;

    /* Connect fake TX callback. */
    io.write = test_write;

    /* Connect deterministic fake uptime. */
    io.uptime_seconds = test_uptime_seconds;

    /* Pass fake transport state to every callback. */
    io.context = transport;

    /* Publish a deterministic hardware-style model name. */
    identity.model = "Guardian-F401-M5-TEST";

    /* Publish M5 firmware major version. */
    identity.firmware_major = 0U;

    /* Publish M5 firmware minor version. */
    identity.firmware_minor = 5U;

    /* Publish M5 firmware patch version. */
    identity.firmware_patch = 0U;

    /* Publish deterministic test device identifier. */
    identity.device_id = 0x12345678UL;

    /* Initialize parser, command service, telemetry and callbacks. */
    assert(
        guardian_embedded_link_init(
            &link,
            &io,
            &identity) == GUARDIAN_PROTOCOL_OK);

    /* Return initialized middleware by value. */
    return link;
}

/* Queue one SET_TELEMETRY request into fake RX storage. */
static void test_queue_telemetry_config(
    test_transport_t *transport,
    uint8_t enabled,
    uint16_t period_ms,
    uint32_t sequence)
{
    /* Create deterministic request storage. */
    guardian_frame_t request = {0};

    /* Store exact encoded request length. */
    size_t encoded_size = 0U;

    /* Configure a host request frame. */
    request.message_type = GUARDIAN_MESSAGE_REQUEST;

    /* Configure the M5 telemetry control command. */
    request.command =
        (uint8_t)GUARDIAN_COMMAND_SET_TELEMETRY;

    /* Use the only flags value defined by protocol v0.1. */
    request.flags = GUARDIAN_SUPPORTED_FLAGS;

    /* Configure request correlation sequence. */
    request.sequence = sequence;

    /* Publish the fixed four-byte configuration payload. */
    request.payload_length = 4U;

    /* Encode telemetry schema v1. */
    request.payload[0] = GUARDIAN_TELEMETRY_SCHEMA_VERSION;

    /* Encode normalized enabled state supplied by the test. */
    request.payload[1] = enabled;

    /* Encode period most-significant byte. */
    request.payload[2] =
        (uint8_t)((period_ms >> 8U) & 0xFFU);

    /* Encode period least-significant byte. */
    request.payload[3] =
        (uint8_t)(period_ms & 0xFFU);

    /* Encode directly into unused fake RX storage. */
    assert(
        guardian_protocol_encode(
            &request,
            &transport->rx[transport->rx_size],
            sizeof(transport->rx) - transport->rx_size,
            &encoded_size) == GUARDIAN_PROTOCOL_OK);

    /* Publish newly queued fake RX bytes. */
    transport->rx_size += encoded_size;
}

/* Decode exactly one captured output frame. */
static guardian_frame_t test_decode_output(
    test_transport_t *transport)
{
    /* Create deterministic decoded frame storage. */
    guardian_frame_t frame = {0};

    /* Decode the complete captured frame. */
    assert(
        guardian_protocol_decode(
            transport->tx,
            transport->tx_size,
            &frame) == GUARDIAN_PROTOCOL_OK);

    /* Return the validated frame by value. */
    return frame;
}

/* Advance middleware telemetry scheduling by the requested milliseconds. */
static void test_tick_ms(
    guardian_embedded_link_t *link,
    uint32_t milliseconds)
{
    /* Track the current synthetic millisecond. */
    uint32_t index = 0U;

    /* Advance exactly the requested number of one-millisecond ticks. */
    for (index = 0U; index < milliseconds; ++index)
    {
        /* Advance the M5 scheduler and timestamp. */
        guardian_embedded_link_tick_1ms(link);
    }
}

/* Verify telemetry remains silent until explicitly enabled. */
static void test_telemetry_disabled_by_default(void)
{
    /* Create deterministic fake transport state. */
    test_transport_t transport = {0};

    /* Initialize middleware over the fake transport. */
    guardian_embedded_link_t link =
        test_link_init(&transport);

    /* Advance beyond the default one-second period. */
    test_tick_ms(
        &link,
        1500U);

    /* Poll with an empty RX queue. */
    guardian_embedded_link_poll(
        &link,
        TEST_QUEUE_CAPACITY);

    /* Require no asynchronous output while disabled. */
    assert(transport.tx_size == 0U);
}

/* Verify one configured telemetry frame is emitted after the exact period. */
static void test_telemetry_emits_due_measurement(void)
{
    /* Create deterministic fake transport state. */
    test_transport_t transport = {0};

    /* Initialize middleware over the fake transport. */
    guardian_embedded_link_t link =
        test_link_init(&transport);

    /* Create one application-provided measurement snapshot. */
    guardian_machine_measurements_t measurements = {0};

    /* Configure signed temperature. */
    measurements.temperature_centi_c = 2534;

    /* Configure RMS vibration. */
    measurements.vibration_mg_rms = 42U;

    /* Configure machine current. */
    measurements.current_ma = 850U;

    /* Configure shaft speed. */
    measurements.rpm = 1500U;

    /* Configure supply voltage. */
    measurements.supply_mv = 3290U;

    /* Configure application status flags. */
    measurements.status_flags = 0x0003U;

    /* Publish the latest application measurement snapshot. */
    guardian_embedded_link_update_telemetry(
        &link,
        &measurements);

    /* Queue telemetry enable at the minimum allowed period. */
    test_queue_telemetry_config(
        &transport,
        1U,
        100U,
        501U);

    /* Process the configuration request and response. */
    guardian_embedded_link_poll(
        &link,
        TEST_QUEUE_CAPACITY);

    /* Decode the configuration acknowledgement. */
    guardian_frame_t response =
        test_decode_output(&transport);

    /* Require successful synchronous configuration response. */
    assert(response.message_type == GUARDIAN_MESSAGE_RESPONSE);

    /* Require normalized telemetry enabled state. */
    assert(response.payload[1] == 1U);

    /* Clear configuration request/response transport bytes. */
    test_clear_transport(&transport);

    /* Advance only 99 milliseconds. */
    test_tick_ms(
        &link,
        99U);

    /* Poll before the sample is due. */
    guardian_embedded_link_poll(
        &link,
        TEST_QUEUE_CAPACITY);

    /* Require no early asynchronous sample. */
    assert(transport.tx_size == 0U);

    /* Advance the final required millisecond. */
    test_tick_ms(
        &link,
        1U);

    /* Poll after the exact configured period. */
    guardian_embedded_link_poll(
        &link,
        TEST_QUEUE_CAPACITY);

    /* Decode the asynchronous telemetry frame. */
    guardian_frame_t telemetry =
        test_decode_output(&transport);

    /* Require the asynchronous telemetry message class. */
    assert(
        telemetry.message_type ==
        GUARDIAN_MESSAGE_TELEMETRY);

    /* Require the dedicated machine telemetry channel. */
    assert(
        telemetry.command ==
        (uint8_t)GUARDIAN_COMMAND_MACHINE_TELEMETRY);

    /* Require the first independent telemetry sequence. */
    assert(telemetry.sequence == 1U);

    /* Require the frozen M5 payload size. */
    assert(telemetry.payload_length == 18U);

    /* Require schema v1. */
    assert(
        telemetry.payload[0] ==
        GUARDIAN_TELEMETRY_SCHEMA_VERSION);

    /* Require the current IDLE application state. */
    assert(
        telemetry.payload[1] ==
        (uint8_t)GUARDIAN_DEVICE_STATE_IDLE);

    /* Require the 100 ms timestamp low byte. */
    assert(telemetry.payload[5] == 100U);

    /* Require signed 25.34 C representation 0x09E6. */
    assert(telemetry.payload[6] == 0x09U);

    /* Require signed 25.34 C representation 0x09E6. */
    assert(telemetry.payload[7] == 0xE6U);

    /* Require 42 mg RMS vibration low byte. */
    assert(telemetry.payload[9] == 42U);

    /* Require the application status flags low byte. */
    assert(telemetry.payload[17] == 0x03U);
}

/* Verify telemetry configuration enforces the minimum period. */
static void test_telemetry_rejects_excessive_rate(void)
{
    /* Create deterministic fake transport state. */
    test_transport_t transport = {0};

    /* Initialize middleware over the fake transport. */
    guardian_embedded_link_t link =
        test_link_init(&transport);

    /* Queue a 99 ms out-of-policy telemetry request. */
    test_queue_telemetry_config(
        &transport,
        1U,
        99U,
        502U);

    /* Process the invalid configuration request. */
    guardian_embedded_link_poll(
        &link,
        TEST_QUEUE_CAPACITY);

    /* Decode the deterministic ERROR frame. */
    guardian_frame_t response =
        test_decode_output(&transport);

    /* Require explicit ERROR message semantics. */
    assert(response.message_type == GUARDIAN_MESSAGE_ERROR);

    /* Require the frozen one-byte error payload. */
    assert(response.payload_length == 1U);

    /* Require INVALID_PAYLOAD for the out-of-policy rate. */
    assert(
        response.payload[0] ==
        (uint8_t)GUARDIAN_ERROR_INVALID_PAYLOAD);
}

/* Execute every M5 portable telemetry middleware test. */
int main(void)
{
    /* Verify default telemetry silence. */
    test_telemetry_disabled_by_default();

    /* Verify exact-period asynchronous sample emission. */
    test_telemetry_emits_due_measurement();

    /* Verify command-channel rate limiting. */
    test_telemetry_rejects_excessive_rate();

    /* Print one concise success message for local and CI logs. */
    (void)printf("Guardian M5 telemetry host tests: PASS\n");

    /* Return the conventional successful process status. */
    return 0;
}
