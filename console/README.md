# guardianctl

M10 adds optional authenticated sessions for privileged commands.

Configure credentials with:

```text
--psk-hex <64 hex characters>
--role operator
```

or:

```text
GUARDIAN_PSK_HEX
GUARDIAN_ROLE
```

Commands:

```text
python tools/guardianctl.py security status
python tools/guardianctl.py security authenticate
python tools/guardianctl.py baseline start --samples 64
python tools/guardianctl.py control arm
```

When a PSK is configured, baseline/control operations use `SECURE_COMMAND` automatically.
