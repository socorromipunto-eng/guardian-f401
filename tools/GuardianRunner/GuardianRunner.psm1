<#
    GuardianRunner - primitives for governed Windows PowerShell 5.1 runners.

    One function per recorded failure class. Nothing speculative: a helper is
    added here when a runner needs it, not before.

      R-35  ConvertTo-GuardianWin32Argument  SG-023, SG-026
      R-38  Test-GuardianProperty            SG-025
      R-01  Invoke-GuardianNative            exit code decides, not STDERR
      R-26  Invoke-GuardianNative            STDOUT and STDERR stay separate
      R-39  Invoke-GuardianNative            output returned verbatim
      R-29  Test-GuardianScript              SG-020
      R-17  Write-GuardianResult             explicit labels, never bare SUCCESS
      R-22  this module is ASCII-only
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

# ---------------------------------------------------------------------------
# R-35: PowerShell 5.1 runs on .NET Framework, where ProcessStartInfo.ArgumentList
# does not exist, so the command line must be built following CommandLineToArgvW
# rules. Joining with spaces (SG-023) and rejecting whitespace (SG-026) are not
# fixes: a quote or a trailing backslash breaks the boundary just as reliably.
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

    $Quoted = @(foreach ($Argument in $Arguments) {
        ConvertTo-GuardianWin32Argument -Argument $Argument
    })

    $StartInfo = New-Object System.Diagnostics.ProcessStartInfo
    $StartInfo.FileName               = $FilePath
    $StartInfo.Arguments              = [string]::Join(' ', $Quoted)
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

# ---------------------------------------------------------------------------
# R-29 / R-22: mechanical parser and ASCII gate over a runner before issuance.
# Parser success is necessary, never sufficient (SG-020).
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
# R-17: explicit labels, matching tools/governance_tooling/result.py so a
# PowerShell runner and a Python gate report in the same shape.
# Each check is a hashtable: @{ Name = '...'; Passed = $true; Detail = '...' }
# Returns the process exit code.
# ---------------------------------------------------------------------------
function Write-GuardianResult {
    [CmdletBinding()]
    [OutputType([int])]
    param(
        [Parameter(Mandatory)][string]$Command,
        [Parameter(Mandatory)][string]$Repository,
        [Parameter(Mandatory)][AllowEmptyCollection()][object[]]$Checks
    )

    $FailedCount = 0

    Write-Host 'GUARDIAN_RUNNER_RESULT_V1'
    Write-Host "COMMAND=$Command"
    Write-Host "REPOSITORY=$Repository"

    foreach ($Check in $Checks) {
        $Status = if ($Check.Passed) { 'PASS' } else { $FailedCount++; 'FAIL' }
        $Suffix = if ($Check.Detail) { ' (' + $Check.Detail + ')' } else { '' }
        Write-Host ('CHECK_' + $Check.Name + '=' + $Status + $Suffix)
    }

    Write-Host "FAILED_CHECK_COUNT=$FailedCount"
    if ($FailedCount -eq 0) { Write-Host 'FINAL=PASS'; return 0 }
    Write-Host 'FINAL=FAIL'
    return 1
}

Export-ModuleMember -Function @(
    'Test-GuardianProperty'
    'ConvertTo-GuardianWin32Argument'
    'Invoke-GuardianNative'
    'Test-GuardianScript'
    'Write-GuardianResult'
)
