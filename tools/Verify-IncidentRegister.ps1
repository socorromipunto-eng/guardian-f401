<#
    Guardian F401 - incident register verifier.

    Re-runnable gate over governance/scripting-incident-register.json.
    Read-only: no mutation, no network, no git writes. Exit 1 on any failure.

    Built on tools/GuardianRunner, so the failure classes recorded in the
    register cannot be reintroduced here by hand.

    Establishes: R-03 git text identity, R-22 ASCII source, R-33 canonical
    container and identifier fields, R-34 rule coherence with exact cardinality.
#>
[CmdletBinding()]
param(
    [string]$RepoRoot
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

# $PSScriptRoot is not populated inside a param default under -File.
if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Split-Path -Parent $PSScriptRoot
}

Import-Module (Join-Path $PSScriptRoot 'GuardianRunner/GuardianRunner.psm1') -Force

$RegisterPath   = Join-Path $RepoRoot 'governance/scripting-incident-register.json'
$GuardrailsPath = Join-Path $RepoRoot 'docs/governance/Guardian-Scripting-Guardrails.md'

$Checks   = New-Object 'System.Collections.Generic.List[object]'
$Problems = New-Object 'System.Collections.Generic.List[string]'

foreach ($Path in @($RegisterPath, $GuardrailsPath)) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Write-Host ('MISSING_FILE=' + $Path)
        exit 1
    }
}

# --- register must be pure ASCII, so byte index equals char index -----------
$Bytes    = [System.IO.File]::ReadAllBytes($RegisterPath)
$NonAscii = @($Bytes | Where-Object { $_ -gt 127 }).Count
[void]$Checks.Add(@{ Name = 'REGISTER_ASCII'; Passed = ($NonAscii -eq 0)
                     Detail = 'nonAsciiBytes=' + $NonAscii })

$Register = [System.Text.Encoding]::ASCII.GetString($Bytes) | ConvertFrom-Json

$ExpectedTopLevel = @('schema_version', 'project', 'status', 'policy', 'incidents')
$ObservedTopLevel = @($Register.PSObject.Properties.Name)
[void]$Checks.Add(@{ Name = 'TOP_LEVEL_CONTRACT'
                     Passed = (($ObservedTopLevel -join '|') -eq ($ExpectedTopLevel -join '|'))
                     Detail = 'observed=' + ($ObservedTopLevel -join ',') })

# --- guardrail rule ids ------------------------------------------------------
$GuardrailText = [System.IO.File]::ReadAllText($GuardrailsPath)
$RuleIds = New-Object 'System.Collections.Generic.HashSet[string]'
foreach ($Match in [regex]::Matches($GuardrailText, '(?m)^\|\s*(R-\d{2})\s*\|')) {
    if (-not $RuleIds.Add($Match.Groups[1].Value)) {
        [void]$Problems.Add('Duplicate guardrail row: ' + $Match.Groups[1].Value)
    }
}
[void]$Checks.Add(@{ Name = 'GUARDRAILS_PARSED'; Passed = ($RuleIds.Count -gt 0)
                     Detail = 'rules=' + $RuleIds.Count })

# --- incidents ---------------------------------------------------------------
$ExpectedKeys = @('id', 'date', 'class', 'root_cause', 'prevention', 'rules')
$SeenIds = New-Object 'System.Collections.Generic.HashSet[string]'
$Ordinal = 0

foreach ($Incident in $Register.incidents) {
    $Ordinal++

    # R-38: prove the key contract before dereferencing any field.
    $Keys = @($Incident.PSObject.Properties.Name)
    if (($Keys -join '|') -ne ($ExpectedKeys -join '|')) {
        [void]$Problems.Add('incident #' + $Ordinal + ' key contract drift: ' + ($Keys -join ','))
        continue
    }

    $Id = [string]$Incident.id
    if (-not $SeenIds.Add($Id)) { [void]$Problems.Add($Id + ' duplicated.') }

    $Expected = 'SG-{0:000}' -f $Ordinal
    if ($Id -ne $Expected) {
        [void]$Problems.Add('Sequence break: expected ' + $Expected + ', got ' + $Id)
    }

    # SG-001..SG-004 predate day-precision dating. Month precision is allowed
    # rather than fabricating a day into an audit record.
    if ([string]$Incident.date -notmatch '^\d{4}-\d{2}(-\d{2})?$') {
        [void]$Problems.Add($Id + ' date is not ISO-8601: ' + [string]$Incident.date)
    }

    foreach ($Field in @('class', 'root_cause', 'prevention')) {
        if ([string]::IsNullOrWhiteSpace([string]$Incident.$Field)) {
            [void]$Problems.Add($Id + ' has empty ' + $Field)
        }
    }

    $Rules = @($Incident.rules)
    if ($Rules.Count -eq 0) { [void]$Problems.Add($Id + ' references no rule.') }
    foreach ($Rule in $Rules) {
        if (-not $RuleIds.Contains([string]$Rule)) {
            [void]$Problems.Add($Id + ' references unknown rule ' + [string]$Rule)
        }
    }
}

[void]$Checks.Add(@{ Name = 'INCIDENT_INTEGRITY'; Passed = ($Problems.Count -eq 0)
                     Detail = 'incidents=' + $Ordinal + ' problems=' + $Problems.Count })

# --- R-03: git text identity under EOL normalization ------------------------
# A raw worktree SHA-256 is not a git object id. Identity is established with
# git hash-object --path against the index object id, so a worktree copy
# normalized to a different line ending is detected rather than assumed equal.
$RegisterRelative = 'governance/scripting-incident-register.json'

$HashObject = Invoke-GuardianNative -FilePath 'git.exe' -WorkingDirectory $RepoRoot `
    -Arguments @('hash-object', '--path', $RegisterRelative, $RegisterRelative)
$IndexEntry = Invoke-GuardianNative -FilePath 'git.exe' -WorkingDirectory $RepoRoot `
    -Arguments @('ls-files', '-s', '--', $RegisterRelative)

# R-39: the trailing newline git appends is the only character removed.
$WorktreeOid = $HashObject.Stdout.TrimEnd([char]10, [char]13)
$IndexOid    = ''
if ($IndexEntry.ExitCode -eq 0 -and $IndexEntry.Stdout -match '^[^ ]+ ([0-9a-f]{40}) ') {
    $IndexOid = $Matches[1]
}

[void]$Checks.Add(@{ Name = 'REGISTER_GIT_IDENTITY'
                     Passed = ($HashObject.ExitCode -eq 0 -and $IndexOid.Length -eq 40 -and
                               $WorktreeOid -eq $IndexOid)
                     Detail = 'worktree=' + $WorktreeOid + ' index=' + $IndexOid })

foreach ($Problem in $Problems) { Write-Host ('PROBLEM=' + $Problem) }

exit (Write-GuardianResult -Command 'verify-incident-register' -Repository $RepoRoot -Checks $Checks)
