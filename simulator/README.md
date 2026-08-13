# Guardian F401 Device Simulator

The simulator implements M9 supervisory-control commands.

```text
python tools/run_simulator.py
python tools/guardianctl.py baseline start --samples 64
python tools/guardianctl.py control arm
python tools/guardianctl.py control status
```

The simulator interlock is closed and its logical output boundary is available by default.

ARM remains safe-off because local run request is false.

Focused tests can change local run request and interlock through the simulator model without exposing those operations as host protocol commands.
