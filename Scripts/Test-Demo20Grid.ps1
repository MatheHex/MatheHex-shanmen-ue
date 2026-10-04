[CmdletBinding()]
param([string]$TaskId = 'Demo20.M1.Grid')
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1') -Force
$gridProject = Resolve-ShanmenProject
if (@(Get-Process -Name UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue).Count) { throw 'Close existing UE before grid automation.' }
$gridUser = Join-Path $gridProject.ProjectRoot 'Saved/Demo20/GridAutomationUser'
New-Item -ItemType Directory -Path $gridUser -Force | Out-Null
$gridPaths = @(@(git -c core.quotepath=false diff --name-only HEAD) + @(git -c core.quotepath=false ls-files --others --exclude-standard) |
    Where-Object { $_ -match '^(Source/|Config/|Content/|Plugins/)' } | Sort-Object -Unique)
if (!$gridPaths.Count) { throw 'No changed production paths to validate.' }
$gridMap = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'ShanmenRegressionMap.json') -Raw | ConvertFrom-Json
$gridGroups = @($gridMap.rules | Where-Object {
    $gridRule = $_
    @($gridPaths | Where-Object { $_ -match $gridRule.pathRegex }).Count -gt 0
} | ForEach-Object requiredGroups | Sort-Object -Unique)
if (!$gridGroups.Count) { throw 'Changed production paths have no mapped regression groups.' }
$gridLogs = @()
foreach ($gridGroup in $gridGroups) {
    $gridEvidence = New-ShanmenEvidenceContext -Project $gridProject -Action "Automation-$gridGroup" -TaskId $TaskId
    $gridLog = Join-Path $gridEvidence.Root 'UnrealEditor.log'
    $gridArguments = @($gridProject.Uproject,'/Engine/Maps/Entry','-unattended','-nop4','-nosplash','-NullRHI','-NoSound',
        '-FORCELOGFLUSH',"-UserDir=$gridUser","-ExecCmds=Automation RunTests $gridGroup",'-TestExit=Automation Test Queue Empty',"-abslog=$gridLog")
    $gridProcess = Start-ShanmenTrackedProcess -FilePath $gridProject.EditorCmd -ArgumentList $gridArguments -WorkingDirectory $gridProject.ProjectRoot -Evidence $gridEvidence -Wait -Hidden
    $gridText = Get-Content -LiteralPath $gridLog -Raw
    $gridCompleted = $gridText -match '(Automation Test Queue Empty\s+\d+\s+tests performed|TEST COMPLETE\. EXIT CODE: 0)'
    if ($gridProcess.State.exit_code -ne 0 -or $gridText -match 'Result=\{Fail\}' -or $gridText -notmatch 'Result=\{Success\}' -or !$gridCompleted) {
        throw "Regression not proven successful: $gridLog"
    }
    [pscustomobject]@{Group=$gridGroup;Success=([regex]::Matches($gridText,'Result=\{Success\}')).Count;
        Fail=([regex]::Matches($gridText,'Result=\{Fail\}')).Count;NativeExit=$gridProcess.State.exit_code;
        Log=$gridLog;SHA256=(Get-FileHash -LiteralPath $gridLog -Algorithm SHA256).Hash} | Format-List
    $gridLogs += $gridLog
}
& (Join-Path $PSScriptRoot 'Test-ShanmenRegressionCoverage.ps1') -ChangedPath $gridPaths -AutomationLogPath $gridLogs
