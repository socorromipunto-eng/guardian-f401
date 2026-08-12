/* Include the embedded middleware under test. */
#include "guardian_embedded_link.h"

/* Include assertion support. */
#include <assert.h>

/* Include fixed-width integer types. */
#include <stdint.h>

/* Include standard output. */
#include <stdio.h>

/* Include memory utilities. */
#include <string.h>

/* Define bounded fake byte storage. */
#define TEST_QUEUE_CAPACITY ((size_t)1024U)

/* Store deterministic fake transport state. */
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

    /* Store fake uptime. */
    uint32_t uptime_seconds;
} test_transport_t;

/* Return one fake RX byte without blocking. */
static int test_read_byte(void *context, uint8_t *byte)
{
    /* Recover fake transport state. */
    test_transport_t *transport =
        (test_transport_t *)context;

    /* Reject invalid storage. */
    if ((transport == NULL) || (byte == NULL))
    {
        /* Report no byte. */
        return 0;
    }

    /* Report an empty queue. */
    if (transport->rx_index >= transport->rx_size)
    {
        /* Return without blocking. */
        return 0;
    }

    /* Copy the next byte. */
    *byte = transport->rx[transport->rx_index];

    /* Advance the RX index. */
    transport->rx_index += 1U;

    /* Report one byte. */
    return 1;
}

/* Capture one complete response atomically. */
static size_t test_write(
    void *context,
    const uint8_t *data,
    size_t length)
{
    /* Recover fake transport state. */
    test_transport_t *transport =
        (test_transport_t *)context;

    /* Reject invalid storage. */
    if ((transport == NULL) ||
        ((data == NULL) && (length != 0U)))
    {
        /* Reject the write. */
        return 0U;
    }

    /* Reject overflow. */
    if ((transport->tx_size + length) > sizeof(transport->tx))
    {
        /* Reject the complete frame. */
        return 0U;
    }

    /* Copy accepted bytes. */
    (void)memcpy(
        &transport->tx[transport->tx_size],
        data,
        length);

    /* Publish TX byte count. */
    transport->tx_size += length;

    /* Report full acceptance. */
    return length;
}

/* Return deterministic fake uptime. */
static uint32_t test_uptime_seconds(void *context)
{
    /* Recover fake transport state. */
    test_transport_t *transport =
        (test_transport_t *)context;

    /* Reject missing state. */
    if (transport == NULL)
    {
        /* Return zero uptime. */
        return 0U;
    }

    /* Return configured uptime. */
    return transport->uptime_seconds;
}

/* Encode one host request into fake RX storage. */
static void test_queue_request(
    test_transport_t *transport,
    uint8_t command,
    uint32_t sequence)
{
    /* Create deterministic request storage. */
    guardian_frame_t request = {0};

    /* Store encoded size. */
    size_t encoded_size = 0U;

    /* Configure request type. */
    request.message_type = GUARDIAN_MESSAGE_REQUEST;

    /* Configure command. */
    request.command = command;

    /* Configure flags. */
    request.flags = GUARDIAN_SUPPORTED_FLAGS;

    /* Configure sequence. */
    request.sequence = sequence;

    /* Configure empty payload. */
    request.payload_length = 0U;

    /* Encode into fake RX storage. */
    assert(
        guardian_protocol_encode(
            &request,
            &transport->rx[transport->rx_size],
            sizeof(transport->rx) - transport->rx_size,
            &encoded_size) == GUARDIAN_PROTOCOL_OK);

    /* Publish queued bytes. */
    transport->rx_size += encoded_size;
}

/* Decode the captured response. */
static guardian_frame_t test_decode_response(
    test_transport_t *transport)
{
    /* Create response storage. */
    guardian_frame_t response = {0};

    /* Decode captured TX bytes. */
    assert(
        guardian_protocol_decode(
            transport->tx,
            transport->tx_size,
            &response) == GUARDIAN_PROTOCOL_OK);

    /* Return the response. */
    return response;
}

/* Initialize middleware over the fake transport. */
static guardian_embedded_link_t test_link_init(
    test_transport_t *transport)
{
    /* Create link storage. */
    guardian_embedded_link_t link = {0};

    /* Create callback configuration. */
    guardian_embedded_io_t io = {0};

    /* Create public identity. */
    guardian_device_identity_t identity = {0};

    /* Connect RX callback. */
    io.read_byte = test_read_byte;

    /* Connect TX callback. */
    io.write = test_write;

    /* Connect uptime callback. */
    io.uptime_seconds = test_uptime_seconds;

    /* Connect callback context. */
    io.context = transport;

    /* Configure model. */
    identity.model = "Guardian-F401-TEST";

    /* Configure firmware major. */
    identity.firmware_major = 0U;

    /* Configure firmware minor. */
    identity.firmware_minor = 4U;

    /* Configure firmware patch. */
    identity.firmware_patch = 0U;

    /* Configure device ID. */
    identity.device_id = 0x12345678UL;

    /* Initialize middleware. */
    assert(
        guardian_embedded_link_init(
            &link,
            &io,
            &identity) == GUARDIAN_PROTOCOL_OK);

    /* Return initialized middleware. */
    return link;
}

