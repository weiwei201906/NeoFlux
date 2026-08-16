# =============================================================================
# NeoFlux - Font Download Script
#
# Downloads open-source fonts into thirdparty/fonts/ for out-of-the-box
# text rendering. Run once after cloning the repository.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File download_fonts.ps1
# =============================================================================

$ErrorActionPreference = "Stop"
$FontsDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Write-Host "NeoFlux font downloader" -ForegroundColor Cyan
Write-Host "Target directory: $FontsDir"

# ---------------------------------------------------------------------------
# Font definitions: name, output filename, download URL(s) in priority order.
# ---------------------------------------------------------------------------
$Fonts = @(
    @{
        Name = "Noto Sans SC Regular (CJK)"
        File = "NotoSansSC-Regular.ttf"
        Urls = @(
            "https://github.com/google/fonts/raw/main/ofl/notosanssc/NotoSansSC%5Bwght%5D.ttf",
            "https://raw.githubusercontent.com/google/fonts/main/ofl/notosanssc/NotoSansSC%5Bwght%5D.ttf"
        )
    },
    @{
        Name = "DejaVu Sans (Latin fallback)"
        File = "DejaVuSans.ttf"
        Urls = @(
            "https://github.com/dejavu-fonts/dejavu-fonts/raw/master/ttf/DejaVuSans.ttf",
            "https://raw.githubusercontent.com/dejavu-fonts/dejavu-fonts/master/ttf/DejaVuSans.ttf"
        )
    }
)

function Download-WithRetry {
    param([string]$Url, [string]$OutFile, [int]$MaxRetries = 3)
    for ($i = 1; $i -le $MaxRetries; $i++) {
        try {
            Write-Host "  Attempt $i/$MaxRetries : $Url"
            Invoke-WebRequest -Uri $Url -OutFile $OutFile -UseBasicParsing -TimeoutSec 120
            if ((Get-Item $OutFile).Length -gt 1000) {
                return $true
            }
            Write-Host "  Downloaded file too small, retrying..." -ForegroundColor Yellow
        } catch {
            Write-Host "  Failed: $($_.Exception.Message)" -ForegroundColor Yellow
        }
    }
    return $false
}

$Downloaded = 0
$Skipped = 0
$Failed = 0

foreach ($font in $Fonts) {
    $outPath = Join-Path $FontsDir $font.File
    if (Test-Path $outPath) {
        $size = (Get-Item $outPath).Length
        if ($size -gt 1000) {
            Write-Host "[SKIP] $($font.Name) already exists ($([math]::Round($size/1KB, 1)) KB)" -ForegroundColor Gray
            $Skipped++
            continue
        }
    }
    Write-Host "[DOWN] $($font.Name)" -ForegroundColor Green
    $success = $false
    foreach ($url in $font.Urls) {
        if (Download-WithRetry -Url $url -OutFile $outPath) {
            $size = (Get-Item $outPath).Length
            Write-Host "  OK: $($font.File) ($([math]::Round($size/1KB, 1)) KB)" -ForegroundColor Green
            $success = $true
            $Downloaded++
            break
        }
    }
    if (-not $success) {
        Write-Host "  FAILED: Could not download $($font.Name)" -ForegroundColor Red
        Write-Host "  Please manually place a .ttf/.otf font in: $FontsDir" -ForegroundColor Yellow
        if (Test-Path $outPath) { Remove-Item $outPath -Force }
        $Failed++
    }
}

Write-Host ""
Write-Host "Summary: $Downloaded downloaded, $Skipped skipped, $Failed failed" -ForegroundColor Cyan
if ($Failed -gt 0) {
    Write-Host "Note: NeoFlux will fall back to system fonts if bundled fonts are missing." -ForegroundColor Yellow
}
