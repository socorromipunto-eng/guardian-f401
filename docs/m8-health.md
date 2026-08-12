# M8 — Machine Health Baseline and Anomaly Detection

## Objective

M8 converts M7 DSP features into a runtime machine-health model.

```text
M6 acquisition
      |
      v
M7 DSP features
      |
      v
quality validation
      |
      v
explicit healthy baseline
      |
      v
mean + variance per feature
      |
      v
weighted normalized deviation
      |
      v
anomaly score + hysteresis
      |
      v
GET_HEALTH_STATUS
```

## Explicit Baseline

M8 never assumes that power-on means healthy operation.

The operator should capture a baseline under a known-good and reasonably stable speed/load regime. Frequency-domain features can legitimately move when machine operating conditions change.

Baseline learning starts only after:

```text
guardianctl baseline start --samples 64
```

The accepted target range is:

```text
16 .. 1024 feature blocks
```

At the M6 reference 4 kHz sample rate and 64 samples per DSP block, one feature block represents 16 ms of acquired vibration data.

A 64-block baseline therefore represents about 1.024 seconds of accepted signal blocks, excluding rejected blocks.

The runtime baseline is not persisted across reset in M8.

## Modeled Features

M8 models seven M7 values independently:

```text
0 RMS vibration
1 crest factor
2 dominant frequency
3 spectral centroid
4 low-band energy share
5 mid-band energy share
6 high-band energy share
```

The baseline uses Welford online mean and variance updates.

No dynamic allocation is used.

## Frozen Baseline

After the requested sample target is reached, baseline statistics stop adapting.

This is deliberate.

A model that automatically adapts during a fault can slowly redefine abnormal behavior as normal.

Future controlled adaptation can be added only with explicit operating-state policy.

## Variance Floors

Perfectly stable training data can produce zero variance.

M8 therefore applies minimum standard-deviation floors:

```text
RMS                  5 mg
crest factor         0.050
dominant frequency   5 Hz
spectral centroid    5 Hz
band share           10 permille
```

These floors prevent divide-by-zero behavior and reduce hypersensitivity to quantization.

## Scoring

For each feature:

```text
distance = abs(current - mean) / max(stddev, floor)
weighted_distance = distance * feature_weight
```

The largest weighted distance drives the bounded anomaly score.

```text
0 weighted sigma -> anomaly score 0
6 weighted sigma -> anomaly score 1000
```

The health score is:

```text
health_score = 1000 - anomaly_score
```

This score is a deterministic engineering indicator.

It is not a probability of failure and is not a safety-certified machine-protection decision.

## State Machine

```text
UNTRAINED
    |
baseline start
    v
LEARNING
    |
target reached
    v
READY
   |
persistent deviation
   v
WARNING
   |
persistent severe deviation
   v
ALARM
```

Warning qualification:

```text
weighted deviation >= 3.0
3 persistent blocks
```

Alarm qualification:

```text
weighted deviation >= 6.0
2 persistent severe blocks
```

Recovery requires:

```text
weighted deviation < 2.0
5 persistent normal blocks
```

The gap between warning and recovery thresholds provides hysteresis.

## Input Quality

M8 rejects feature vectors when M6/M7 report:

```text
ADC saturation
DMA block loss
ADC error
DMA error
invalid DSP result
sample-rate mismatch
```

Rejected data does not update the baseline or anomaly score.

`DEFAULT_CALIBRATION` is not rejected because software demos and early hardware bring-up need to function, but M8 preserves a quality flag showing that the learned baseline used reference external-sensor calibration.

## Host Commands

Start baseline:

```text
python tools/guardianctl.py baseline start --samples 64
```

Read model state:

```text
python tools/guardianctl.py health
```

Machine-readable output:

```text
python tools/guardianctl.py --json health
```

Reset runtime model:

```text
python tools/guardianctl.py baseline reset
```

## Simulator Behavior

The simulator fast-forwards the requested healthy baseline immediately.

This keeps software-only demonstrations short.

Real STM32 firmware learns only from actual successive M7 feature blocks.

## Keil Integration

Add the new portable M8 source:

```text
firmware/Health/Src/guardian_health.c
```

Add the include path:

```text
firmware/Health/Inc
```

M8 also updates:

```text
firmware/Platform/Src/guardian_embedded_link.c
firmware/App/Src/guardian_firmware_app.c
firmware/Protocol/Inc/guardian_protocol.h
```

The health implementation uses `sqrtf`, so the target must retain the normal C math-library support already required by the M7 DSP pipeline.

## Security and Safety Boundary

Baseline-control commands are unauthenticated in M8 because command authentication and authorization are planned for M10.

Do not expose an M8 device control channel to an untrusted network.

M8 health output is advisory machine-condition information.

M9 will use explicit supervisory-control policy and fault handling rather than treating one anomaly score as an actuator command.

## Next Milestone

M9 adds supervisory control:

```text
operating-state machine
fault latching
output policy
safe actuator abstraction
health-driven degraded/fault transitions
```
