param(
    [Parameter(Mandatory = $true)][string]$Path,
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Utf8NoBom = [System.Text.UTF8Encoding]::new($false)

if (-not (Test-Path -LiteralPath $Path)) {
    [pscustomobject]@{ Path = $Path; Exists = $false; Files = 0; Bytes = 0; ManifestSha256 = $null }
    return
}

$Item = Get-Item -LiteralPath $Path -Force
$Resolved = $Item.FullName.TrimEnd('\')
$Files = if ($Item.PSIsContainer) {
    @(Get-ChildItem -LiteralPath $Resolved -Recurse -File -Force | Sort-Object FullName)
} else {
    @($Item)
}
$Lines = foreach ($File in $Files) {
    $Relative = if ($Item.PSIsContainer) {
        $File.FullName.Substring($Resolved.Length).TrimStart('\')
    } else { $File.Name }
    $Hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $File.FullName).Hash.ToUpperInvariant()
    "{0}`t{1}`t{2}" -f $Relative.Replace('\', '/'), $File.Length, $Hash
}
$Canonical = ($Lines -join "`n") + "`n"
$Hasher = [System.Security.Cryptography.SHA256]::Create()
try {
    $ManifestSha256 = ([BitConverter]::ToString(
        $Hasher.ComputeHash($Utf8NoBom.GetBytes($Canonical)))).Replace('-', '')
} finally { $Hasher.Dispose() }

if ($OutputPath) {
    $Parent = Split-Path -Parent $OutputPath
    if ($Parent) { New-Item -ItemType Directory -Path $Parent -Force | Out-Null }
    [System.IO.File]::WriteAllText($OutputPath, $Canonical, $Utf8NoBom)
}

[pscustomobject]@{
    Path = $Resolved
    Exists = $true
    Files = $Files.Count
    Bytes = [long](($Files | Measure-Object Length -Sum).Sum)
    ManifestSha256 = $ManifestSha256
}
