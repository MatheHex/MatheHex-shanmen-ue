[CmdletBinding()]
param(
    [string]$StageId = 'Manual.I',
    [string]$EngineRoot,
    [string]$OutputRoot,
    [switch]$RequireClean
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Import-Module (Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1') -Force
$project = Resolve-ShanmenProject -EngineRoot $EngineRoot -AllowMissingEngine
if ([string]::IsNullOrWhiteSpace($OutputRoot)) {
    $stamp = (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssfffZ')
    $OutputRoot = Join-Path $project.ProjectRoot (
        'Saved\FoundationAudit\{0}\{1}' -f
            (ConvertTo-ShanmenSafeName $StageId), $stamp)
}
$OutputRoot = [IO.Path]::GetFullPath($OutputRoot)
if (-not (Test-ShanmenPathWithin -Candidate $OutputRoot -Parent (Join-Path $project.ProjectRoot 'Saved'))) {
    throw 'Foundation audit output must stay under the project Saved directory.'
}
New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null

$checks = New-Object System.Collections.Generic.List[object]
function Add-FoundationCheck {
    param(
        [string]$Id,
        [string]$Category,
        [ValidateSet('BLOCKER', 'WARNING', 'INFO')][string]$Severity,
        [bool]$Passed,
        [string]$Diagnostic,
        [string[]]$Evidence = @()
    )
    $checks.Add([pscustomobject]@{
        id = $Id
        category = $Category
        severity = $Severity
        passed = $Passed
        diagnostic = $Diagnostic
        evidence = @($Evidence)
    })
}

$gitHead = $null
$gitValid = $false
try {
    $gitOutput = @(& git -C $project.ProjectRoot rev-parse HEAD 2>$null)
    if ($gitOutput.Count -gt 0) { $gitHead = [string]$gitOutput[0] }
    $gitValid = $gitHead -match '^[0-9a-fA-F]{40}$' -and
        (Test-Path -LiteralPath (Join-Path $project.ProjectRoot '.git\HEAD') -PathType Leaf)
}
catch { $gitValid = $false }
Add-FoundationCheck -Id 'VC.BASELINE' -Category 'version-control' -Severity 'BLOCKER' -Passed $gitValid -Diagnostic $(if ($gitValid) { "Git baseline available at $gitHead." } else { 'No valid Git baseline.' }) -Evidence @((Join-Path $project.ProjectRoot '.git'))

$dirty = @()
if ($gitValid) { $dirty = @(& git -C $project.ProjectRoot status --porcelain) }
$cleanPassed = -not $RequireClean -or $dirty.Count -eq 0
Add-FoundationCheck -Id 'VC.WORKTREE' -Category 'version-control' -Severity $(if ($RequireClean) { 'BLOCKER' } else { 'INFO' }) -Passed $cleanPassed -Diagnostic "Tracked changes: $($dirty.Count); RequireClean=$RequireClean." -Evidence @($dirty | Select-Object -First 20)

$snapshotParent = Join-Path (Split-Path -Parent $project.ProjectRoot) 'Snapshots'
$snapshots = @(Get-ChildItem -LiteralPath $snapshotParent -Directory -Filter 'Dev.D.UE.0.0.9B_Foundation_*' -ErrorAction SilentlyContinue)
Add-FoundationCheck -Id 'VC.SNAPSHOT' -Category 'version-control' -Severity 'BLOCKER' -Passed ($snapshots.Count -gt 0) -Diagnostic "Foundation snapshots found: $($snapshots.Count)." -Evidence @($snapshots.FullName)

$engineValid = $null -ne $project.EngineRoot
Add-FoundationCheck -Id 'UE.RESOLUTION' -Category 'invocation' -Severity 'BLOCKER' -Passed $engineValid -Diagnostic $(if ($engineValid) { "Resolved UE $($project.EngineAssociation): $($project.EngineRoot)" } else { 'Unreal Engine could not be resolved.' }) -Evidence @($project.Uproject)

$invokeScript = Join-Path $PSScriptRoot 'Invoke-Shanmen.ps1'
$moduleScript = Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1'
$projectLauncher = Join-Path $project.ProjectRoot 'LATEST_DEMO.bat'
$workspaceLauncher = Join-Path (Split-Path -Parent $project.ProjectRoot) 'latest demo.bat'
$launcherText = if (Test-Path -LiteralPath $projectLauncher) { Get-Content -LiteralPath $projectLauncher -Raw } else { '' }
$workspaceLauncherText = if (Test-Path -LiteralPath $workspaceLauncher) { Get-Content -LiteralPath $workspaceLauncher -Raw } else { '' }
$canonicalLauncher = (Test-Path -LiteralPath $invokeScript) -and
    (Test-Path -LiteralPath $moduleScript) -and
    $launcherText.Contains('Invoke-Shanmen.ps1') -and
    $workspaceLauncherText.Contains('LATEST_DEMO.bat')
Add-FoundationCheck -Id 'INVOCATION.CANONICAL' -Category 'invocation' -Severity 'BLOCKER' -Passed $canonicalLauncher -Diagnostic 'Workspace and project launchers must delegate to the canonical PowerShell entry.' -Evidence @($workspaceLauncher, $projectLauncher, $invokeScript, $moduleScript)

$developmentExeText = if (Test-Path -LiteralPath $workspaceLauncher) { $workspaceLauncherText } else { '' }
$noRawDevelopmentLaunch = -not $developmentExeText.Contains('Binaries\Win64\demo_map.exe')
Add-FoundationCheck -Id 'INVOCATION.NO_RAW_GAME_EXE' -Category 'invocation' -Severity 'BLOCKER' -Passed $noRawDevelopmentLaunch -Diagnostic 'The workspace launcher must not execute an uncooked Development game binary.' -Evidence @($workspaceLauncher)

$latestPlan = $null
try { $latestPlan = Get-ShanmenLatestLaunchPlan -Project $project }
catch { }
$latestPlanValid = $null -ne $latestPlan -and
    (Test-Path -LiteralPath $latestPlan.FilePath -PathType Leaf) -and
    $latestPlan.Mode -in 'PACKAGED_CANDIDATE', 'EDITOR_GAME_FALLBACK'
Add-FoundationCheck -Id 'INVOCATION.LATEST_PLAN' -Category 'invocation' -Severity 'BLOCKER' -Passed $latestPlanValid -Diagnostic $(if ($latestPlanValid) { "Resolved Latest launch mode: $($latestPlan.Mode); target: $($latestPlan.FilePath)" } else { 'Latest launch target could not be resolved without starting Unreal.' }) -Evidence @($(if ($latestPlan) { $latestPlan.FilePath }))

$entryDocumentation = @(
    (Join-Path $project.ProjectRoot 'PROJECT.md'),
    (Join-Path $project.ProjectRoot 'PROJECT_INFO_CARD.md'),
    (Join-Path $project.ProjectRoot 'Docs\Process\I_STAGE_FOUNDATION_GATE.md')
)
$rawExeDocumentation = @($entryDocumentation | Where-Object {
    (Test-Path -LiteralPath $_ -PathType Leaf) -and
    (Get-Content -LiteralPath $_ -Raw) -match '(?im)^\s*-\s*(源码构建后入口|当前入口|入口)\s*[:：].*Binaries\\Win64\\demo_map\.exe'
})
Add-FoundationCheck -Id 'INVOCATION.DOCS_NO_RAW_GAME_EXE' -Category 'invocation' -Severity 'BLOCKER' -Passed ($rawExeDocumentation.Count -eq 0) -Diagnostic "Authoritative entry documents pointing to the uncooked game EXE: $($rawExeDocumentation.Count)." -Evidence @($rawExeDocumentation)

$handoffScript = Join-Path $PSScriptRoot 'Update-HandoffLedger.ps1'
$policyDocument = Join-Path $project.ProjectRoot 'Docs\Process\I_STAGE_FOUNDATION_GATE.md'
Add-FoundationCheck -Id 'HANDOFF.LEDGER' -Category 'handoff' -Severity 'BLOCKER' -Passed (Test-Path -LiteralPath $handoffScript -PathType Leaf) -Diagnostic 'Prompt/Report transfer requires a durable transition ledger.' -Evidence @($handoffScript)
$handoffText = if (Test-Path -LiteralPath $handoffScript -PathType Leaf) { Get-Content -LiteralPath $handoffScript -Raw } else { '' }
$handoffStateMachine = $handoffText.Contains("'ResumeBlocked'") -and
    $handoffText.Contains("'ATTACHMENT_VISIBLE'") -and
    $handoffText.Contains("'NEXT_PROMPT_HASHED'") -and
    $handoffText.Contains('Attachment confirmation must exactly match the Report file')
Add-FoundationCheck -Id 'HANDOFF.STATE_MACHINE' -Category 'handoff' -Severity 'BLOCKER' -Passed $handoffStateMachine -Diagnostic 'The ledger must enforce exact attachment confirmation, resumable blocking, and next-Prompt acquisition.' -Evidence @($handoffScript)
Add-FoundationCheck -Id 'STAGE.I_GATE' -Category 'stage-policy' -Severity 'BLOCKER' -Passed (Test-Path -LiteralPath $policyDocument -PathType Leaf) -Diagnostic 'Every I stage must retain the project-level foundation audit gate.' -Evidence @($policyDocument)

$testFiles = @(Get-ChildItem -LiteralPath (Join-Path $project.ProjectRoot 'Source\demo_map') -File -Recurse -Filter '*Test*.cpp')
$unguarded = foreach ($file in $testFiles) {
    $head = (Get-Content -LiteralPath $file.FullName -TotalCount 20) -join "`n"
    if ($head -notmatch 'WITH_DEV_AUTOMATION_TESTS') { $file.FullName }
}
Add-FoundationCheck -Id 'BUILD.TEST_GUARDS' -Category 'build' -Severity 'BLOCKER' -Passed (@($unguarded).Count -eq 0) -Diagnostic "Test files: $($testFiles.Count); unguarded: $(@($unguarded).Count)." -Evidence @($unguarded)

$buildRules = Join-Path $project.ProjectRoot 'Source\demo_map\demo_map.Build.cs'
$buildRulesText = Get-Content -LiteralPath $buildRules -Raw
Add-FoundationCheck -Id 'BUILD.TEST_MODULE_DEBT' -Category 'architecture' -Severity 'WARNING' -Passed (-not $buildRulesText.Contains('bUseUnity = false')) -Diagnostic 'Automation still shares the Runtime module and globally disables Unity compilation; migrate in a bounded I stage.' -Evidence @($buildRules)

$sourceFiles = @(Get-ChildItem -LiteralPath (Join-Path $project.ProjectRoot 'Source\demo_map') -File -Recurse -Include '*.cpp', '*.h')
$largeSources = foreach ($file in $sourceFiles) {
    $lineCount = (Get-Content -LiteralPath $file.FullName | Measure-Object -Line).Lines
    if ($lineCount -gt 4000) { "$lineCount`t$($file.FullName)" }
}
Add-FoundationCheck -Id 'ARCH.MONOLITHS' -Category 'architecture' -Severity 'WARNING' -Passed (@($largeSources).Count -eq 0) -Diagnostic "Source files above 4,000 lines: $(@($largeSources).Count)." -Evidence @($largeSources)

$profileTypes = Join-Path $project.ProjectRoot 'Source\demo_map\demo_mapPersistentProfileTypes.cpp'
$profileStore = Join-Path $project.ProjectRoot 'Source\demo_map\demo_mapProfileRepository.cpp'
$profileTypesText = Get-Content -LiteralPath $profileTypes -Raw
$profileStoreText = Get-Content -LiteralPath $profileStore -Raw
$persistenceSafe = $profileTypesText.Contains('FPaths::ProjectSavedDir()') -and
    $profileStoreText.Contains('TempPath()') -and
    $profileStoreText.Contains('BackupPath()') -and
    $profileStoreText.Contains('IFileManager::Get().Move')
Add-FoundationCheck -Id 'PERSISTENCE.ATOMIC_BOUNDARY' -Category 'persistence' -Severity 'BLOCKER' -Passed $persistenceSafe -Diagnostic 'Production storage must remain under ProjectSavedDir with temp, backup, readback and replace semantics.' -Evidence @($profileTypes, $profileStore)

$controller = Join-Path $project.ProjectRoot 'Source\demo_map\demo_mapPlayerController.cpp'
$controllerText = Get-Content -LiteralPath $controller -Raw
$inputCentralized = $controllerText.Contains('ReconcileInputContext') -and
    $controllerText.Contains('FInputModeGameOnly') -and
    $controllerText.Contains('FInputModeGameAndUI') -and
    $controllerText.Contains('FInputModeUIOnly')
Add-FoundationCheck -Id 'INPUT.CONTEXT_RESOLVER' -Category 'input' -Severity 'BLOCKER' -Passed $inputCentralized -Diagnostic 'Gameplay/UI input modes must remain centralized and explicitly restorable.' -Evidence @($controller)

$engineConfig = Join-Path $project.ProjectRoot 'Config\DefaultEngine.ini'
$gameConfig = Join-Path $project.ProjectRoot 'Config\DefaultGame.ini'
$engineLines = Get-Content -LiteralPath $engineConfig
$duplicateRhi = @($engineLines | Where-Object { $_ -eq 'DefaultGraphicsRHI=DefaultGraphicsRHI_DX12' }).Count -gt 1
$tokenCommitted = @($engineLines | Where-Object { $_ -match '^SecurityToken=.+' }).Count -gt 0
$templateName = (Get-Content -LiteralPath $gameConfig -Raw).Contains('ProjectName=Top Down Game Template')
$configClean = -not $duplicateRhi -and -not $tokenCommitted -and -not $templateName
Add-FoundationCheck -Id 'CONFIG.HYGIENE' -Category 'configuration' -Severity 'BLOCKER' -Passed $configClean -Diagnostic "duplicate_rhi=$duplicateRhi; committed_android_token=$tokenCommitted; template_name=$templateName." -Evidence @($engineConfig, $gameConfig)

$invocationFiles = @(Get-ChildItem -LiteralPath $PSScriptRoot -File | Where-Object {
    $_.Extension -in '.ps1', '.psm1', '.bat'
})
$hardcodedEngine = @($invocationFiles | Where-Object {
    (Get-Content -LiteralPath $_.FullName -Raw) -match 'C:\\Program Files\\Epic Games\\UE_[0-9]'
})
Add-FoundationCheck -Id 'INVOCATION.LEGACY_ENGINE_PATHS' -Category 'invocation' -Severity 'WARNING' -Passed ($hardcodedEngine.Count -eq 0) -Diagnostic "Legacy scripts with hard-coded UE paths: $($hardcodedEngine.Count). Canonical entry is authoritative; migrate these by use." -Evidence @($hardcodedEngine | ForEach-Object { $_.FullName })

$staleTaskIds = @($invocationFiles | Where-Object {
    (Get-Content -LiteralPath $_.FullName -Raw) -match 'Dev\.D\.UE\.0\.0\.(8|9)(\.|-)'
})
Add-FoundationCheck -Id 'INVOCATION.STALE_TASK_IDS' -Category 'invocation' -Severity 'WARNING' -Passed ($staleTaskIds.Count -eq 0) -Diagnostic "Scripts with stale pre-0.0.9B task identifiers: $($staleTaskIds.Count)." -Evidence @($staleTaskIds | ForEach-Object { $_.FullName })

$directProcessScripts = @($invocationFiles | Where-Object {
    $_.Extension -eq '.ps1' -and
    (Get-Content -LiteralPath $_.FullName -Raw).Contains('Start-Process')
})
Add-FoundationCheck -Id 'INVOCATION.PROCESS_WRAPPER_DEBT' -Category 'invocation' -Severity 'WARNING' -Passed ($directProcessScripts.Count -eq 0) -Diagnostic "Legacy/specialized scripts still calling Start-Process outside the canonical tracked wrapper: $($directProcessScripts.Count). Migrate them in bounded I stages before F invokes them." -Evidence @($directProcessScripts | ForEach-Object { $_.FullName })

$blockers = @($checks | Where-Object { $_.severity -eq 'BLOCKER' -and -not $_.passed })
$warnings = @($checks | Where-Object { $_.severity -eq 'WARNING' -and -not $_.passed })
$status = if ($blockers.Count -gt 0) { 'FAIL' } elseif ($warnings.Count -gt 0) { 'PASS_WITH_DEBT' } else { 'PASS' }
$report = [ordered]@{
    schema_version = 1
    stage_id = $StageId
    generated_utc = (Get-Date).ToUniversalTime().ToString('o')
    project_root = $project.ProjectRoot
    status = $status
    blocker_count = $blockers.Count
    warning_count = $warnings.Count
    checks = @($checks | ForEach-Object { $_ })
}
$jsonPath = Join-Path $OutputRoot 'foundation-audit.json'
$markdownPath = Join-Path $OutputRoot 'foundation-audit.md'
Write-ShanmenJsonAtomic -Path $jsonPath -Value $report

$markdown = New-Object System.Collections.Generic.List[string]
$markdown.Add("# Foundation Audit — $StageId")
$markdown.Add('')
$markdown.Add("- Status: **$status**")
$markdown.Add("- Generated UTC: $($report.generated_utc)")
$markdown.Add("- Blockers: $($blockers.Count)")
$markdown.Add("- Warnings: $($warnings.Count)")
$markdown.Add('')
$markdown.Add('| Result | Severity | Category | Check | Diagnostic |')
$markdown.Add('|---|---|---|---|---|')
foreach ($check in $checks) {
    $result = if ($check.passed) { 'PASS' } else { 'FAIL' }
    $diagnostic = $check.diagnostic.Replace('|', '\|')
    $markdown.Add("| $result | $($check.severity) | $($check.category) | $($check.id) | $diagnostic |")
}
[IO.File]::WriteAllLines($markdownPath, $markdown, (New-Object Text.UTF8Encoding($false)))

Write-Host "Foundation audit: $status"
Write-Host "JSON: $jsonPath"
Write-Host "Markdown: $markdownPath"
if ($blockers.Count -gt 0) { exit 2 }
exit 0
