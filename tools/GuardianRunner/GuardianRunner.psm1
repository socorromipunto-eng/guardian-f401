<#
    GuardianRunner - shared primitives for governed Windows PowerShell 5.1 runners.

    Exists so a runner is ~40 lines instead of ~700, and so the failure classes
    already recorded in governance/scripting-incident-register.json cannot be
    reintroduced by hand (R-19).

    Rule coverage:
      R-01  native failure is decided by exit code, never by STDERR content
      R-02  collections are explicitly wrapped and counted
      R-06  repository preflight before any mutation
      R-17  output uses explicit semantic labels, never a bare SUCCESS
      R-22  this module is ASCII-only
      R-26  STDOUT and STDERR stay on separate channels
      R-29  mechanical parser gate available to callers
      R-35  argument boundaries preserved by Win32 quoting, not a whitespace join
      R-38  properties proven via PSObject.Properties before dereference
      R-39  native output is never trimmed before its grammar is parsed
#>

Set-StrictMode -Version 2.0

# ---------------------------------------------------------------------------
# R-38: the only safe way to ask whether a property exists under StrictMode 2.0.
# "$null -ne $o.prop" throws PropertyNotFoundException first (SG-025).
# ---------------------------------------------------------------------------
function Test-GuardianProperty {
    [CmdletBinding()]
    [OutputType([bool])]
    param(
        [Parameter(Mandatory)][AllowNull()]$InputObject,
        [Parameter(Mandatory)][string]$Name
    )
    if ($null -eq $InputObject) { return $false }
    return ([bool]($InputObject.PSObject.Properties.Name -contains $Name))
}

function Get-GuardianProperty {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][AllowNull()]$InputObject,
        [Parameter(Mandatory)][string]$Name,
        $Default = $null
    )
    if (Test-GuardianProperty -InputObject $InputObject -Name $Name) {
        return $InputObject.$Name
    }
    return $Default
}

# ---------------------------------------------------------------------------
# R-35: Windows PowerShell 5.1 runs on .NET Framework, where
# ProcessStartInfo.ArgumentList does not exist. The command line must therefore
# be built by hand following the CommandLineToArgvW rules exactly, so the child
# process recovers the same argument vector it was given.
# Joining with spaces (SG-023) and rejecting whitespace (SG-026) are not fixes.
# ---------------------------------------------------------------------------
function ConvertTo-GuardianWin32Argument {
    [CmdletBinding()]
    [OutputType([string])]
    param(
        [Parameter(Mandatory)][AllowEmptyString()][string]$Argument
    )

    if ($Argument.Length -gt 0 -and $Argument -notmatch '[ \t\n\v"]') {
        return $Argument
    }

    $Builder = New-Object System.Text.StringBuilder
    [void]$Builder.Append([char]34)

    for ($Index = 0; $Index -lt $Argument.Length; $Index++) {

        $Backslashes = 0
        while ($Index -lt $Argument.Length -and $Argument[$Index] -eq [char]92) {
            $Index++
            $Backslashes++
        }

        if ($Index -eq $Argument.Length) {
            # Trailing backslashes precede the closing quote, so they double.
            [void]$Builder.Append([string][char]92 * ($Backslashes * 2))
            break
        }
        elseif ($Argument[$Index] -eq [char]34) {
            # Backslashes before a literal quote double, then the quote escapes.
            [void]$Builder.Append([string][char]92 * ($Backslashes * 2 + 1))
            [void]$Builder.Append([char]34)
        }
        else {
            [void]$Builder.Append([string][char]92 * $Backslashes)
            [void]$Builder.Append($Argument[$Index])
        }
    }

    [void]$Builder.Append([char]34)
    return $Builder.ToString()
}

function ConvertTo-GuardianWin32CommandLine {
    [CmdletBinding()]
    [OutputType([string])]
    param(
        [Parameter(Mandatory)][AllowEmptyCollection()][AllowEmptyString()][string[]]$Arguments
    )
    $Quoted = @(foreach ($Argument in $Arguments) {
        ConvertTo-GuardianWin32Argument -Argument $Argument
    })
    return [string]::Join(' ', $Quoted)
}

