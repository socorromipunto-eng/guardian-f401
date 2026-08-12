#ifndef GUARDIAN_EMBEDDED_LINK_H
#define GUARDIAN_EMBEDDED_LINK_H

/* Include the device command service. */
#include "guardian_device_service.h"

/* Include the incremental Guardian parser. */
#include "guardian_parser.h"

/* Include size_t for bounded poll budgets. */
#include <stddef.h>

/* Include fixed-width integer types. */
#include <stdint.h>

/* Define the default foreground RX byte budget. */
#define GUARDIAN_EMBEDDED_DEFAULT_RX_BUDGET ((size_t)64U)

/* Read one transport byte without blocking. */
typedef int (*guardian_embedded_read_byte_fn)(
    void *context,
    uint8_t *byte);

/* Queue one complete byte block for transmission. */
typedef size_t (*guardian_embedded_write_fn)(
    void *context,
    const uint8_t *data,
    size_t length);

/* Return monotonic uptime in whole seconds. */
typedef uint32_t (*guardian_embedded_uptime_fn)(
    void *context);

/* Store platform callbacks required by middleware. */
typedef struct
{
    /* Store the non-blocking RX callback. */
    guardian_embedded_read_byte_fn read_byte;

    /* Store the bounded TX callback. */
    guardian_embedded_write_fn write;

    /* Store the uptime callback. */
    guardian_embedded_uptime_fn uptime_seconds;

    /* Store opaque platform state. */
    void *context;
} guardian_embedded_io_t;

/* Store communication diagnostics. */
typedef struct
{
    /* Count validated RX frames. */
    uint32_t rx_frames;

    /* Count accepted TX frames. */
    uint32_t tx_frames;

    /* Count protocol/transport failures. */
    uint32_t protocol_errors;

    /* Count responses rejected by the TX queue. */
    uint32_t tx_queue_failures;

    /* Store the latest Guardian error. */
    uint8_t last_error;
} guardian_embedded_link_stats_t;

/* Store the complete embedded protocol middleware. */
typedef struct
{
    /* Store incremental parser state. */
    guardian_parser_t parser;

    /* Store device service configuration. */
    guardian_device_service_t service;

    /* Store platform callbacks. */
    guardian_embedded_io_t io;

    /* Store published application state. */
    guardian_device_state_t state;

    /* Store cumulative diagnostics. */
    guardian_embedded_link_stats_t stats;
} guardian_embedded_link_t;

/* Initialize middleware and public identity. */
guardian_protocol_result_t guardian_embedded_link_init(
    guardian_embedded_link_t *link,
    const guardian_embedded_io_t *io,
    const guardian_device_identity_t *identity);

/* Change the published application state. */
void guardian_embedded_link_set_state(
    guardian_embedded_link_t *link,
    guardian_device_state_t state);

/* Process bounded foreground RX work without blocking. */
void guardian_embedded_link_poll(
    guardian_embedded_link_t *link,
    size_t rx_budget);

/* Return a copy of middleware diagnostics. */
guardian_embedded_link_stats_t guardian_embedded_link_stats(
    const guardian_embedded_link_t *link);

#endif
