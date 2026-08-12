# Firmware

M8 adds a transport-independent runtime machine-health model.

```text
guardian_dsp_features_t
        |
        v
guardian_health_ingest()
        |
        v
baseline / anomaly state
        |
        v
GET_HEALTH_STATUS
```

Baseline learning is explicit, bounded and frozen after completion.

No health-model work runs inside ADC, DMA, timer or UART interrupt context.

The runtime model is not persisted across reset in M8.

See `docs/m8-health.md`.
