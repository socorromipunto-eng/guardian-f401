# M7 — DSP / FFT / Spectral Features

M7 consumes each calibrated 64-sample vibration block produced by M6.

Pipeline:

```text
M6 calibrated vibration block
        |
        v
DC removal
        |
        v
time-domain RMS + peak + crest factor
        |
        v
64-point Hann window
        |
        v
portable radix-2 FFT
        |
        v
one-sided power spectrum
        |
        +--> dominant frequency + amplitude
        +--> spectral centroid
        `--> low/mid/high band energy
        |
        v
GET_DSP_FEATURES
        |
        v
guardianctl dsp
```

Reference sample rate: 4000 Hz.

FFT size: 64 samples.

Nominal bin spacing at 4 kHz:

```text
62.5 Hz
```

Bands:

```text
low  = 0 .. 500 Hz
mid  = >500 .. 1500 Hz
high = >1500 Hz .. Nyquist
```

The FFT implementation is portable C and uses static lookup tables. It does not allocate memory dynamically and runs only in foreground code.

The dominant frequency uses bounded three-point parabolic interpolation when neighboring bins are available.

The firmware preserves the M6 acquisition quality flags in each DSP snapshot.

Host command:

```text
python tools/guardianctl.py dsp
python tools/guardianctl.py --json dsp
```

The simulator returns deterministic synthetic DSP features so the complete host/protocol path can be tested without hardware.

Physical completion still requires Keil timing measurements on the real STM32F401CDU6. M8 will consume these features for baseline modeling and anomaly detection.
