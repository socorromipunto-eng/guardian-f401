# guardianctl

M8 adds explicit machine-health baseline lifecycle and status commands.

```text
python tools/guardianctl.py baseline start --samples 64
python tools/guardianctl.py health
python tools/guardianctl.py --json health
python tools/guardianctl.py baseline reset
```

The same commands use TCP simulator transport or physical serial transport.
