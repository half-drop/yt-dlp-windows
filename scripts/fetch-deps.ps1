# Download portable yt-dlp + ffmpeg into deps\ next to the solution (or -OutDir).
# Usage: .\scripts\fetch-deps.ps1 [-OutDir path\to\deps]

param(
    [string]$OutDir
)

$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$root = Split-Path -Parent $PSScriptRoot
if (-not $OutDir) { $OutDir = Join-Path $root 'deps' }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$OutDir = (Resolve-Path $OutDir).Path

$ProgressPreference = 'SilentlyContinue'

function Get-File($Url, $Dest) {
    Write-Host "Downloading $Url"
    $tmp = "$Dest.partial"
    curl.exe -L --fail --retry 3 --retry-delay 2 -o $tmp $Url
    if ($LASTEXITCODE -ne 0) { throw "Download failed: $Url" }
    Move-Item -Force $tmp $Dest
}

# --- yt-dlp.exe ---
$yt = Join-Path $OutDir 'yt-dlp.exe'
Get-File 'https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe' $yt

# --- ffmpeg + ffprobe (essentials build) ---
$needFfmpeg = -not ((Test-Path (Join-Path $OutDir 'ffmpeg.exe')) -and (Test-Path (Join-Path $OutDir 'ffprobe.exe')))
if ($needFfmpeg) {
    $zip = Join-Path $env:TEMP 'ffmpeg-release-essentials.zip'
    $extract = Join-Path $env:TEMP 'ffmpeg-essentials-extract'
    Get-File 'https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip' $zip
    if (Test-Path $extract) { Remove-Item -Recurse -Force $extract }
    Expand-Archive -Path $zip -DestinationPath $extract -Force
    $bin = Get-ChildItem -Path $extract -Recurse -Filter 'ffmpeg.exe' | Select-Object -First 1
    if (-not $bin) { throw 'ffmpeg.exe not found in archive' }
    $src = $bin.Directory.FullName
    Copy-Item -Force (Join-Path $src 'ffmpeg.exe') $OutDir
    Copy-Item -Force (Join-Path $src 'ffprobe.exe') $OutDir
    Remove-Item -Recurse -Force $extract
    Remove-Item -Force $zip
}

Write-Host ""
Write-Host "OK: $OutDir"
Get-ChildItem $OutDir -Filter '*.exe' | ForEach-Object { Write-Host ("  {0}  {1:N1} MB" -f $_.Name, ($_.Length / 1MB)) }
