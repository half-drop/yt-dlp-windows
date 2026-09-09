# Build YtDlpMfc (Release x64)
# Run from yt-dlp-windows directory, or from any path.

param(
    [ValidateSet('Debug','Release')]
    [string]$Configuration = 'Release',
    [ValidateSet('x64','Win32')]
    [string]$Platform = 'x64'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$sln = Join-Path $root 'YtDlpMfc.sln'

function Find-MSBuild {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $path = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild `
            -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
        if ($path) { return $path }
    }
    $cmd = Get-Command msbuild -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    return $null
}

$msbuild = Find-MSBuild
if (-not $msbuild) {
    Write-Error "未找到 MSBuild。请安装 Visual Studio 2022（含 C++ 桌面开发 + MFC）。"
}

Write-Host "Using MSBuild: $msbuild"
Write-Host "Building $sln ($Configuration|$Platform) ..."

& $msbuild $sln /p:Configuration=$Configuration /p:Platform=$Platform /m /v:m /nologo
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$exe = Join-Path $root "bin\$Platform\$Configuration\YtDlpMfc.exe"
if (Test-Path $exe) {
    Write-Host "OK: $exe"
    Write-Host "Run with: & '$exe'"
} else {
    Write-Warning "Build finished but exe not found at $exe"
}
