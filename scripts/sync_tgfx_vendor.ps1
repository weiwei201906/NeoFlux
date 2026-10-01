# =============================================================================
# sync_tgfx_vendor.ps1
#
# Idempotent vendor sync for the pinned tgfx third_party dependencies listed in
# thirdparty/tgfx/DEPS. Direct github.com:443 access is reset on this network,
# so every clone goes through the ghproxy mirror.
#
# Behaviour:
#   - If a vendor dir already exists and its git HEAD == pinned commit, skip.
#   - If a vendor dir exists but is missing/incomplete or at the wrong commit,
#     remove it and re-clone at the pinned commit.
#   - shaderc/glslang are GL-only marker dirs: tgfx configure must NOT try to
#     fetch them (they are only needed by D3D12/Vulkan/Metal shader compilers).
#
# Run from the repo root:  pwsh -File scripts/sync_tgfx_vendor.ps1
# =============================================================================

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$tpDir = Join-Path $repoRoot 'thirdparty\tgfx\third_party'
$mirror = 'https://ghproxy.net/https://github.com'

# The ghproxy proxy breaks HTTP/2 mid-transfer ("stream not closed cleanly").
# Force HTTP/1.1 and a large post buffer for reliable clones.
git config --global http.version HTTP/1.1
git config --global http.postBuffer 524288000
git config --global http.lowSpeedLimit 1000
git config --global http.lowSpeedTime 60

# dir, owner/repo, pinned commit (from thirdparty/tgfx/DEPS)
$vendors = @(
  @{ dir = 'vendor_tools';        repo = 'libpag/vendor_tools';                    commit = 'fdcd3c28c02c9d39aeef5aefede96645b8bc593b' },
  @{ dir = 'pathkit';             repo = 'libpag/pathkit';                         commit = '6dc44ae85b4614d4e5accac2d7059d7420db9ed2' },
  @{ dir = 'skcms';              repo = 'libpag/skcms';                           commit = '0a4e501a0e61d62dc38a0157e4355a58f5b01b81' },
  @{ dir = 'zlib';               repo = 'madler/zlib';                            commit = 'da607da739fa6047df13e66a2af6b8bec7c2a498' },
  @{ dir = 'libpng';             repo = 'glennrp/libpng';                         commit = '3061454d980de7d53608f594194cfac722721d2a' },
  @{ dir = 'libwebp';            repo = 'webmproject/libwebp';                   commit = '233960a0ad8c640acd458a6966dea09e12c1325a' },
  @{ dir = 'libjpeg-turbo';      repo = 'libjpeg-turbo/libjpeg-turbo';            commit = '0a9b9721782d3a60a5c16c8c9a7abf3d4b1ecd42' },
  @{ dir = 'freetype';           repo = 'freetype/freetype';                      commit = '0a0221a1347e2f1e07c395263540026e9a0aa7c7' },
  @{ dir = 'harfbuzz';           repo = 'harfbuzz/harfbuzz';                      commit = '9ef44a2d67ac870c1f7f671f6dc98d08a2579865' },
  @{ dir = 'googletest';         repo = 'google/googletest';                      commit = '6910c9d9165801d8827d628cb72eb7ea9dd538c5' },
  @{ dir = 'json';               repo = 'nlohmann/json';                          commit = 'fec56a1a16c6e1c1b1f4e116a20e79398282626c' },
  @{ dir = 'expat';              repo = 'libexpat/libexpat';                     commit = '92810461043fce37e70079b37ab1f04490a8f039' },
  @{ dir = 'concurrentqueue';    repo = 'cameron314/concurrentqueue';             commit = '6dd38b8a1dbaa7863aa907045f32308a56a6ff5d' },
  @{ dir = 'highway';            repo = 'google/highway';                         commit = 'a523516d35e22a4ba8e2e70a319062cb87352de6' },
  @{ dir = 'SPIRV-Cross';         repo = 'KhronosGroup/SPIRV-Cross';               commit = 'ebe2aa0cd80f5eb5cd8a605da604cacf72205f3b' },
  @{ dir = 'Vulkan-Headers';      repo = 'KhronosGroup/Vulkan-Headers';            commit = 'b5c8f996196ba4aa6d8f97e52b5d3b6e70f7e4e2' },
  @{ dir = 'volk';               repo = 'zeux/volk';                              commit = '0b17a763ba5643e32da1b2152f8140461b3b7345' },
  @{ dir = 'VulkanMemoryAllocator'; repo = 'GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator'; commit = '1d8f600fd424278486eade7ed3e877c99f0846b1' }
)

function Get-VendorHead([string]$path) {
  try {
    $head = git -C $path rev-parse HEAD 2>$null
    if ($LASTEXITCODE -eq 0) { return $head.Trim() }
  } catch {}
  return $null
}

foreach ($v in $vendors) {
  $dirPath = Join-Path $tpDir $v.dir
  $url = "$mirror/$($v.repo).git"

  $head = Get-VendorHead $dirPath
  if ($head -eq $v.commit) {
    Write-Host "[skip] $($v.dir) already at $($v.commit.Substring(0,10))"
    continue
  }

  if (Test-Path $dirPath) {
    Write-Host "[re-clone] $($v.dir) (head=$head) -> $($v.commit.Substring(0,10))"
    Remove-Item -Recurse -Force $dirPath
  } else {
    Write-Host "[clone] $($v.dir)"
  }

  # Depth-1 fetch of the pinned commit: downloads only the snapshot, not full
  # history, which is far more robust through the flaky proxy.
  $url = "$mirror/$($v.repo).git"
  $ok = $false
  for ($attempt = 1; $attempt -le 3 -and -not $ok; ++$attempt) {
    if (Test-Path $dirPath) { Remove-Item -Recurse -Force $dirPath }
    git init -q $dirPath
    git -C $dirPath remote add origin $url
    git -C $dirPath fetch -q --depth 1 origin $($v.commit)
    if ($LASTEXITCODE -ne 0) {
      Write-Host "  attempt $attempt failed (fetch); retrying"
      continue
    }
    git -C $dirPath checkout -q FETCH_HEAD
    if ($LASTEXITCODE -ne 0) {
      Write-Host "  attempt $attempt failed (checkout); retrying"
      continue
    }
    $ok = $true
  }
  if (-not $ok) { throw "git clone failed for $($v.repo) after retries" }

  $newHead = Get-VendorHead $dirPath
  if ($newHead -ne $v.commit) {
    throw "vendor $($v.dir) ended at $newHead, expected $($v.commit)"
  }
  Write-Host "[ok]   $($v.dir) @ $($newHead.Substring(0,10))"
}

# GL-only marker dirs so tgfx configure skips shaderc/glslang network sync.
foreach ($marker in @('shaderc', 'glslang')) {
  $mPath = Join-Path $tpDir $marker
  if (-not (Test-Path $mPath)) {
    New-Item -ItemType Directory -Path $mPath | Out-Null
  }
  $keep = Join-Path $mPath 'README.md'
  if (-not (Test-Path $keep)) {
    Set-Content -Path $keep -Encoding utf8 -Value (@(
      "# $marker",
      "GL-only build marker: tgfx configure skips shaderc/glslang network sync.",
      "shaderc/glslang are only required by the D3D12/Vulkan/Metal backends."
    ) -join "`r`n")
  }
  Write-Host "[marker] $marker"
}

Write-Host "Vendor sync complete."
