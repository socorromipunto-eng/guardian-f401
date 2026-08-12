# guardianctl

`guardianctl` supports synchronous diagnostics and M5 asynchronous telemetry.

## Simulator

Start the software device:

```text
python tools/run_simulator.py
```

Stream ten live samples:

```text
python tools/guardianctl.py telemetry --period-ms 500 --count 10
```

Machine-readable JSON Lines:

```text
python tools/guardianctl.py --json telemetry --period-ms 250 --count 20
```

## Physical UART

Install the optional serial dependency:

```text
python -m pip install -r console/requirements-serial.txt
```

Then:

```text
python tools/guardianctl.py --serial-port COM5 telemetry --period-ms 500 --count 10
```

The command automatically:

```text
enable telemetry
      |
receive bounded sample count
      |
disable telemetry
```

The telemetry period is limited to 100–60000 ms.
