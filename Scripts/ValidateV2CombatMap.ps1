param()
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$MapFile = Join-Path $ProjectRoot 'Content\DemoV2\Maps\L_V2_CombatDemo.umap'
$LogFile = Join-Path $ProjectRoot 'Docs\0.2.0版本开发档案\0.2.5.0_MAP_GENERATION.log'
if (-not (Test-Path -LiteralPath $MapFile)) { throw "V2-C map missing: $MapFile" }
if ((Get-Item -LiteralPath $MapFile).Length -le 0) { throw 'V2-C map is empty' }
if (-not (Test-Path -LiteralPath $LogFile)) { throw 'Map generation log missing' }
if (-not (Select-String -LiteralPath $LogFile -SimpleMatch 'V2E_MAP_GENERATION: PASS' -Quiet)) { throw 'Map generation PASS marker missing' }
$hash = (Get-FileHash -LiteralPath $MapFile -Algorithm SHA256).Hash
Write-Output "V2E_MAP_VALIDATION: PASS Path=$MapFile SHA256=$hash"
