# Guardian F401 Robustness and Fuzzing

M11 contains deterministic defensive mutation drivers and optional libFuzzer targets.

## Python

```text
PYTHONPATH=protocol/python:simulator/src:console/src:fuzz/python \
python -m unittest discover -s fuzz/python/tests -v

python tools/run_robustness.py --seed 0xC0FFEE11 --iterations 1000
```

## GCC Sanitizer Drivers

Parser:

```text
gcc \
  -std=c11 \
  -Wall -Wextra -Wpedantic -Werror \
  -O1 -g -fno-omit-frame-pointer \
  -fsanitize=address,undefined \
  -Ifirmware/Protocol/Inc \
  firmware/Protocol/Src/guardian_protocol.c \
  firmware/Protocol/Src/guardian_parser.c \
  fuzz/c/parser_mutation_driver.c \
  -o parser_mutation_driver

./parser_mutation_driver 10000
```

Security:

```text
gcc \
  -std=c11 \
  -Wall -Wextra -Wpedantic -Werror \
  -O1 -g -fno-omit-frame-pointer \
  -fsanitize=address,undefined \
  -Ifirmware/Protocol/Inc \
  -Ifirmware/Security/Inc \
  firmware/Security/Src/guardian_crypto.c \
  firmware/Security/Src/guardian_security.c \
  fuzz/c/security_mutation_driver.c \
  -o security_mutation_driver

./security_mutation_driver 10000
```

## Clang libFuzzer

Parser:

```text
clang \
  -std=c11 \
  -Wall -Wextra -Wpedantic -Werror \
  -O1 -g -fno-omit-frame-pointer \
  -fsanitize=fuzzer,address,undefined \
  -Ifirmware/Protocol/Inc \
  firmware/Protocol/Src/guardian_protocol.c \
  firmware/Protocol/Src/guardian_parser.c \
  fuzz/c/parser_libfuzzer.c \
  -o parser_fuzzer

./parser_fuzzer fuzz/corpus/parser -max_len=512
```

Security:

```text
clang \
  -std=c11 \
  -Wall -Wextra -Wpedantic -Werror \
  -O1 -g -fno-omit-frame-pointer \
  -fsanitize=fuzzer,address,undefined \
  -Ifirmware/Protocol/Inc \
  -Ifirmware/Security/Inc \
  firmware/Security/Src/guardian_crypto.c \
  firmware/Security/Src/guardian_security.c \
  fuzz/c/security_libfuzzer.c \
  -o security_fuzzer

./security_fuzzer fuzz/corpus/security -max_len=512
```

The fuzz targets are intended only for defensive testing of this repository.
