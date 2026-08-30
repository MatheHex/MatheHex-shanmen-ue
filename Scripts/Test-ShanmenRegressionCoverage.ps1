[CmdletBinding()]
param(
    [string]$BaseRef = 'HEAD^',
    [string]$HeadRef = 'HEAD',
    [string[]]$ChangedPath,
    [string[]]$AutomationLogPath = @(),
    [string]$MappingPath = (Join-Path $PSScriptRoot 'ShanmenRegressionMap.json')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function ConvertTo-RepoPath {
    param([Parameter(Mandatory)][string]$Path)

    return ($Path.Trim() -replace '\\', '/').TrimStart('./')
}

function Test-AnyRegex {
    param(
        [Parameter(Mandatory)][string]$Value,
        [Parameter(Mandatory)][object[]]$Patterns
    )

    foreach ($Pattern in $Patterns)
    {
        if ($Value -match [string]$Pattern)
        {
            return $true
        }
    }
    return $false
}

function Test-GroupCoverage {
    param(
        [Parameter(Mandatory)][string]$RequiredGroup,
        [Parameter(Mandatory)][string]$ExecutedGroup
    )

    if ($RequiredGroup.Equals(
            $ExecutedGroup,
            [System.StringComparison]::OrdinalIgnoreCase))
    {
        return $true
    }
    if ($RequiredGroup.StartsWith(
            "${ExecutedGroup}.",
            [System.StringComparison]::OrdinalIgnoreCase))
    {
        return $true
    }
    return $false
}

function Read-AutomationEvidence {
    param([Parameter(Mandatory)][string]$Path)

    $Resolved = (Resolve-Path -LiteralPath $Path).Path
    $Text = Get-Content -LiteralPath $Resolved -Raw
    $CommandMatches = [regex]::Matches(
        $Text,
        '(?m)Cmd:\s+Automation RunTests\s+(?<Group>[^\r\n"]+)')
    $Groups = @(
        $CommandMatches
        | ForEach-Object { $_.Groups['Group'].Value.Trim() }
        | Where-Object { $_ }
        | Sort-Object -Unique)
    $SuccessCount = ([regex]::Matches(
            $Text,
            'Test Completed\. Result=\{Success\}')).Count
    $FailCount = ([regex]::Matches(
            $Text,
            'Test Completed\. Result=\{Fail\}')).Count
    # UE 5.8 command-line automation emits TEST COMPLETE instead of the
    # historical queue-empty sentence. Both are native terminal-success
    # markers; test and process failures are still checked independently.
    $QueueEmptyCount = ([regex]::Matches(
            $Text,
            'Automation Test Queue Empty\s+\d+\s+tests performed|\*{4}\s+TEST COMPLETE\. EXIT CODE:\s*0\s+\*{4}')).Count
    $FatalCount = ([regex]::Matches(
            $Text,
            'Fatal error|Unhandled Exception|Ensure condition failed')).Count

    $Problems = [System.Collections.Generic.List[string]]::new()
    if ($CommandMatches.Count -ne 1)
    {
        $Problems.Add("expected exactly one RunTests command, found $($CommandMatches.Count)")
    }
    if ($Groups.Count -ne 1)
    {
        $Problems.Add("expected exactly one RunTests group, found $($Groups.Count)")
    }
    if ($SuccessCount -lt 1)
    {
        $Problems.Add('no successful test result')
    }
    if ($FailCount -ne 0)
    {
        $Problems.Add("failed tests=$FailCount")
    }
    if ($QueueEmptyCount -lt 1)
    {
        $Problems.Add('terminal completion marker missing')
    }
    if ($FatalCount -ne 0)
    {
        $Problems.Add("fatal/unhandled/ensure markers=$FatalCount")
    }

    return [pscustomobject]@{
        Path = $Resolved
        Group = if ($Groups.Count -eq 1) { $Groups[0] } else { '' }
        SuccessCount = $SuccessCount
        FailCount = $FailCount
        QueueEmptyCount = $QueueEmptyCount
        FatalCount = $FatalCount
        SHA256 = (Get-FileHash -LiteralPath $Resolved -Algorithm SHA256).Hash
        IsHealthy = $Problems.Count -eq 0
        Problems = @($Problems)
    }
}

if (-not (Test-Path -LiteralPath $MappingPath -PathType Leaf))
{
    throw "REGRESSION_COVERAGE: mapping file missing: $MappingPath"
}

$Config = Get-Content -LiteralPath $MappingPath -Raw | ConvertFrom-Json
if ($Config.schemaVersion -ne 1)
{
    throw "REGRESSION_COVERAGE: unsupported mapping schema $($Config.schemaVersion)"
}
if ((-not $Config.rules) -or (-not $Config.productionPathRegexes) -or (-not $Config.ignoredPathRegexes))
{
    throw 'REGRESSION_COVERAGE: mapping must define rules, production paths, and ignored paths'
}

$ChangedPaths = if ($PSBoundParameters.ContainsKey('ChangedPath'))
{
    @($ChangedPath)
}
else
{
    $GitOutput = @(& git diff --name-only --diff-filter=ACMRTUXB "$BaseRef...$HeadRef" --)
    if ($LASTEXITCODE -ne 0)
    {
        throw "REGRESSION_COVERAGE: git diff failed for $BaseRef...$HeadRef"
    }
    $GitOutput
}
$ChangedPaths = @(
    $ChangedPaths
    | ForEach-Object { ConvertTo-RepoPath -Path $_ }
    | Where-Object { $_ }
    | Sort-Object -Unique)

$RequiredGroups = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::OrdinalIgnoreCase)
$MatchedRules = [System.Collections.Generic.HashSet[string]]::new(
    [System.StringComparer]::OrdinalIgnoreCase)