# ---------------------------------------------------------------------------
# R-01 / R-26 / R-39: exit code decides failure, stdout and stderr stay apart,
# and output is returned verbatim, never trimmed before the caller parses it.
# ---------------------------------------------------------------------------
function Invoke-GuardianNative {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][AllowEmptyCollection()][AllowEmptyString()][string[]]$Arguments,
        [Parameter(Mandatory)][string]$WorkingDirectory
    )

    $StartInfo = New-Object System.Diagnostics.ProcessStartInfo
    $StartInfo.FileName               = $FilePath
    $StartInfo.Arguments              = ConvertTo-GuardianWin32CommandLine -Arguments $Arguments
    $StartInfo.WorkingDirectory       = $WorkingDirectory
    $StartInfo.UseShellExecute        = $false
    $StartInfo.RedirectStandardOutput = $true
    $StartInfo.RedirectStandardError  = $true
    $StartInfo.CreateNoWindow         = $true

    $Process = New-Object System.Diagnostics.Process
    $Process.StartInfo = $StartInfo

    if (-not $Process.Start()) {
        throw "Unable to start native process: $FilePath"
    }

    # Read both pipes before waiting, or a full pipe buffer deadlocks the child.
    $StdoutTask = $Process.StandardOutput.ReadToEndAsync()
    $StderrTask = $Process.StandardError.ReadToEndAsync()
    $Process.WaitForExit()

    $Result = [pscustomobject]@{
        ExitCode = [int]$Process.ExitCode
        Stdout   = [string]$StdoutTask.GetAwaiter().GetResult()
        Stderr   = [string]$StderrTask.GetAwaiter().GetResult()
    }
    $Process.Dispose()
    return $Result
}

function Invoke-GuardianGit {
    [CmdletBinding()]
    [OutputType([string])]
    param(
        [Parameter(Mandatory)][string]$RepoRoot,
        [Parameter(Mandatory, ValueFromRemainingArguments)][string[]]$Arguments
    )
    $Result = Invoke-GuardianNative -FilePath 'git.exe' -Arguments $Arguments -WorkingDirectory $RepoRoot
    if ($Result.ExitCode -ne 0) {
        throw ('git ' + ($Arguments -join ' ') + ' failed with exit ' +
               $Result.ExitCode + ': ' + $Result.Stderr)
    }
    return $Result.Stdout
}

# ---------------------------------------------------------------------------
# R-29 / R-22: mechanical parser gate and ASCII gate over a runner before it is
# issued. Parser success is necessary, never sufficient (SG-020).
# ---------------------------------------------------------------------------
function Test-GuardianScript {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Path
    )

    $Bytes    = [System.IO.File]::ReadAllBytes($Path)
    $NonAscii = @($Bytes | Where-Object { $_ -gt 127 }).Count

    $Tokens = $null
    $Errors = $null
    [void][System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$Tokens, [ref]$Errors)
    $ParseErrors = @($Errors)

    return [pscustomobject]@{
        Path          = $Path
        NonAsciiBytes = [int]$NonAscii
        ParseErrors   = [int]$ParseErrors.Count
        Messages      = @($ParseErrors | ForEach-Object {
                              [string]$_.Extent.StartLineNumber + ': ' + [string]$_.Message })
        Passed        = ([int]$NonAscii -eq 0 -and $ParseErrors.Count -eq 0)
    }
}

