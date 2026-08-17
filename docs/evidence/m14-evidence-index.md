# M14 assurance evidence index

This repository records evidence identifiers and hashes. Raw controlled
evidence remains outside the repository to avoid publishing workstation paths,
transcripts or unnecessary operational metadata.

| Evidence | Manifest SHA-256 |
|---|---|
| RFC 8785 evaluation | `A360330199D1141D2150A1C2E79BA6F463064E60BD5E66B12321FBAB09C5D271` |
| JCS differential evaluation | `15B64345B56529CBD0F9683D552E54F792CC7103F4B0BB30EEB50D2E01089A4A` |
| Guard/limits corrected evaluation | `AE3F39964E7DCBA917F17E1035C6445CD716C0EB0047109FB241B994E0F37154` |
| Structural-limits evaluation | `FF9A156403A897183B6947368CFC1D9FC266131770AAB08FBBDF9BEE0B67184B` |
| S1 post-commit validation | `548CAF33B1AEDC1FCDE74DF4FCD4A50ABC93EBD3D9EEF30F915ECBFB62017B5C` |
| S2 post-commit validation | `75BB607E0BB39ACF72CC625362A3171ABF31219CB7A7C91AB9AA60A8886FA61C` |
| Initial remote CPython 3.12 run | `FA695D8A1F971B6253EF9F9892874BC0092E3FDADEDB7EFFBAC5BBFA27C51F38` |
| EOF pre-commit review | `C855DBADAA1C8BBBAF659AB70CFBEABE5067965AA894EBC888E306CEB8ED89F2` |
| EOF local commit | `A6F7174B173073B73FC7F9A6D0A2760DC4CDAAC271A14AF4CFF225C550A08ED6` |
| Final post-commit, push and remote CI | `0E3715DF6F80FECD9C804F0FA87FCF9AC22EFE917EFAC18F3E8C902B7C3B626A` |

## Superseded or failed evidence

Failed and superseded attempts remain preserved in the controlled evidence
area. They are not silently deleted and are not used as the final PASS basis.
This index intentionally does not repeat local absolute paths.
