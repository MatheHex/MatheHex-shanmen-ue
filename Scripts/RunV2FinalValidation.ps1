[CmdletBinding()]
param(
    [string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [ValidateRange(1, 2)][int]$MaxParallelHeadless = 2,
    [switch]$SkipBuild,
    [switch]$SkipPackage,
    [string]$OutputRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) "Builds\demo_map")
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Stop-ProcessTree {
    param([int]$RootProcessId)
    $all = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue)
    $children = @{}
    foreach ($process in $all) {
        $parent = [int]$process.ParentProcessId
        if (-not $children.ContainsKey($parent)) { $children[$parent] = New-Object System.Collections.ArrayList }
        [void]$children[$parent].Add([int]$process.ProcessId)
    }
    $ordered = New-Object System.Collections.Generic.List[int]
    function Add-Children([int]$id) {
        if ($children.ContainsKey($id)) {
            foreach ($child in @($children[$id])) { Add-Children $child; $ordered.Add($child) }
        }
    }
    Add-Children $RootProcessId
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
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $actualFile
    $startInfo.Arguments = $actualArguments
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $CreateNoWindow
    if (-not [string]::IsNullOrWhiteSpace($WorkingDirectory)) { $startInfo.WorkingDirectory = $WorkingDirectory }
    if ($RedirectOutput) {
        $startInfo.RedirectStandardOutput = $true
        $startInfo.RedirectStandardError = $true
    }
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    [void]$process.Start()
    return $process
}

function Invoke-ProcessWithTimeout {
    param(
        [string]$FilePath,
        [string[]]$ArgumentList,
        [string]$LogPath,
        [int]$TimeoutSeconds
    )
    $startedAt = Get-Date
    $process = Start-ManagedProcess -FilePath $FilePath -ArgumentList $ArgumentList -CreateNoWindow $true -RedirectOutput $true
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $timedOut = -not $process.WaitForExit($TimeoutSeconds * 1000)
    if ($timedOut) {
        Stop-ProcessTree -RootProcessId $process.Id
        $process.WaitForExit()
    }
    $process.WaitForExit()
    $exitCode = if ($timedOut) { 124 } else { $process.ExitCode }
    $stdout = $stdoutTask.Result
    $stderr = $stderrTask.Result
    @($stdout, $stderr) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Set-Content -LiteralPath $LogPath -Encoding UTF8
    [pscustomobject]@{
        exit_code = $exitCode
        timed_out = $timedOut
        duration_seconds = [math]::Round(((Get-Date) - $startedAt).TotalSeconds, 3)
        log_path = $LogPath
    }
}

function Get-LogFailureCounts {
    param([string]$LogPath, [string]$ExpectedPass, [string]$ExpectedFail)
    $text = if (Test-Path -LiteralPath $LogPath) { Get-Content -Raw -LiteralPath $LogPath } else { '' }
    [pscustomobject]@{
        pass_count = ([regex]::Matches($text, [regex]::Escape($ExpectedPass))).Count
        fail_count = ([regex]::Matches($text, [regex]::Escape($ExpectedFail))).Count
        fatal_count = ([regex]::Matches($text, '(?im)Fatal error:')).Count
        assertion_count = ([regex]::Matches($text, '(?im)Assertion failed')).Count
        unhandled_count = ([regex]::Matches($text, '(?im)Unhandled Exception')).Count
        access_violation_count = ([regex]::Matches($text, '(?im)Access Violation')).Count
    }
}

function Complete-TestResult {
    param($Test, [int]$ExitCode, [bool]$TimedOut, [string]$LogPath, [datetime]$StartedAt, [double]$DurationSeconds, [long]$PeakWorkingSetBytes)
    $counts = Get-LogFailureCounts -LogPath $LogPath -ExpectedPass $Test.expected_pass_marker -ExpectedFail $Test.expected_fail_marker
    $fresh = (Test-Path -LiteralPath $LogPath) -and ((Get-Item -LiteralPath $LogPath).LastWriteTime -ge $StartedAt.AddSeconds(-2))
    $passed = $ExitCode -eq 0 -and -not $TimedOut -and $fresh -and $counts.pass_count -eq 1 -and $counts.fail_count -eq 0 -and $counts.fatal_count -eq 0 -and $counts.assertion_count -eq 0 -and $counts.unhandled_count -eq 0 -and $counts.access_violation_count -eq 0
    [pscustomobject]@{
        id = [string]$Test.id
        command_flag = [string]$Test.command_flag
        category = [string]$Test.category
        critical = [bool]$Test.critical
        status = if ($passed) { 'passed' } else { 'failed' }
        exit_code = $ExitCode
        timed_out = $TimedOut
        fresh_log = $fresh
        pass_marker_count = $counts.pass_count
        fail_marker_count = $counts.fail_count
        fatal_count = $counts.fatal_count
        assertion_count = $counts.assertion_count
        unhandled_exception_count = $counts.unhandled_count
        access_violation_count = $counts.access_violation_count
        duration_seconds = [math]::Round($DurationSeconds, 3)
        peak_working_set_bytes = $PeakWorkingSetBytes
        log_path = $LogPath
    }
}

