# guardianctl

`guardianctl` is the host-side diagnostics and management console for Guardian F401.

M4 supports both the M2 simulator and the physical STM32 UART command channel.

## Simulator

```text
python tools/run_simulator.py
python tools/guardianctl.py ping
python tools/guardianctl.py info
python tools/guardianctl.py status
```

## Physical STM32F401 UART

Install pyserial:

```text
python -m pip install -r console/requirements-serial.txt
```

Example Windows port:

```text
python tools/guardianctl.py --serial-port COM5 ping
python tools/guardianctl.py --serial-port COM5 info
python tools/guardianctl.py --serial-port COM5 status
```

The high-level client is transport-independent:

```text
GuardianClient
      |
      +--> GuardianTcpTransport
      |
      `--> GuardianSerialTransport
```
