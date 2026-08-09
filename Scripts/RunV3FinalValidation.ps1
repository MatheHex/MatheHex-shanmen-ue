[CmdletBinding()]
param(
    [string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$BuildOutputRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) "Builds\demo_map"),
    [switch]$SkipBuild,
    [switch]$SkipPackage
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Stop-ProcessTree {
    param([int]$RootProcessId)
    $all = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue)
    $children = @{}
    foreach ($item in $all) {
        $parent = [int]$item.ParentProcessId
        if (-not $children.ContainsKey($parent)) { $children[$parent] = New-Object System.Collections.ArrayList }
        [void]$children[$parent].Add([int]$item.ProcessId)
    }
    $ordered = New-Object System.Collections.Generic.List[int]
    function Add-Descendants([int]$id) {
        if ($children.ContainsKey($id)) {
            foreach ($child in @($children[$id])) { Add-Descendants $child; $ordered.Add($child) }
        }
    }
    Add-Descendants $RootProcessId
    foreach ($id in $ordered) { Stop-Process -Id $id -Force -ErrorAction SilentlyContinue }
    Stop-Process -Id $RootProcessId -Force -ErrorAction SilentlyContinue
}

function Start-ManagedProcess {
    param(
        [string]$FilePath,
        [string[]]$ArgumentList,
        [string]$WorkingDirectory = '',
        [bool]$CreateNoWindow = $true,
        [bool]$RedirectOutput = $false
    )
    $actualFile = $FilePath
    $actualArguments = $ArgumentList -join ' '
    if ([System.IO.Path]::GetExtension($FilePath) -ieq '.bat') {
        $actualFile = Join-Path $env:SystemRoot 'System32\cmd.exe'
        $actualArguments = '/d /s /c ""{0}" {1}"' -f $FilePath, $actualArguments
    }
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $actualFile
    $info.Arguments = $actualArguments
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $CreateNoWindow
    if ($WorkingDirectory) { $info.WorkingDirectory = $WorkingDirectory }
    if ($RedirectOutput) {
        $info.RedirectStandardOutput = $true
        $info.RedirectStandardError = $true
    }
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $info
    [void]$process.Start()
    return $process
}

function Invoke-CapturedProcess {
    param(
        [string]$FilePath,
        [string[]]$ArgumentList,
        [string]$LogPath,
        [int]$TimeoutSeconds,
        [string]$WorkingDirectory = ''
    )
    $start = Get-Date
    $process = Start-ManagedProcess -FilePath $FilePath -ArgumentList $ArgumentList -WorkingDirectory $WorkingDirectory -RedirectOutput $true
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $timedOut = -not $process.WaitForExit($TimeoutSeconds * 1000)
    if ($timedOut) { Stop-ProcessTree -RootProcessId $process.Id; $process.WaitForExit() }
    $process.WaitForExit()
    $content = @($stdoutTask.Result, $stderrTask.Result) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    $content | Set-Content -LiteralPath $LogPath -Encoding UTF8
    [pscustomobject]@{
        exit_code = if ($timedOut) { 124 } else { $process.ExitCode }
        timed_out = $timedOut
        started_at = $start.ToString('o')
        ended_at = (Get-Date).ToString('o')
        duration_seconds = [math]::Round(((Get-Date) - $start).TotalSeconds, 3)
        log_path = $LogPath
        command = ('"{0}" {1}' -f $FilePath, ($ArgumentList -join ' '))
    }
}

function Get-TextCount {
    param([string]$Text, [string]$Needle)
    if ([string]::IsNullOrEmpty($Needle)) { return 0 }
    return ([regex]::Matches($Text, [regex]::Escape($Needle))).Count
}

