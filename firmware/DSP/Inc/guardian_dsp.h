#ifndef GUARDIAN_DSP_H
#define GUARDIAN_DSP_H

/* Include the calibrated M6 signal block consumed by the DSP pipeline. */
#include "guardian_acquisition.h"

/* Include Guardian frame and protocol result types for GET_DSP_FEATURES. */
#include "guardian_protocol.h"

/* Include fixed-width integer types for deterministic feature fields. */
#include <stdint.h>

/* Define the fixed FFT length matched to one M6 DMA acquisition block. */
#define GUARDIAN_DSP_FFT_SIZE ((uint16_t)64U)

/* Define the number of unique bins in one real-valued one-sided spectrum including Nyquist. */
#define GUARDIAN_DSP_ONE_SIDED_BIN_COUNT ((uint16_t)33U)

/* Define the first DSP feature payload schema revision. */
#define GUARDIAN_DSP_SCHEMA_VERSION ((uint8_t)0x01U)

/* Define the exact GET_DSP_FEATURES response payload length. */
#define GUARDIAN_DSP_PAYLOAD_SIZE ((uint16_t)32U)

/* Mark one feature snapshot as valid. */
#define GUARDIAN_DSP_STATUS_VALID ((uint16_t)0x0001U)

/* Mark that the input mean was removed before spectral processing. */
#define GUARDIAN_DSP_STATUS_DC_REMOVED ((uint16_t)0x0002U)

/* Mark that a Hann window was applied before FFT processing. */
#define GUARDIAN_DSP_STATUS_HANN_WINDOW ((uint16_t)0x0004U)

/* Mark that a non-zero dominant spectral component was identified. */
#define GUARDIAN_DSP_STATUS_DOMINANT_VALID ((uint16_t)0x0008U)

/* Mark a signal block whose AC content was effectively silent. */
#define GUARDIAN_DSP_STATUS_SILENT ((uint16_t)0x0010U)

/* Define deterministic portable DSP outcomes. */
typedef enum
{
    /* Indicate successful feature extraction. */
    GUARDIAN_DSP_OK = 0,

    /* Indicate a missing required pointer. */
    GUARDIAN_DSP_ERROR_NULL_ARGUMENT,

    /* Indicate an unsupported signal-block length. */
    GUARDIAN_DSP_ERROR_LENGTH,

    /* Indicate an invalid zero sample rate. */
    GUARDIAN_DSP_ERROR_SAMPLE_RATE
} guardian_dsp_result_t;

/* Store one deterministic M7 feature snapshot. */
typedef struct
{
    /* Preserve the M6 acquisition block sequence used by this analysis. */
    uint32_t block_sequence;

    /* Preserve the physical vibration sample rate in hertz. */
    uint16_t sample_rate_hz;

    /* Store AC RMS vibration magnitude in milli-g. */
    uint16_t rms_mg;

    /* Store the largest absolute DC-removed sample in milli-g. */
    uint16_t peak_mg;

    /* Store crest factor multiplied by 1000. */
    uint16_t crest_factor_milli;

    /* Store interpolated dominant frequency in hundredths of one hertz. */
    uint32_t dominant_frequency_centi_hz;

    /* Store estimated dominant sinusoidal peak amplitude in milli-g. */
    uint16_t dominant_peak_mg;

    /* Store power-weighted spectral centroid in hundredths of one hertz. */
    uint32_t spectral_centroid_centi_hz;

    /* Store 0-500 Hz spectral energy share in permille. */
    uint16_t low_band_permille;

    /* Store >500-1500 Hz spectral energy share in permille. */
    uint16_t mid_band_permille;

    /* Store >1500 Hz through Nyquist spectral energy share in permille. */
    uint16_t high_band_permille;

    /* Preserve the acquisition quality flags associated with the analyzed block. */
    uint16_t acquisition_status_flags;

    /* Store DSP-specific processing and validity flags. */
    uint16_t dsp_status_flags;
} guardian_dsp_features_t;

/* Analyze one complete calibrated M6 vibration block. */
guardian_dsp_result_t guardian_dsp_analyze(
    const guardian_acquisition_signal_block_t *signal_block,
    guardian_dsp_features_t *features);

/* Encode one deterministic GET_DSP_FEATURES payload. */
guardian_protocol_result_t guardian_dsp_encode_payload(
    const guardian_dsp_features_t *features,
    uint8_t *payload,
    uint16_t payload_capacity,
    uint16_t *payload_length);

/* Process GET_DSP_FEATURES and build one correlated RESPONSE or ERROR frame. */
guardian_protocol_result_t guardian_dsp_handle_request(
    const guardian_dsp_features_t *features,
    uint8_t features_valid,
    const guardian_frame_t *request,
    guardian_frame_t *response);

#endif