function Test-PngSet {
    param([string]$Directory, [int]$ExpectedCount)
    Add-Type -AssemblyName System.Drawing
    $files = @(Get-ChildItem -LiteralPath $Directory -Filter *.png -File -ErrorAction SilentlyContinue | Sort-Object Name)
    $invalid = New-Object System.Collections.Generic.List[string]
    foreach ($file in $files) {
        if ($file.Length -le 0) { $invalid.Add("$($file.Name):empty"); continue }
        $bitmap = New-Object System.Drawing.Bitmap($file.FullName)
        try {
            if ($bitmap.Width -ne 1280 -or $bitmap.Height -ne 720) { $invalid.Add("$($file.Name):$($bitmap.Width)x$($bitmap.Height)"); continue }
            $nonBlack = $false
            for ($x = 32; $x -lt $bitmap.Width -and -not $nonBlack; $x += 96) {
                for ($y = 24; $y -lt $bitmap.Height; $y += 72) {
                    $pixel = $bitmap.GetPixel($x, $y)
                    if (($pixel.R + $pixel.G + $pixel.B) -gt 12) { $nonBlack = $true; break }
                }
            }
            if (-not $nonBlack) { $invalid.Add("$($file.Name):black") }
        } finally { $bitmap.Dispose() }
    }
    [pscustomobject]@{ passed = ($files.Count -eq $ExpectedCount -and $invalid.Count -eq 0); count = $files.Count; invalid = @($invalid); files = @($files.Name) }
}

function Write-SummaryFiles {
    param($Summary, [string]$BuildRoot)
    $jsonPath = Join-Path $BuildRoot 'V2_FINAL_TEST_SUMMARY.json'
    $mdPath = Join-Path $BuildRoot 'V2_FINAL_TEST_SUMMARY.md'
    $Summary | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add('# V2 Final Test Summary')
    $lines.Add('')
    $lines.Add("- Status: $($Summary.status)")
    $lines.Add("- Build root: ``$BuildRoot``")
    $lines.Add("- Package exe: ``$($Summary.package_exe)``")
    $lines.Add("- Total duration seconds: $($Summary.total_duration_seconds)")
    $lines.Add('')
    $lines.Add('| Test | Status | Exit | PASS markers | FAIL markers | Seconds |')
    $lines.Add('|---|---:|---:|---:|---:|---:|')
    foreach ($test in @($Summary.tests)) { $lines.Add("| $($test.id) | $($test.status) | $($test.exit_code) | $($test.pass_marker_count) | $($test.fail_marker_count) | $($test.duration_seconds) |") }
    $lines.Add('')
    $lines.Add("- Visual screenshots: $($Summary.visual_validation.count)")
    $lines.Add("- Critical failures: $(@($Summary.critical_failures).Count)")
    $lines | Set-Content -LiteralPath $mdPath -Encoding UTF8
    [pscustomobject]@{ json = $jsonPath; markdown = $mdPath }
}

$runnerStart = Get-Date
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$derivedProjectRoot = Split-Path -Parent $scriptRoot
if (-not $PSBoundParameters.ContainsKey('ProjectRoot')) { $ProjectRoot = $derivedProjectRoot }
$ProjectRoot = [System.IO.Path]::GetFullPath($ProjectRoot)
$EngineRoot = [System.IO.Path]::GetFullPath($EngineRoot)
$projectFile = Join-Path $ProjectRoot 'demo_map.uproject'
$manifestPath = Join-Path $ProjectRoot 'Docs\0.2.0版本开发档案\0.2.6.0_TEST_MANIFEST.json'
$buildBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$uatBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'

foreach ($required in @($projectFile, $manifestPath, $buildBat, $uatBat, $editorCmd)) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Required path missing: $required" }
}
$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
$enabledTests = @($manifest.tests | Where-Object enabled)
if ($enabledTests.Count -lt 10 -or @($enabledTests.id | Select-Object -Unique).Count -ne $enabledTests.Count) { throw 'V2 test manifest is incomplete or contains duplicate ids.' }