function Complete-StrictResult {
    param(
        $Test,
        $ProcessResult,
        [string]$EvidenceLog,
        [bool]$AllowMultiplePass = $false
    )
    $text = if (Test-Path -LiteralPath $EvidenceLog) { Get-Content -LiteralPath $EvidenceLog -Raw } else { '' }
    $passCount = Get-TextCount -Text $text -Needle ([string]$Test.required_pass_marker)
    $failCount = Get-TextCount -Text $text -Needle ([string]$Test.required_fail_marker)
    $fatalCount = ([regex]::Matches($text, '(?im)Fatal error:|Assertion failed|Unhandled Exception|Access Violation')).Count
    $missingCount = ([regex]::Matches($text, '(?im)Missing Asset')).Count
    $projectErrorCount = ([regex]::Matches($text, '(?im)^.*Logdemo_map: Error:')).Count
    $markerOk = if ($AllowMultiplePass) { $passCount -gt 0 } else { $passCount -eq 1 }
    $passed = $ProcessResult.exit_code -eq [int]$Test.required_exit_code -and -not $ProcessResult.timed_out -and $markerOk -and $failCount -eq 0 -and $fatalCount -eq 0 -and $missingCount -eq 0 -and $projectErrorCount -eq 0
    [pscustomobject]@{
        test_id = [string]$Test.test_id
        category = [string]$Test.category
        status = if ($passed) { 'passed' } else { 'failed' }
        critical = [bool]$Test.critical
        command = $ProcessResult.command
        exit_code = $ProcessResult.exit_code
        required_exit_code = [int]$Test.required_exit_code
        timed_out = $ProcessResult.timed_out
        pass_marker = [string]$Test.required_pass_marker
        pass_marker_count = $passCount
        fail_marker_count = $failCount
        fatal_assertion_unhandled_count = $fatalCount
        missing_asset_count = $missingCount
        project_error_count = $projectErrorCount
        started_at = $ProcessResult.started_at
        ended_at = $ProcessResult.ended_at
        duration_seconds = $ProcessResult.duration_seconds
        log_path = $EvidenceLog
    }
}

function Test-PngSet {
    param([string]$Directory)
    Add-Type -AssemblyName System.Drawing
    $required = @(
        'V3_FINAL_CLOSED_CHEST_LABELS.png',
        'V3_FINAL_OPENED_CHEST_LOOT.png',
        'V3_FINAL_INVENTORY_EQUIPMENT_ATTRIBUTES.png',
        'V3_FINAL_CLOSE_RANGE_PROJECTILE.png',
        'V3_FINAL_ENEMY_DROPS.png',
        'V3_FINAL_DEATH_SETTLEMENT.png',
        'V3_FINAL_EXTRACTION_STASH_NEW_RUN.png',
        'V3_FINAL_ABANDON_SETTLEMENT.png'
    )
    $results = New-Object System.Collections.ArrayList
    foreach ($name in $required) {
        $path = Join-Path $Directory $name
        $exists = Test-Path -LiteralPath $path -PathType Leaf
        $width = 0; $height = 0; $bytes = 0L; $valid = $false
        if ($exists) {
            $bytes = (Get-Item -LiteralPath $path).Length
            $bitmap = New-Object System.Drawing.Bitmap($path)
            try { $width = $bitmap.Width; $height = $bitmap.Height } finally { $bitmap.Dispose() }
            $valid = $bytes -gt 0 -and $width -eq 1280 -and $height -eq 720
        }
        [void]$results.Add([pscustomobject]@{ name=$name; path=$path; exists=$exists; bytes=$bytes; width=$width; height=$height; valid=$valid })
    }
    [pscustomobject]@{ passed=(@($results | Where-Object { -not $_.valid }).Count -eq 0); count=@($results | Where-Object valid).Count; files=@($results) }
}

$runnerStart = Get-Date
$ProjectRoot = [System.IO.Path]::GetFullPath($ProjectRoot)
$EngineRoot = [System.IO.Path]::GetFullPath($EngineRoot)
$BuildOutputRoot = [System.IO.Path]::GetFullPath($BuildOutputRoot)
$projectFile = Join-Path $ProjectRoot 'demo_map.uproject'
$manifestPath = Join-Path $ProjectRoot 'Scripts\V3_TEST_MANIFEST.json'
$buildBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$uatBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
foreach ($path in @($projectFile,$manifestPath,$buildBat,$uatBat,$editorCmd)) {
    if (-not (Test-Path -LiteralPath $path)) { throw "Required path missing: $path" }
}

