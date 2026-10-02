<#
    Self-check for GuardianRunner. Plain assertions, no test framework.

    R-35 argument quoting is verified against CommandLineToArgvW, the function
    every child process uses to rebuild its argument vector. Verification against
    an expectation restated by the test would establish internal consistency only,
    which is the condition under which SG-023 passed review.

    Run:  powershell -NoProfile -ExecutionPolicy Bypass -File GuardianRunner.Tests.ps1
#>
[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

Import-Module (Join-Path $PSScriptRoot 'GuardianRunner.psm1') -Force

$Failures = New-Object 'System.Collections.Generic.List[string]'
function Assert-That {
    param([string]$Name, [bool]$Condition, [string]$Detail = '')
    if ($Condition) {
        Write-Host "CHECK_$Name=PASS"
    }
    else {
        Write-Host ("CHECK_" + $Name + "=FAIL " + $Detail)
        [void]$Failures.Add($Name)
    }
}

# --- ground truth: the Win32 parser the child process actually runs ----------
Add-Type -Namespace Win32Native -Name Shell -MemberDefinition @'
[DllImport("shell32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
public static extern System.IntPtr CommandLineToArgvW(
    [MarshalAs(UnmanagedType.LPWStr)] string lpCmdLine, out int pNumArgs);
'@

function Get-Argv {
    param([string]$CommandLine)
    $Count = 0
    $Pointer = [Win32Native.Shell]::CommandLineToArgvW($CommandLine, [ref]$Count)
    if ($Pointer -eq [System.IntPtr]::Zero) { throw 'CommandLineToArgvW failed.' }
    try {
        $Result = New-Object 'System.Collections.Generic.List[string]'
        for ($Index = 0; $Index -lt $Count; $Index++) {
            $Element = [System.Runtime.InteropServices.Marshal]::ReadIntPtr(
                $Pointer, $Index * [System.IntPtr]::Size)
            [void]$Result.Add([System.Runtime.InteropServices.Marshal]::PtrToStringUni($Element))
        }
        return $Result.ToArray()
    }
    finally {
        [void][System.Runtime.InteropServices.Marshal]::FreeHGlobal($Pointer)
    }
}

# --- R-35: every argument must survive the round trip intact ----------------
$Cases = @(
    @('simple'),
    @('with space'),
    @('two', 'separate args'),
    @('quote"inside'),
    @('"fully quoted"'),
    @('trailing\'),
    @('trailing\\'),
    @('back\slash\path'),
    @('C:\Program Files\git\git.exe'),
    @('mixed a\"b c\'),
    @('--format=%s %b'),
    @('rule R-35 requires boundaries'),   # the SG-023 git grep pattern shape
    @('tab`there'.Replace('`', "`t")),
    @('a', '', 'b'),
    @('')
)

$CaseIndex = 0
foreach ($Case in $Cases) {
    $CaseIndex++
    $Original = @($Case)
    $CommandLine = [string]::Join(' ', @(foreach ($Argument in $Original) {
        ConvertTo-GuardianWin32Argument -Argument $Argument }))

    # argv[0] is the program name and is parsed by different rules, so prepend one.
    $Recovered = @(Get-Argv -CommandLine ('prog.exe ' + $CommandLine))
    $Recovered = @($Recovered[1..($Recovered.Length - 1)])

    $Same = ($Recovered.Count -eq $Original.Count)
    if ($Same) {
        for ($Index = 0; $Index -lt $Original.Count; $Index++) {
            if ($Recovered[$Index] -ne $Original[$Index]) { $Same = $false; break }
        }
    }
    Assert-That -Name "ARGV_ROUNDTRIP_$CaseIndex" -Condition $Same `
        -Detail ("sent=[" + ($Original -join '][') + "] line=[" + $CommandLine +
                 "] got=[" + ($Recovered -join '][') + "]")
}

# --- R-38: property existence must not throw under StrictMode 2.0 -----------
$Object = '{"id":"SG-001"}' | ConvertFrom-Json
Assert-That -Name 'PROP_PRESENT' -Condition (Test-GuardianProperty -InputObject $Object -Name 'id')
Assert-That -Name 'PROP_ABSENT'  -Condition (-not (Test-GuardianProperty -InputObject $Object -Name 'rules'))
Assert-That -Name 'PROP_NULL_INPUT' -Condition (-not (Test-GuardianProperty -InputObject $null -Name 'id'))

# R-38 evidence: the naive guard throws before its comparison is evaluated,
# which is the condition Test-GuardianProperty exists to remove.
$Threw = $false
try { if ($null -ne $Object.rules) { } } catch { $Threw = $true }
Assert-That -Name 'NAIVE_GUARD_THROWS' -Condition $Threw `
    -Detail 'expected PropertyNotFoundException under StrictMode 2.0'

# --- R-01 / R-26: exit code decides, channels stay separate -----------------
$RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Ok = Invoke-GuardianNative -FilePath 'git.exe' -WorkingDirectory $RepoRoot `
        -Arguments @('rev-parse', '--abbrev-ref', 'HEAD')
Assert-That -Name 'NATIVE_EXIT_ZERO' -Condition ($Ok.ExitCode -eq 0)
Assert-That -Name 'NATIVE_STDOUT'    -Condition ($Ok.Stdout.Length -gt 0)
Assert-That -Name 'NATIVE_STDERR_SEPARATE' -Condition ($Ok.Stderr.Length -eq 0)

$Bad = Invoke-GuardianNative -FilePath 'git.exe' -WorkingDirectory $RepoRoot `
        -Arguments @('rev-parse', 'refs/heads/definitely-not-a-branch-xyz')
Assert-That -Name 'NATIVE_EXIT_NONZERO' -Condition ($Bad.ExitCode -ne 0)

# R-39: trailing newline is preserved, not trimmed away by the helper.
Assert-That -Name 'NATIVE_OUTPUT_UNTRIMMED' -Condition ($Ok.Stdout -match '\n$')

# --- the SG-023 case end to end: a pattern containing whitespace ------------
$Pattern = 'Native process invocation SHALL'
$GrepArgs = @('grep', '-c', $Pattern, 'HEAD', '--',
              'docs/governance/Guardian-Scripting-Guardrails.md')

$Grep = Invoke-GuardianNative -FilePath 'git.exe' -WorkingDirectory $RepoRoot -Arguments $GrepArgs
Assert-That -Name 'SG023_WHITESPACE_PATTERN' -Condition ($Grep.ExitCode -eq 0) `
    -Detail ('exit=' + $Grep.ExitCode + ' stderr=' + $Grep.Stderr.Trim())

# Negative control: the SG-023 defect itself. A naive whitespace join splits the
# pattern, so git reads 'process' as a tree-ish and fails. If this ever starts
# passing, the round-trip checks above are no longer proving anything.
$NaiveLine = $GrepArgs -join ' '
$NaiveInfo = New-Object System.Diagnostics.ProcessStartInfo
$NaiveInfo.FileName = 'git.exe'
$NaiveInfo.Arguments = $NaiveLine
$NaiveInfo.WorkingDirectory = $RepoRoot
$NaiveInfo.UseShellExecute = $false
$NaiveInfo.RedirectStandardOutput = $true
$NaiveInfo.RedirectStandardError = $true
$NaiveInfo.CreateNoWindow = $true
$NaiveProcess = New-Object System.Diagnostics.Process
$NaiveProcess.StartInfo = $NaiveInfo
[void]$NaiveProcess.Start()
$NaiveOut = $NaiveProcess.StandardOutput.ReadToEndAsync()
$NaiveErr = $NaiveProcess.StandardError.ReadToEndAsync()
$NaiveProcess.WaitForExit()
$NaiveExit = [int]$NaiveProcess.ExitCode
[void]$NaiveOut.GetAwaiter().GetResult()
$NaiveStderr = [string]$NaiveErr.GetAwaiter().GetResult()
$NaiveProcess.Dispose()

Assert-That -Name 'SG023_NAIVE_JOIN_STILL_BROKEN' -Condition ($NaiveExit -ne $Grep.ExitCode) `
    -Detail ('naive exit=' + $NaiveExit + ' vs correct exit=' + $Grep.ExitCode +
             ' stderr=' + $NaiveStderr.Trim())

# --- R-29 / R-22: every PowerShell file in the repository, not just this one.
# R-29 applies to every runner, so a gate restricted to its own source
# establishes nothing about the next one issued.
$ToolsRoot = Split-Path -Parent $PSScriptRoot
# -Include has no effect when combined with -LiteralPath. Extension selection
# is explicit so the gate cannot silently widen to other file types.
$Scripts = @(Get-ChildItem -LiteralPath $ToolsRoot -Recurse -File |
             Where-Object { $_.Extension -match '^\.psm?1$' })

Assert-That -Name 'SCRIPTS_DISCOVERED' -Condition ($Scripts.Count -gt 0) `
    -Detail ('count=' + $Scripts.Count)

foreach ($Script in $Scripts) {
    $Gate = Test-GuardianScript -Path $Script.FullName
    Assert-That -Name ('GATE_' + $Script.Name.Replace('.', '_')) -Condition $Gate.Passed `
        -Detail ('nonAscii=' + $Gate.NonAsciiBytes + ' cr=' + $Gate.CarriageReturns +
                 ' parseErrors=' + $Gate.ParseErrors +
                 ' ' + ($Gate.Messages -join '; '))
}

Write-Host ''
Write-Host ('FAILED_CHECK_COUNT=' + $Failures.Count)
if ($Failures.Count -gt 0) {
    Write-Host ('FAILED=' + ($Failures -join ','))
    Write-Host 'FINAL=FAIL'
    exit 1
}
Write-Host 'FINAL=PASS'
exit 0
