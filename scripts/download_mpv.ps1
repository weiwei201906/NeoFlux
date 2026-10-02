#Requires -Version 5.1
# ---------------------------------------------------------------------------
# NeoFlux - scripts/download_mpv.ps1
#
# Downloads the latest Windows mpv build from the community shinchiro/mpv-win32
# GitHub releases, extracts only what NeoFlux needs:
#   thirdparty/mpv-bundle/include/mpv/*.h
#   thirdparty/mpv-bundle/libmpv-2.dll
#
# The bundle is fetched fresh on every configure when it is missing, so the
# project always tracks the newest mpv release instead of a vendored binary.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts/download_mpv.ps1
# ---------------------------------------------------------------------------

[CmdletBinding()]
param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = "Stop"
$bundleDir = Join-Path $RepoRoot "thirdparty\mpv-bundle"

# Already present? Nothing to do.
if ((Test-Path (Join-Path $bundleDir "include\mpv\client.h")) -and
    (Test-Path (Join-Path $bundleDir "libmpv-2.dll"))) {
    Write-Host "mpv bundle already present at $bundleDir"
    exit 0
}

New-Item -ItemType Directory -Force -Path $bundleDir | Out-Null

# Query the latest release of shinchiro/mpv-win32. We pick the MSVC x86_64
# build (it ships the libmpv-2.dll and headers we need).
$api = "https://api.github.com/repos/shinchiro/mpv-win32/releases/latest"
Write-Host "Querying latest mpv Windows build from $api"
$rel = Invoke-RestMethod -Uri $api -Headers @{ "User-Agent" = "neoflux" }

$asset = $rel.assets | Where-Object {
    $_.name -match "mpv-x86_64-windows-msvc" -and $_.name -match "\.7z$"
} | Select-Object -First 1

if ($null -eq $asset) {
    Write-Error "Could not find an mpv-x86_64-windows-msvc .7z asset in the latest release."
    exit 1
}

$archive = Join-Path $env:TEMP $asset.name
Write-Host "Downloading $($asset.browser_download_url) -> $archive"
Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $archive -UseBasicParsing

# Extract with 7z (ships with GitHub's Windows runner and most dev boxes).
$sevenZip = "7z.exe"
if (-not (Get-Command $sevenZip -ErrorAction SilentlyContinue)) {
    # Fallback to the standard install path on GitHub runners.
    $candidate = "C:\Program Files\7-Zip\7z.exe"
    if (Test-Path $candidate) { $sevenZip = $candidate }
    else {
        Write-Error "7z.exe not found; please install 7-Zip or run on a GitHub Windows runner."
        exit 1
    }
}

$extractDir = Join-Path $env:TEMP ("mpv-extract-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $extractDir | Out-Null
& $sevenZip x $archive "-o$extractDir" -y | Out-Null
if ($LASTEXITCODE -ne 0) { Write-Error "7z extraction failed"; exit 1 }

# The archive root contains include/mpv/*.h and a bin/libmpv-2.dll.
$srcInclude = Get-ChildItem -Recurse -Path $extractDir -Filter "client.h" |
    Where-Object { $_.FullName -match "include\\mpv$" } | Select-Object -First 1
if ($null -eq $srcInclude) { Write-Error "mpv headers not found in archive"; exit 1 }
$mpvIncludeDir = $srcInclude.Directory.Parent.FullName  # .../include

$srcDll = Get-ChildItem -Recurse -Path $extractDir -Filter "libmpv-2.dll" | Select-Object -First 1
if ($null -eq $srcDll) { Write-Error "libmpv-2.dll not found in archive"; exit 1 }

$dstInclude = Join-Path $bundleDir "include"
New-Item -ItemType Directory -Force -Path $dstInclude | Out-Null
Copy-Item -Recurse -Force (Join-Path $mpvIncludeDir "mpv") $dstInclude
Copy-Item -Force $srcDll.FullName (Join-Path $bundleDir "libmpv-2.dll")

Write-Host "mpv bundle ready at $bundleDir"
Remove-Item -Recurse -Force $extractDir -ErrorAction SilentlyContinue
Remove-Item -Force $archive -ErrorAction SilentlyContinue