$UnmappedPaths = [System.Collections.Generic.List[string]]::new()

foreach ($Path in $ChangedPaths)
{
    if (Test-AnyRegex -Value $Path -Patterns @($Config.ignoredPathRegexes))
    {
        continue
    }

    $PathMatched = $false
    foreach ($Rule in @($Config.rules))
    {
        if ($Path -match [string]$Rule.pathRegex)
        {
            $PathMatched = $true
            [void]$MatchedRules.Add([string]$Rule.name)
            foreach ($Group in @($Rule.requiredGroups))
            {
                [void]$RequiredGroups.Add([string]$Group)
            }
        }
    }

    if (-not $PathMatched)
    {
        $IsProduction = Test-AnyRegex `
            -Value $Path `
            -Patterns @($Config.productionPathRegexes)
        if ($IsProduction)
        {
            $UnmappedPaths.Add($Path)
        }
        else
        {
            $UnmappedPaths.Add("$Path (unclassified)")
        }
    }
}

if ($UnmappedPaths.Count -gt 0)
{
    throw "REGRESSION_COVERAGE: unmapped changed paths: $($UnmappedPaths -join ', ')"
}

$Evidence = @(
    foreach ($Path in @($AutomationLogPath))
    {
        Read-AutomationEvidence -Path $Path
    })
$UnhealthyEvidence = @($Evidence | Where-Object { -not $_.IsHealthy })
if ($UnhealthyEvidence.Count -gt 0)
{
    $Details = @(
        $UnhealthyEvidence
        | ForEach-Object {
            "$($_.Path): $($_.Problems -join '; ')"
        }) -join ' | '
    throw "REGRESSION_COVERAGE: unhealthy evidence: $Details"
}

$MissingGroups = [System.Collections.Generic.List[string]]::new()
$CoverageRows = [System.Collections.Generic.List[object]]::new()
foreach ($RequiredGroup in @($RequiredGroups | Sort-Object))
{
    $CoveringEvidence = @(
        $Evidence
        | Where-Object {
            Test-GroupCoverage `
                -RequiredGroup $RequiredGroup `
                -ExecutedGroup $_.Group
        })
    if ($CoveringEvidence.Count -eq 0)
    {
        $MissingGroups.Add($RequiredGroup)
        continue
    }
    $CoverageRows.Add([pscustomobject]@{
            RequiredGroup = $RequiredGroup
            Evidence = @($CoveringEvidence | ForEach-Object { $_.Path })
        })
}

if ($MissingGroups.Count -gt 0)
{
    throw "REGRESSION_COVERAGE: missing required groups: $($MissingGroups -join ', ')"
}

Write-Output (
    'REGRESSION_COVERAGE: PASS Changed={0} Rules={1} Required={2} Logs={3}' `
        -f $ChangedPaths.Count,
            $MatchedRules.Count,
            $RequiredGroups.Count,
            $Evidence.Count)
foreach ($Row in $CoverageRows)
{
    Write-Output (
        'REGRESSION_COVERAGE: Group={0} Evidence={1}' `
            -f $Row.RequiredGroup,
                (($Row.Evidence | ForEach-Object {
                            Split-Path -Leaf $_
                        }) -join ','))
}
foreach ($Item in $Evidence)
{
    Write-Output (
        'REGRESSION_COVERAGE: Log={0} Group={1} Success={2} SHA256={3}' `
            -f (Split-Path -Leaf $Item.Path),
                $Item.Group,
                $Item.SuccessCount,
                $Item.SHA256)
}