$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
$tests = @($manifest.tests)
if ($tests.Count -ne 18 -or @($tests.test_id | Select-Object -Unique).Count -ne 18) { throw 'Manifest must contain 18 unique tests.' }

$busy = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue | Where-Object {
    $_.Name -match '^(UnrealEditor|UnrealEditor-Cmd|demo_map|UnrealBuildTool|AutomationTool|ShaderCompileWorker)\.exe$' -and $_.CommandLine -like "*$ProjectRoot*"
})
if ($busy.Count -gt 0) { throw "Active project process found: $($busy.ProcessId -join ', ')" }

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$runRoot = Join-Path $ProjectRoot "Saved\Automation\0.3.5.0\Runs\$stamp"
$logRoot = Join-Path $runRoot 'Logs'
$fixedRoot = Join-Path $ProjectRoot 'Saved\Automation\0.3.5.0'
$screenshotRoot = Join-Path $fixedRoot 'Screenshots'
$packageCandidateRoot = Join-Path $BuildOutputRoot "$stamp-0.3.5.0"
$packageArchiveRoot = Join-Path $packageCandidateRoot 'Package'
New-Item -ItemType Directory -Path $logRoot,$screenshotRoot,$packageArchiveRoot -Force | Out-Null
Get-ChildItem -LiteralPath $screenshotRoot -Filter '*.png' -File -ErrorAction SilentlyContinue | Remove-Item -Force

