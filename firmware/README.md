# Firmware

M7 adds portable foreground DSP on the calibrated M6 vibration block.

```text
64 signed vibration samples in mg
        |
        v
guardian_dsp_analyze()
        |
        v
RMS / peak / crest / FFT / spectral bands
        |
        v
GET_DSP_FEATURES
```

No DSP work runs inside ADC, DMA, timer or UART interrupt handlers.

The target still requires a Keil build and physical timing validation.

See `docs/m7-dsp.md`.
