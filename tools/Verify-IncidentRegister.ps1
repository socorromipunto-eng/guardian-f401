<#
    Guardian F401 - incident register verifier.

    Re-runnable gate over governance/scripting-incident-register.json.
    No mutation, no network, no git writes. Exit 1 on any failure.

    R-38: every property read is existence-checked first. Under
    StrictMode 2.0 a missing property throws PropertyNotFoundException,
    so `$null -ne $o.prop` is not a guard (SG-025).
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

$RegisterPath   = Join-Path $RepoRoot 'governance/scripting-incident-register.json'
$GuardrailsPath = Join-Path $RepoRoot 'docs/governance/Guardian-Scripting-Guardrails.md'

$Failures = New-Object 'System.Collections.Generic.List[string]'
function Add-Failure { param([string]$Message) [void]$Failures.Add($Message) }

foreach ($Path in @($RegisterPath, $GuardrailsPath)) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Write-Error "Missing required file: $Path"
        exit 1
    }
}

# --- register must be pure ASCII: byte index == char index downstream ---
$Bytes = [System.IO.File]::ReadAllBytes($RegisterPath)
$NonAscii = @($Bytes | Where-Object { $_ -gt 127 }).Count
if ($NonAscii -ne 0) { Add-Failure "Register contains $NonAscii non-ASCII byte(s)." }

$Register = [System.Text.Encoding]::ASCII.GetString($Bytes) | ConvertFrom-Json

$ExpectedTopLevel = @('schema_version','project','status','policy','incidents')
$ObservedTopLevel = @($Register.PSObject.Properties.Name)
if ("$ObservedTopLevel" -ne "$ExpectedTopLevel") {
    Add-Failure "Top-level keys drift. Expected [$ExpectedTopLevel]; observed [$ObservedTopLevel]."
}

# --- guardrail rule ids ---
$GuardrailText = [System.IO.File]::ReadAllText($GuardrailsPath)
$RuleIds = New-Object 'System.Collections.Generic.HashSet[string]'
foreach ($M in [regex]::Matches($GuardrailText, '(?m)^\|\s*(R-\d{2})\s*\|')) {
    if (-not $RuleIds.Add($M.Groups[1].Value)) {
        Add-Failure "Duplicate guardrail row: $($M.Groups[1].Value)"
    }
}
if ($RuleIds.Count -eq 0) { Add-Failure 'No guardrail rules parsed.' }

# --- incidents ---
$ExpectedKeys = @('id','date','class','root_cause','prevention','rules')
$SeenIds = New-Object 'System.Collections.Generic.HashSet[string]'
$Ordinal = 0

foreach ($Incident in $Register.incidents) {
    $Ordinal++
    $Label = "incident #$Ordinal"

    $Keys = @($Incident.PSObject.Properties.Name)
    if ("$Keys" -ne "$ExpectedKeys") {
        Add-Failure "$Label key contract drift: [$Keys]"
        continue
    }

    $Id = [string]$Incident.id
    $Label = $Id
    if (-not $SeenIds.Add($Id)) { Add-Failure "$Label duplicated." }

    $Expected = 'SG-{0:000}' -f $Ordinal
    if ($Id -ne $Expected) { Add-Failure "Sequence break: expected $Expected, got $Id." }

    # SG-001..SG-004 predate day-precision dating; month precision is allowed
    # rather than fabricating a day into an audit record.
    if ([string]$Incident.date -notmatch '^\d{4}-\d{2}(-\d{2})?$') {
        Add-Failure "$Label date is not ISO-8601: $($Incident.date)"
    }
    foreach ($Field in @('class','root_cause','prevention')) {
        if ([string]::IsNullOrWhiteSpace([string]$Incident.$Field)) {
            Add-Failure "$Label has empty $Field."
        }
    }

    $Rules = @($Incident.rules)
    if ($Rules.Count -eq 0) { Add-Failure "$Label references no rule." }
    foreach ($Rule in $Rules) {
        if (-not $RuleIds.Contains([string]$Rule)) {
            Add-Failure "$Label references unknown rule $Rule."
        }
    }
}

Write-Host "incidents=$Ordinal rules=$($RuleIds.Count) failures=$($Failures.Count)"
if ($Failures.Count -gt 0) {
    $Failures | ForEach-Object { Write-Host "FAIL: $_" }
    exit 1
}
Write-Host 'PASS'
exit 0
