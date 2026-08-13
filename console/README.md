# guardianctl

M9 adds supervisory-control status and safety-gated actions.

```text
python tools/guardianctl.py control status
python tools/guardianctl.py control arm
python tools/guardianctl.py control disarm
python tools/guardianctl.py control clear-fault
```

The protocol deliberately has no host RUN/START action in M9.

Use `baseline start` before ARM.