/* Verify PING through the embedded middleware. */
static void test_embedded_ping(void)
{
    /* Create fake transport. */
    test_transport_t transport = {0};

    /* Initialize middleware. */
    guardian_embedded_link_t link =
        test_link_init(&transport);

    /* Queue PING. */
    test_queue_request(
        &transport,
        (uint8_t)GUARDIAN_COMMAND_PING,
        77U);

    /* Process all fake bytes. */
    guardian_embedded_link_poll(
        &link,
        TEST_QUEUE_CAPACITY);

    /* Decode the response. */
    guardian_frame_t response =
        test_decode_response(&transport);

    /* Require PONG. */
    assert(response.message_type == GUARDIAN_MESSAGE_RESPONSE);

    /* Require command correlation. */
    assert(response.command == (uint8_t)GUARDIAN_COMMAND_PING);

    /* Require sequence correlation. */
    assert(response.sequence == 77U);

    /* Require PONG length. */
    assert(response.payload_length == 4U);

    /* Require PONG bytes. */
    assert(memcmp(response.payload, "PONG", 4U) == 0);
}

/* Verify DEVICE_INFO schema compatibility. */
static void test_embedded_device_info(void)
{
    /* Create fake transport. */
    test_transport_t transport = {0};

    /* Initialize middleware. */
    guardian_embedded_link_t link =
        test_link_init(&transport);

    /* Queue DEVICE_INFO. */
    test_queue_request(
        &transport,
        (uint8_t)GUARDIAN_COMMAND_DEVICE_INFO,
        78U);

    /* Process all fake bytes. */
    guardian_embedded_link_poll(
        &link,
        TEST_QUEUE_CAPACITY);

    /* Decode the response. */
    guardian_frame_t response =
        test_decode_response(&transport);

    /* Require schema v1. */
    assert(response.payload[0] == 1U);

    /* Require firmware 0.4.0. */
    assert(response.payload[1] == 0U);

    /* Require firmware 0.4.0. */
    assert(response.payload[2] == 4U);

    /* Require firmware 0.4.0. */
    assert(response.payload[3] == 0U);

    /* Require big-endian device ID. */
    assert(response.payload[4] == 0x12U);

    /* Require big-endian device ID. */
    assert(response.payload[5] == 0x34U);

    /* Require big-endian device ID. */
    assert(response.payload[6] == 0x56U);

    /* Require big-endian device ID. */
    assert(response.payload[7] == 0x78U);
}

/* Verify GET_STATUS counters. */
static void test_embedded_status(void)
{
    /* Create fake transport. */
    test_transport_t transport = {0};

    /* Configure uptime. */
    transport.uptime_seconds = 123U;

    /* Initialize middleware. */
    guardian_embedded_link_t link =
        test_link_init(&transport);

    /* Queue GET_STATUS. */
    test_queue_request(
        &transport,
        (uint8_t)GUARDIAN_COMMAND_GET_STATUS,
        79U);

    /* Process all fake bytes. */
    guardian_embedded_link_poll(
        &link,
        TEST_QUEUE_CAPACITY);

    /* Decode the response. */
    guardian_frame_t response =
        test_decode_response(&transport);

    /* Require schema v1. */
    assert(response.payload[0] == 1U);

    /* Require IDLE. */
    assert(response.payload[1] == (uint8_t)GUARDIAN_DEVICE_STATE_IDLE);

    /* Require uptime low byte. */
    assert(response.payload[5] == 123U);

    /* Require one received frame. */
    assert(response.payload[9] == 1U);

    /* Require zero previous TX frames. */
    assert(response.payload[13] == 0U);
}

/* Verify unknown commands fail closed. */
static void test_embedded_unknown_command(void)
{
    /* Create fake transport. */
    test_transport_t transport = {0};

    /* Initialize middleware. */
    guardian_embedded_link_t link =
        test_link_init(&transport);

    /* Queue unknown command. */
    test_queue_request(
        &transport,
        0xFEU,
        80U);

    /* Process all fake bytes. */
    guardian_embedded_link_poll(
        &link,
        TEST_QUEUE_CAPACITY);

    /* Decode the response. */
    guardian_frame_t response =
        test_decode_response(&transport);

    /* Require ERROR message. */
    assert(response.message_type == GUARDIAN_MESSAGE_ERROR);

    /* Require one-byte error payload. */
    assert(response.payload_length == 1U);

    /* Require UNKNOWN_COMMAND. */
    assert(response.payload[0] == (uint8_t)GUARDIAN_ERROR_UNKNOWN_COMMAND);
}

/* Execute all portable M4 middleware tests. */
int main(void)
{
    /* Verify PING. */
    test_embedded_ping();

    /* Verify device metadata. */
    test_embedded_device_info();

    /* Verify runtime status. */
    test_embedded_status();

    /* Verify unknown-command rejection. */
    test_embedded_unknown_command();

    /* Print one concise success message. */
    (void)printf("Guardian M4 embedded link host tests: PASS\n");

    /* Return process success. */
    return 0;
}