$busy = @(Get-CimInstance Win32_Process | Where-Object { $_.Name -match '^(UnrealEditor|UnrealEditor-Cmd|demo_map)\.exe$' -and $_.CommandLine -like "*$ProjectRoot*" })
if ($busy.Count -gt 0) { throw "Project is occupied by an Unreal process: $($busy.ProcessId -join ', ')" }

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$buildRoot = Join-Path ([System.IO.Path]::GetFullPath($OutputRoot)) $stamp
$packageRoot = Join-Path $buildRoot 'Package'
$tempLogs = Join-Path $buildRoot 'TempLogs'
$visualDir = Join-Path $tempLogs 'VisualAcceptance\V2FinalVisibleAcceptance'
New-Item -ItemType Directory -Path $packageRoot, $tempLogs, $visualDir -Force | Out-Null

$buildResults = [ordered]@{}
if ($SkipBuild) { $buildResults.editor = [pscustomobject]@{ status='skipped'; exit_code=0 }; $buildResults.game = [pscustomobject]@{ status='skipped'; exit_code=0 } }
else {
    $editorBuild = Invoke-ProcessWithTimeout -FilePath $buildBat -ArgumentList @('demo_mapEditor','Win64','Development',("-Project=`"{0}`"" -f $projectFile),'-WaitMutex','-NoHotReloadFromIDE') -LogPath (Join-Path $tempLogs 'build_editor.log') -TimeoutSeconds 1200
    $buildResults.editor = [pscustomobject]@{ status=if($editorBuild.exit_code -eq 0){'passed'}else{'failed'}; exit_code=$editorBuild.exit_code; duration_seconds=$editorBuild.duration_seconds; log_path=$editorBuild.log_path }
    if ($editorBuild.exit_code -ne 0) { throw "Editor build failed. See $($editorBuild.log_path)" }
    $gameBuild = Invoke-ProcessWithTimeout -FilePath $buildBat -ArgumentList @('demo_map','Win64','Development',("-Project=`"{0}`"" -f $projectFile),'-WaitMutex','-NoHotReloadFromIDE') -LogPath (Join-Path $tempLogs 'build_game.log') -TimeoutSeconds 1200
    $buildResults.game = [pscustomobject]@{ status=if($gameBuild.exit_code -eq 0){'passed'}else{'failed'}; exit_code=$gameBuild.exit_code; duration_seconds=$gameBuild.duration_seconds; log_path=$gameBuild.log_path }
    if ($gameBuild.exit_code -ne 0) { throw "Game build failed. See $($gameBuild.log_path)" }
}

$smokeLog = Join-Path $tempLogs 'editor_cmd_smoke.log'
$smoke = Invoke-ProcessWithTimeout -FilePath $editorCmd -ArgumentList @(("`"{0}`"" -f $projectFile),'/Game/DemoV2/Maps/L_V2_CombatDemo','-game','-unattended','-nop4','-nosplash','-nullrhi','-nosound','-ExecCmds=quit',("-AbsLog=`"{0}`"" -f $smokeLog)) -LogPath (Join-Path $tempLogs 'editor_cmd_smoke_stdout.log') -TimeoutSeconds 180
$smokeText = if (Test-Path -LiteralPath $smokeLog) { Get-Content -Raw -LiteralPath $smokeLog } else { '' }
$smokePassed = $smoke.exit_code -eq 0 -and $smokeText -match 'L_V2_CombatDemo' -and $smokeText -notmatch '(?im)Fatal error:|Assertion failed|Unhandled Exception|Access Violation'
$buildResults.editor_cmd_smoke = [pscustomobject]@{ status=if($smokePassed){'passed'}else{'failed'}; exit_code=$smoke.exit_code; duration_seconds=$smoke.duration_seconds; log_path=$smokeLog }
if (-not $smokePassed) { throw "Editor-Cmd smoke failed. See $smokeLog" }

$buildCookRunSeconds = 0.0
if ($SkipPackage) { $buildResults.package = [pscustomobject]@{ status='skipped'; exit_code=0 } }
else {
    $uatLog = Join-Path $tempLogs 'BuildCookRun.log'
    $uat = Invoke-ProcessWithTimeout -FilePath $uatBat -ArgumentList @('BuildCookRun',("-project=`"{0}`"" -f $projectFile),'-noP4','-platform=Win64','-clientconfig=Development','-build','-cook','-map=/Game/DemoV2/Maps/L_V2_CombatDemo','-stage','-package','-pak','-iostore','-archive',("-archivedirectory=`"{0}`"" -f $packageRoot),'-utf8output') -LogPath $uatLog -TimeoutSeconds 2400
    $buildCookRunSeconds = $uat.duration_seconds
    $buildResults.package = [pscustomobject]@{ status=if($uat.exit_code -eq 0){'passed'}else{'failed'}; exit_code=$uat.exit_code; duration_seconds=$uat.duration_seconds; log_path=$uatLog }
    if ($uat.exit_code -ne 0) { throw "BuildCookRun failed. See $uatLog" }
}

$packageExe = Join-Path $packageRoot 'Windows\demo_map.exe'
if (-not (Test-Path -LiteralPath $packageExe) -or (Get-Item -LiteralPath $packageExe).Length -le 0) { throw "Final package executable missing: $packageExe" }

$headless = @($enabledTests | Where-Object { -not $_.requires_rendering })
$pending = New-Object System.Collections.Queue
foreach ($test in $headless) { $pending.Enqueue($test) }
$running = New-Object System.Collections.ArrayList
$testResults = New-Object System.Collections.ArrayList

while ($pending.Count -gt 0 -or $running.Count -gt 0) {
    while ($pending.Count -gt 0 -and $running.Count -lt $MaxParallelHeadless) {
        $test = $pending.Dequeue()
        $log = Join-Path $tempLogs ("{0}.log" -f $test.id)
        if (Test-Path -LiteralPath $log) { throw "Refusing to reuse test log: $log" }
        $startedAt = Get-Date
        $arguments = @('/Game/DemoV2/Maps/L_V2_CombatDemo',("-{0}" -f $test.command_flag),'-nullrhi','-unattended','-nosplash','-nosound','-log',("-AbsLog={0}" -f $log))
        $process = Start-ManagedProcess -FilePath $packageExe -ArgumentList $arguments -WorkingDirectory (Split-Path -Parent $packageExe) -CreateNoWindow $true
        [void]$running.Add([pscustomobject]@{ Test=$test; Process=$process; Log=$log; StartedAt=$startedAt; Deadline=$startedAt.AddSeconds([int]$test.timeout_seconds); PeakWorkingSetBytes=0L })
    }
    Start-Sleep -Milliseconds 250
    foreach ($entry in @($running)) {
        $entry.Process.Refresh()
        $matchingProcesses = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue | Where-Object { $_.Name -eq 'demo_map.exe' -and $_.CommandLine -like "*$($entry.Log)*" })
        $workingSetSum = [long](($matchingProcesses | Measure-Object WorkingSetSize -Sum).Sum)
        if ($workingSetSum -gt $entry.PeakWorkingSetBytes) { $entry.PeakWorkingSetBytes = $workingSetSum }
        $timedOut = (Get-Date) -gt $entry.Deadline -and -not $entry.Process.HasExited
        if ($timedOut) { Stop-ProcessTree -RootProcessId $entry.Process.Id; $entry.Process.WaitForExit() }
        if ($entry.Process.HasExited -or $timedOut) {
            $exitCode = if ($timedOut) { 124 } else { $entry.Process.ExitCode }
            $peak = [long]$entry.PeakWorkingSetBytes
            $result = Complete-TestResult -Test $entry.Test -ExitCode $exitCode -TimedOut $timedOut -LogPath $entry.Log -StartedAt $entry.StartedAt -DurationSeconds (((Get-Date)-$entry.StartedAt).TotalSeconds) -PeakWorkingSetBytes $peak
            [void]$testResults.Add($result)
            [void]$running.Remove($entry)
        }
    }
}

foreach ($test in @($enabledTests | Where-Object requires_rendering)) {
    $log = Join-Path $tempLogs ("{0}.log" -f $test.id)
    $startedAt = Get-Date
    $arguments = @('/Game/DemoV2/Maps/L_V2_CombatDemo',("-{0}" -f $test.command_flag),("-V2FinalVisualOutput={0}" -f $visualDir),'-windowed','-ResX=1280','-ResY=720','-unattended','-nosplash','-nosound','-log',("-AbsLog={0}" -f $log))
    $process = Start-ManagedProcess -FilePath $packageExe -ArgumentList $arguments -WorkingDirectory (Split-Path -Parent $packageExe) -CreateNoWindow $false
    $peak = 0L
    $deadline = $startedAt.AddSeconds([int]$test.timeout_seconds)
    while (-not $process.HasExited -and (Get-Date) -le $deadline) {
        $matchingProcesses = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue | Where-Object { $_.Name -eq 'demo_map.exe' -and $_.CommandLine -like "*$log*" })
        $workingSetSum = [long](($matchingProcesses | Measure-Object WorkingSetSize -Sum).Sum)
        if ($workingSetSum -gt $peak) { $peak = $workingSetSum }
        Start-Sleep -Milliseconds 100
        $process.Refresh()
    }
    $timedOut = -not $process.HasExited
    if ($timedOut) { Stop-ProcessTree -RootProcessId $process.Id; $process.WaitForExit() }
    $result = Complete-TestResult -Test $test -ExitCode $(if($timedOut){124}else{$process.ExitCode}) -TimedOut $timedOut -LogPath $log -StartedAt $startedAt -DurationSeconds (((Get-Date)-$startedAt).TotalSeconds) -PeakWorkingSetBytes $peak
    [void]$testResults.Add($result)
}

$visualTest = $enabledTests | Where-Object id -eq 'v2_final_visible'
$visualValidation = Test-PngSet -Directory $visualDir -ExpectedCount ([int]$visualTest.expected_screenshot_count)
if (-not $visualValidation.passed) {
    $visibleResult = $testResults | Where-Object id -eq 'v2_final_visible'
    if ($visibleResult) { $visibleResult.status = 'failed' }
}

$packageFiles = @(Get-ChildItem -LiteralPath (Split-Path -Parent $packageExe) -Recurse -File)
$packageIndependence = [ordered]@{
    exe_nonzero = ((Get-Item -LiteralPath $packageExe).Length -gt 0)
    has_ucas = @($packageFiles | Where-Object Extension -eq '.ucas').Count -gt 0
    has_utoc = @($packageFiles | Where-Object Extension -eq '.utoc').Count -gt 0
    has_pak = @($packageFiles | Where-Object Extension -eq '.pak').Count -gt 0
    has_runtime_dll = @($packageFiles | Where-Object Extension -eq '.dll').Count -gt 0
    contains_source_directory = (Test-Path -LiteralPath (Join-Path (Split-Path -Parent $packageExe) 'Source'))
    contains_unreal_editor = @($packageFiles | Where-Object Name -eq 'UnrealEditor.exe').Count -gt 0
}
$packageIndependence.passed = $packageIndependence.exe_nonzero -and $packageIndependence.has_ucas -and $packageIndependence.has_utoc -and $packageIndependence.has_pak -and $packageIndependence.has_runtime_dll -and -not $packageIndependence.contains_source_directory -and -not $packageIndependence.contains_unreal_editor

$orderedResults = @($testResults | Sort-Object { [array]::IndexOf(@($enabledTests.id), $_.id) })
$criticalFailures = @($orderedResults | Where-Object { $_.critical -and $_.status -ne 'passed' })
if (-not $packageIndependence.passed) { $criticalFailures += 'package_independence' }
$summary = [ordered]@{
    stage = 'V2-FINAL'
    status = if ($criticalFailures.Count -eq 0) { 'passed' } else { 'failed' }
    generated_at = (Get-Date).ToString('o')
    project_root = $ProjectRoot
    engine_root = $EngineRoot
    build_root = $buildRoot
    package_exe = $packageExe
    manifest_path = $manifestPath
    build = $buildResults
    build_cook_run_seconds = $buildCookRunSeconds
    tests = $orderedResults
    visual_validation = $visualValidation
    package_independence = $packageIndependence
    package_file_count = $packageFiles.Count
    package_size_bytes = [long](($packageFiles | Measure-Object Length -Sum).Sum)
    peak_packaged_working_set_bytes = [long](($orderedResults | Measure-Object peak_working_set_bytes -Maximum).Maximum)
    total_duration_seconds = [math]::Round(((Get-Date)-$runnerStart).TotalSeconds,3)
    critical_failures = @($criticalFailures | ForEach-Object { if($_ -is [string]){$_}else{$_.id} })
}
$paths = Write-SummaryFiles -Summary $summary -BuildRoot $buildRoot
Write-Output "V2_FINAL_BUILD_ROOT=$buildRoot"
Write-Output "V2_FINAL_PACKAGE_EXE=$packageExe"
Write-Output "V2_FINAL_SUMMARY_JSON=$($paths.json)"
Write-Output "V2_FINAL_SUMMARY_MD=$($paths.markdown)"
Write-Output "V2_FINAL_STATUS=$($summary.status)"
if ($summary.status -eq 'passed') { exit 0 }
exit 1
