# M11 — Robustness, Fuzzing and Fault Injection

## Objective

M11 hardens the Guardian F401 software stack by continuously exercising malformed transport data, framing corruption, secure-envelope tampering and replay/counter faults.

The milestone is intentionally defensive.

```text
arbitrary bytes
     |
     v
Guardian parser
     |
     +--> CRC failure
     +--> oversized length
     +--> malformed frame
     `--> bounded resynchronization
              |
              v
       later canonical frame
```

M10 secure commands are exercised independently:

```text
valid authenticated command
          |
          +--> bit flip after MAC
          +--> tag corruption
          +--> length corruption
          +--> truncation
          +--> trailing bytes
          +--> exact replay
          `--> valid-MAC counter gap
                    |
                    v
              reject before
             unintended action
```

## Main Invariants

M11 treats these as release gates:

```text
parser never reads or writes beyond fixed bounds
parser remains recoverable after bounded malformed input
arbitrary parser bytes do not crash or hang
tampered secure envelopes never execute privileged commands
failed secure authentication does not consume the valid next counter
accepted secure requests execute at most once
exact replay is rejected
skipped strict counter is rejected
legacy M1-M10 regressions remain green
```

## Deterministic Python Campaign

Run:

```text
python tools/run_robustness.py
```

Reproduce one exact campaign:

```text
python tools/run_robustness.py --seed 0xC0FFEE11 --iterations 1000
```

The runner executes:

```text
frame mutation + parser recovery
secure-envelope after-MAC tampering
exact authenticated replay
valid-MAC skipped counter
```

Mutation classes:

```text
FLIP_BIT
REPLACE_BYTE
DELETE_BYTE
DUPLICATE_SLICE
TRUNCATE
INSERT_NOISE
POISON_LENGTH
CORRUPT_CRC
CORRUPT_MAGIC
```

Every case records a deterministic per-case seed.

## Parser Recovery Gate

A mutated stream is followed by one maximum-frame-sized zero flush and a canonical PING.

The flush bytes cannot begin Guardian magic.

Because Guardian frames are statically bounded, this completes or discards any pending bounded candidate before the recovery probe.

The campaign requires the canonical PING to be recovered afterward.

This verifies bounded recovery without assuming packet-aligned transport reads.

## Secure-Envelope Tamper Gate

Each case creates:

```text
fresh secure-mode simulator
fresh M10 OPERATOR session
UNTRAINED M8 state
valid counter-1 BASELINE_CONTROL envelope
```

The envelope is mutated only **after** the valid HMAC has been calculated.

The device must return:

```text
UNAUTHORIZED
```

and M8 must remain:

```text
UNTRAINED
baseline_samples = 0
```

This prevents a malformed authenticated envelope from partially reaching privileged application logic.

## Replay and Counter Fault Injection

A valid authenticated baseline command is executed exactly once.

Then M11 sends:

```text
the exact same authenticated request again
```

Expected result:

```text
REPLAY_DETECTED
```

The campaign then sends a newly authenticated request whose HMAC is valid but whose counter skips the expected value.

Expected result:

```text
REPLAY_DETECTED
```

Neither rejected request may change M8 state.

## Portable C Mutation Drivers

The deterministic C drivers are:

```text
fuzz/c/parser_mutation_driver.c
fuzz/c/security_mutation_driver.c
```

They are designed to run under:

```text
AddressSanitizer
UndefinedBehaviorSanitizer
```

Typical Linux commands are documented in `fuzz/README.md`.

The parser driver mutates valid frame bytes and checks internal parser bounds plus bounded recovery.

The security driver builds valid M10 secure envelopes, mutates them after HMAC creation, checks fail-closed behavior, then verifies the untouched original still succeeds once.

## libFuzzer Harnesses

Long-running developer fuzz targets are:

```text
fuzz/c/parser_libfuzzer.c
fuzz/c/security_libfuzzer.c
```

They use the production C parser and production M10 security decoder directly.

The committed corpus includes both malformed seeds and a valid authenticated secure-envelope seed.

The CI workflow runs bounded smoke campaigns.

Long-running fuzzing can use the same harnesses with a larger corpus and no `-runs` limit.

## Seed Corpus

Parser corpus:

```text
fuzz/corpus/parser/ping.bin
fuzz/corpus/parser/status.bin
fuzz/corpus/parser/oversize-header.bin
fuzz/corpus/parser/noise-magic.bin
```

Security corpus:

```text
fuzz/corpus/security/valid-baseline-secure.bin
fuzz/corpus/security/truncated-secure.bin
fuzz/corpus/security/malformed-prefix.bin
```

Binary corpus entries are marked `-text` in `.gitattributes` so Windows line-ending conversion cannot modify them.

## CI

`.github/workflows/robustness-tests.yml` runs:

```text
focused Python M11 tests
1000-case deterministic Python campaigns
10000-case parser mutation driver under ASan/UBSan
10000-case security mutation driver under ASan/UBSan
5000-run parser libFuzzer smoke campaign
5000-run security libFuzzer smoke campaign
```

The workflow uses the official `actions/checkout@v6` and `actions/setup-python@v6` actions.

## Reproduction and Triage

When a deterministic Python campaign fails, preserve:

```text
master seed
iteration
mutation kind
per-case seed
mutation description
```

Re-run using the same master seed first.

For a libFuzzer crash, preserve the generated crash artifact and run the target directly with that artifact.

A minimized crashing input belongs in the relevant committed corpus after the defect is fixed.

## What M11 Does Not Prove

Passing fuzz and sanitizer campaigns does not prove memory safety, functional safety or cryptographic correctness.

M11 increases defect-discovery coverage and gives reproducible regression gates.

The physical STM32 target still requires:

```text
Keil target build
UART electrical fault testing
DMA overrun/underrun testing
brownout/reset testing
sensor fault injection
watchdog validation
real timing/load validation
```

## Security Boundary

All fuzzing in this milestone targets Guardian's own parser, simulator and authenticated command implementation.

It is not network exploitation tooling.

No credential discovery, remote scanning or third-party attack automation is included.

## Next Milestone

M12 completes the planned lifecycle roadmap:

```text
signed firmware images
image authenticity verification
rollback protection
version policy
failure-safe update state machine
firmware-update test vectors
```