# ---------------------------------------------------------------------------
# R-06: prove branch, HEAD, index and worktree scope before any mutation.
# Throws on any drift, so a runner cannot proceed on an unexpected baseline.
# ---------------------------------------------------------------------------
function Assert-GuardianRepositoryState {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$RepoRoot,
        [Parameter(Mandatory)][string]$ExpectedBranch,
        [Parameter(Mandatory)][string]$ExpectedHead,
        [string[]]$ExpectedWorktreePaths = @(),
        [switch]$RequireCleanIndex
    )

    # R-39: only the single trailing newline git appends is removed.
    $Branch = (Invoke-GuardianGit -RepoRoot $RepoRoot rev-parse --abbrev-ref HEAD) -replace '\r?\n$', ''
    $Head   = (Invoke-GuardianGit -RepoRoot $RepoRoot rev-parse HEAD) -replace '\r?\n$', ''

    if ($Branch -ne $ExpectedBranch) { throw "Branch drift. Expected=$ExpectedBranch Observed=$Branch" }
    if ($Head -ne $ExpectedHead) { throw "HEAD drift. Expected=$ExpectedHead Observed=$Head" }

    $Worktree = @((Invoke-GuardianGit -RepoRoot $RepoRoot diff --name-only) -split '\r?\n' |
                  Where-Object { $_.Length -gt 0 })
    $Index    = @((Invoke-GuardianGit -RepoRoot $RepoRoot diff --cached --name-only) -split '\r?\n' |
                  Where-Object { $_.Length -gt 0 })

    if ($RequireCleanIndex -and $Index.Count -ne 0) {
        throw ('Index must be clean. Staged=' + ($Index -join ', '))
    }

    $Expected = @($ExpectedWorktreePaths)
    $ObservedKey = ($Worktree | Sort-Object) -join '|'
    $ExpectedKey = ($Expected | Sort-Object) -join '|'
    if ($ObservedKey -ne $ExpectedKey) {
        throw ('Worktree scope drift. Expected=[' + $ExpectedKey +
               '] Observed=[' + $ObservedKey + ']')
    }

    return [pscustomobject]@{
        Branch        = $Branch
        Head          = $Head
        WorktreePaths = $Worktree
        IndexPaths    = $Index
    }
}

# ---------------------------------------------------------------------------
# R-17: semantic labels. Mirrors tools/governance_tooling/result.py so a
# PowerShell runner and a Python gate report in the same shape.
# ---------------------------------------------------------------------------
function New-GuardianResult {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$Command,
        [Parameter(Mandatory)][string]$Repository
    )
    return [pscustomobject]@{
        Command    = $Command
        Repository = $Repository
        Checks     = New-Object 'System.Collections.Generic.List[object]'
        Authority  = [ordered]@{
            COMMIT = 'NOT_GRANTED'
            PUSH   = 'NOT_GRANTED'
            PR     = 'NOT_GRANTED'
            MERGE  = 'NOT_GRANTED'
        }
    }
}

function Add-GuardianCheck {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Result,
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][bool]$Passed,
        [string]$Detail = ''
    )
    $Status = if ($Passed) { 'PASS' } else { 'FAIL' }
    [void]$Result.Checks.Add([pscustomobject]@{
        Name   = $Name
        Status = $Status
        Detail = $Detail
    })
}

function Write-GuardianResult {
    [CmdletBinding()]
    [OutputType([int])]
    param(
        [Parameter(Mandatory)]$Result
    )
    $Failed = @($Result.Checks | Where-Object { $_.Status -eq 'FAIL' })

    Write-Host 'GUARDIAN_RUNNER_RESULT_V1'
    Write-Host "COMMAND=$($Result.Command)"
    Write-Host "REPOSITORY=$($Result.Repository)"
    foreach ($Check in $Result.Checks) {
        $Suffix = if ($Check.Detail) { ' (' + $Check.Detail + ')' } else { '' }
        Write-Host ('CHECK_' + $Check.Name + '=' + $Check.Status + $Suffix)
    }
    foreach ($Key in $Result.Authority.Keys) {
        Write-Host ('AUTHORITY_' + $Key + '=' + $Result.Authority[$Key])
    }
    Write-Host "FAILED_CHECK_COUNT=$($Failed.Count)"
    if ($Failed.Count -eq 0) { Write-Host 'FINAL=PASS'; return 0 }
    Write-Host 'FINAL=FAIL'
    return 1
}

Export-ModuleMember -Function @(
    'Test-GuardianProperty'
    'Get-GuardianProperty'
    'ConvertTo-GuardianWin32Argument'
    'ConvertTo-GuardianWin32CommandLine'
    'Invoke-GuardianNative'
    'Invoke-GuardianGit'
    'Test-GuardianScript'
    'Assert-GuardianRepositoryState'
    'New-GuardianResult'
    'Add-GuardianCheck'
    'Write-GuardianResult'
)
