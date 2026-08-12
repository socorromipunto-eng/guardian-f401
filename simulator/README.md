# Guardian F401 Device Simulator

The simulator implements M8 baseline and machine-health commands.

Start the simulator:

```text
python tools/run_simulator.py
```

Then:

```text
python tools/guardianctl.py baseline start --samples 64
python tools/guardianctl.py health
```

The simulator fast-forwards deterministic healthy baseline samples immediately.

This behavior is for software-only protocol and UI demonstration.

Real STM32 firmware learns baseline samples from successive M7 acquisition blocks.