$builds = New-Object System.Collections.ArrayList
if (-not $SkipBuild) {
    foreach ($target in @('demo_mapEditor','demo_map')) {
        $id = if ($target -eq 'demo_mapEditor') { 'editor_build' } else { 'game_build' }
        $log = Join-Path $logRoot "$id.log"
        $args = @($target,'Win64','Development',("-Project=`"{0}`"" -f $projectFile),'-WaitMutex','-NoHotReloadFromIDE','-NoUBA','-MaxParallelActions=1')
        $p = Invoke-CapturedProcess -FilePath $buildBat -ArgumentList $args -LogPath $log -TimeoutSeconds 1800
        $text = Get-Content -LiteralPath $log -Raw
        $ok = $p.exit_code -eq 0 -and $text -match 'Result: Succeeded' -and $text -notmatch '(?im)error C\d{4}:|fatal error:|BUILD FAILED'
        [void]$builds.Add([pscustomobject]@{ id=$id; status=if($ok){'passed'}else{'failed'}; exit_code=$p.exit_code; result_succeeded=($text -match 'Result: Succeeded'); command=$p.command; started_at=$p.started_at; ended_at=$p.ended_at; duration_seconds=$p.duration_seconds; log_path=$log })
        if (-not $ok) { throw "$id failed: $log" }
    }
} else { [void]$builds.Add([pscustomobject]@{id='editor_build';status='skipped'}); [void]$builds.Add([pscustomobject]@{id='game_build';status='skipped'}) }

$testResults = New-Object System.Collections.ArrayList
$native = $tests | Where-Object test_id -eq 'native_v3'
$nativeLog = Join-Path $logRoot 'native_v3.log'
$nativeStdout = Join-Path $logRoot 'native_v3_stdout.log'
$nativeArgs = @(("`"{0}`"" -f $projectFile),'/Game/DemoV3/Maps/L_V3_ProgressionDemo','-unattended','-nop4','-nosplash','-nullrhi','-nosound','-ExecCmds="Automation RunTests demo_map.V3;Quit"','-TestExit="Automation Test Queue Empty"',("-AbsLog=`"{0}`"" -f $nativeLog))
$nativeProcess = Invoke-CapturedProcess -FilePath $editorCmd -ArgumentList $nativeArgs -LogPath $nativeStdout -TimeoutSeconds ([int]$native.timeout_seconds)
[void]$testResults.Add((Complete-StrictResult -Test $native -ProcessResult $nativeProcess -EvidenceLog $nativeLog -AllowMultiplePass $true))

foreach ($test in @($tests | Where-Object category -eq 'v2_regression')) {
    $log = Join-Path $logRoot "$($test.test_id).log"
    $stdout = Join-Path $logRoot "$($test.test_id)_stdout.log"
    $args = @(("`"{0}`"" -f $projectFile),[string]$test.map,'-game',[string]$test.command,'-unattended','-nop4','-nosplash','-nullrhi','-nosound',("-AbsLog=`"{0}`"" -f $log))
    $p = Invoke-CapturedProcess -FilePath $editorCmd -ArgumentList $args -LogPath $stdout -TimeoutSeconds ([int]$test.timeout_seconds)
    [void]$testResults.Add((Complete-StrictResult -Test $test -ProcessResult $p -EvidenceLog $log))
}

$uatLog = Join-Path $logRoot 'build_cook_run.log'
if (-not $SkipPackage) {
	$uatArgs = @('BuildCookRun',("-project=`"{0}`"" -f $projectFile),'-noP4','-platform=Win64','-clientconfig=Development','-build','-cook','-map=/Game/DemoV3/Maps/L_V3_ProgressionDemo','-stage','-package','-pak','-iostore','-archive',("-archivedirectory=`"{0}`"" -f $packageArchiveRoot),'-utf8output','-UbtArgs="-NoUBA -MaxParallelActions=1"')
	$uat = Invoke-CapturedProcess -FilePath $uatBat -ArgumentList $uatArgs -LogPath $uatLog -TimeoutSeconds 3600
	$uatText = Get-Content -LiteralPath $uatLog -Raw
	if ($uat.exit_code -ne 0 -and $uatText -match 'Failed reading oplog from Zen') {
		$firstAttemptLog = Join-Path $logRoot 'build_cook_run_zen_failed_attempt1.log'
		Move-Item -LiteralPath $uatLog -Destination $firstAttemptLog
		Start-Sleep -Seconds 2
		$uat = Invoke-CapturedProcess -FilePath $uatBat -ArgumentList $uatArgs -LogPath $uatLog -TimeoutSeconds 3600
		$uatText = Get-Content -LiteralPath $uatLog -Raw
	}
    $uatOk = $uat.exit_code -eq 0 -and $uatText -match 'BUILD SUCCESSFUL' -and $uatText -notmatch '(?im)BUILD FAILED|Fatal error:|Assertion failed|Unhandled Exception|Missing Asset|^.*Logdemo_map: Error:'
    [void]$builds.Add([pscustomobject]@{id='build_cook_run';status=if($uatOk){'passed'}else{'failed'};exit_code=$uat.exit_code;build_successful=($uatText -match 'BUILD SUCCESSFUL');command=$uat.command;started_at=$uat.started_at;ended_at=$uat.ended_at;duration_seconds=$uat.duration_seconds;log_path=$uatLog})
    if (-not $uatOk) { throw "BuildCookRun failed: $uatLog" }
} else { [void]$builds.Add([pscustomobject]@{id='build_cook_run';status='skipped'}) }

$packageExe = Join-Path $packageArchiveRoot 'Windows\demo_map.exe'
$gameExe = Join-Path $packageArchiveRoot 'Windows\demo_map\Binaries\Win64\demo_map-Win64-Shipping.exe'
if (-not (Test-Path -LiteralPath $gameExe)) { $gameExe = Join-Path $packageArchiveRoot 'Windows\demo_map\Binaries\Win64\demo_map.exe' }
if (-not (Test-Path -LiteralPath $packageExe)) { throw "Package bootstrap missing: $packageExe" }
if (-not (Test-Path -LiteralPath $gameExe)) { throw "Package game executable missing: $gameExe" }

foreach ($test in @($tests | Where-Object package_required)) {
    $log = Join-Path $logRoot ([System.IO.Path]::GetFileName(([string]$test.log_path).Replace('{run_root}', $runRoot)))
    if ($test.test_id -eq 'v3_final_visible') { $log = Join-Path $logRoot 'package_v3_final_visible.log' }
    $args = @([string]$test.map,[string]$test.command,'-unattended','-nop4','-nosplash','-nosound','-log',("-AbsLog=`"{0}`"" -f $log))
    $createNoWindow = $true
    if ($test.screenshot_required) {
        $args += @('-windowed','-ResX=1280','-ResY=720',("-V3FinalVisualOutput=`"{0}`"" -f $screenshotRoot))
        $createNoWindow = $false
    } else { $args += '-nullrhi' }
    $start = Get-Date
    $process = Start-ManagedProcess -FilePath $packageExe -ArgumentList $args -WorkingDirectory (Split-Path -Parent $packageExe) -CreateNoWindow $createNoWindow
    $timedOut = -not $process.WaitForExit(([int]$test.timeout_seconds) * 1000)
    if ($timedOut) { Stop-ProcessTree -RootProcessId $process.Id; $process.WaitForExit() }
    $process.WaitForExit()
    $p = [pscustomobject]@{exit_code=if($timedOut){124}else{$process.ExitCode};timed_out=$timedOut;started_at=$start.ToString('o');ended_at=(Get-Date).ToString('o');duration_seconds=[math]::Round(((Get-Date)-$start).TotalSeconds,3);command=('"{0}" {1}' -f $packageExe,($args -join ' '))}
    [void]$testResults.Add((Complete-StrictResult -Test $test -ProcessResult $p -EvidenceLog $log))
}

$png = Test-PngSet -Directory $screenshotRoot
$packageFiles = @(Get-ChildItem -LiteralPath (Join-Path $packageArchiveRoot 'Windows') -File -Recurse)
$nativeText = if (Test-Path -LiteralPath $nativeLog) { Get-Content -LiteralPath $nativeLog -Raw } else { '' }
$nativeNames = @([regex]::Matches($nativeText,'(?m)Test Started\. Name=\{([^}]+)\}') | ForEach-Object { $_.Groups[1].Value } | Select-Object -Unique)
$criticalFailures = @($testResults | Where-Object { $_.critical -and $_.status -ne 'passed' })
$status = if ($criticalFailures.Count -eq 0 -and $png.passed -and @($builds | Where-Object status -eq 'failed').Count -eq 0) { 'passed' } else { 'failed' }
$result = [ordered]@{
    task_id = '0.3.5.0'
    status = $status
    started_at = $runnerStart.ToString('o')
    ended_at = (Get-Date).ToString('o')
    duration_seconds = [math]::Round(((Get-Date)-$runnerStart).TotalSeconds,3)
    project_root = $ProjectRoot
    engine_root = $EngineRoot
    manifest_path = $manifestPath
    run_root = $runRoot
    builds = @($builds)
    tests = @($testResults)
    native_v3 = [ordered]@{ discovered_count=$nativeNames.Count; success_marker_count=(Get-TextCount $nativeText ([string]$native.required_pass_marker)); names=$nativeNames }
    package = [ordered]@{
        candidate_root = $packageCandidateRoot
        archive_root = $packageArchiveRoot
        bootstrap_exe = $packageExe
        game_exe = $gameExe
        file_count = $packageFiles.Count
        total_bytes = [long](($packageFiles | Measure-Object Length -Sum).Sum)
        bootstrap_sha256 = (Get-FileHash -LiteralPath $packageExe -Algorithm SHA256).Hash
        game_sha256 = (Get-FileHash -LiteralPath $gameExe -Algorithm SHA256).Hash
    }
    screenshots = $png
    critical_failures = @($criticalFailures | ForEach-Object { $_.test_id })
}
$uniqueResult = Join-Path $runRoot 'V3_FINAL_RESULT.json'
$fixedResult = Join-Path $fixedRoot 'V3_FINAL_RESULT.json'
$result | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $uniqueResult -Encoding UTF8
$result | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $fixedResult -Encoding UTF8
Write-Host "V3 final result: $fixedResult"
Write-Host "Final candidate: $packageExe"
Write-Host "Status: $status"
if ($status -ne 'passed') { exit 1 }
exit 0
