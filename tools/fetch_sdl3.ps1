# Downloads the latest SDL3 development package for Windows (MSVC) into third_party/SDL3.
# Run once after cloning; the result is committed so CI and other machines don't need to.
param(
    [string]$Version = ""  # optional explicit version, e.g. "3.4.18"; default: latest release
)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$dest = Join-Path $root "third_party\SDL3"

if (Test-Path "$dest\include\SDL3\SDL.h") {
    Write-Host "SDL3 already vendored at third_party/SDL3 - nothing to do."
    exit 0
}

# TLS 1.2 for older Windows PowerShell versions
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

if ($Version -ne "") {
    $release = Invoke-RestMethod "https://api.github.com/repos/libsdl-org/SDL/releases/tags/release-$Version"
} else {
    $release = Invoke-RestMethod "https://api.github.com/repos/libsdl-org/SDL/releases/latest"
}
$tag = $release.tag_name
$version = $tag.TrimStart("release-")
Write-Host "SDL3 release: $tag"

# The -VC.zip devel package: headers, import libs, CMake config.
# (SDL3-<ver>-win32-x64.zip is runtime-only: just the DLL.)
$asset = $release.assets | Where-Object { $_.name -eq "SDL3-devel-$version-VC.zip" } | Select-Object -First 1
if (-not $asset) {
    throw "No SDL3-devel-$version-VC.zip asset found in $tag. Check https://github.com/libsdl-org/SDL/releases"
}

$tmpZip = Join-Path $env:TEMP $asset.name
Write-Host "Downloading $($asset.name)..."
Invoke-WebRequest $asset.browser_download_url -OutFile $tmpZip

$stage = Join-Path $env:TEMP "opencube_sdl3_extract"
Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue
Expand-Archive $tmpZip $stage

# Use the versioned root folder when the zip has exactly one, else the stage root.
$dirs = Get-ChildItem $stage | Where-Object { $_.PSIsContainer }
$files = Get-ChildItem $stage | Where-Object { -not $_.PSIsContainer }
$src = if (($dirs.Count -eq 1) -and ($files.Count -eq 0)) { $dirs[0].FullName } else { $stage }

New-Item -ItemType Directory -Force $dest | Out-Null
Copy-Item "$src\*" $dest -Recurse -Force
Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item $tmpZip -Force -ErrorAction SilentlyContinue

if (-not (Test-Path "$dest\include\SDL3\SDL.h")) {
    throw "Extraction finished but $dest\include\SDL3\SDL.h is missing - unexpected zip layout."
}

# Trim to what the build actually uses: x64 only, no PDBs.
Remove-Item "$dest\lib\arm64", "$dest\lib\x86" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item "$dest\lib\*.pdb" -Force -ErrorAction SilentlyContinue

Write-Host "SDL3 $version vendored into third_party/SDL3"
