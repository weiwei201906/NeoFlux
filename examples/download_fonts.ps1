# =============================================================================
# NeoFlux - Download Fonts (Windows PowerShell)
#
# Downloads the default NotoSansSC font (SIL OFL 1.1) to thirdparty/fonts/.
# Run this before building examples if you need CJK text rendering.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File examples/download_fonts.ps1
# =============================================================================

$ErrorActionPreference = "Stop"

$FontDir = Join-Path $PSScriptRoot "..\thirdparty\fonts"
$FontFile = Join-Path $FontDir "NotoSansSC-Regular.ttf"
$FontUrl = "https://github.com/googlefonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf"

if (-not (Test-Path $FontDir)) {
    New-Item -ItemType Directory -Path $FontDir -Force | Out-Null
}

if (Test-Path $FontFile) {
    Write-Host "Font already exists: $FontFile"
    exit 0
}

Write-Host "Downloading NotoSansSC-Regular.otf..."
Write-Host "  URL: $FontUrl"
Write-Host "  Dest: $FontFile"

try {
    Invoke-WebRequest -Uri $FontUrl -OutFile $FontFile -UseBasicParsing
    $size = (Get-Item $FontFile).Length
    Write-Host "Downloaded: $([math]::Round($size / 1MB, 2)) MB"
} catch {
    Write-Host "Download failed: $_" -ForegroundColor Red
    Write-Host "Please download manually from:"
    Write-Host "  $FontUrl"
    Write-Host "and save to: $FontFile"
    exit 1
}

Write-Host "Done. Font ready for use."
