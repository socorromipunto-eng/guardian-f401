# Firmware

M10 adds portable PSK authenticated sessions around privileged M8/M9 commands.

```text
AUTH_BEGIN
AUTH_FINISH
SECURE_COMMAND
GET_SECURITY_STATUS
```

Portable modules:

```text
firmware/Security/Src/guardian_crypto.c
firmware/Security/Src/guardian_security.c
```

The STM32 application requires secure wrapping for baseline/control commands by default.

No production PSK is stored in the repository.

Board/product integration must supply a high-entropy 32-byte PSK and cryptographically strong nonce callback.

See `docs/m10-security.md`.
