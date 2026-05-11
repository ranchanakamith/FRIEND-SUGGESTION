# build.ps1 — Configure and build FriendSuggestionUI
# Run from the project directory:  .\build.ps1

$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath  = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmake   = "$vsPath\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

if (-not (Test-Path $cmake)) {
    Write-Error "CMake not found in VS installation. Install 'C++ CMake tools for Windows' via VS Installer."
    exit 1
}

$projDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = "$projDir\build"

Write-Host ""
Write-Host "=== Configuring (first run downloads GLFW + ImGui from GitHub) ===" -ForegroundColor Cyan
& $cmake -B $buildDir -S $projDir -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) { Write-Error "CMake configure failed"; exit 1 }

Write-Host ""
Write-Host "=== Building Release ===" -ForegroundColor Cyan
& $cmake --build $buildDir --config Release
if ($LASTEXITCODE -ne 0) { Write-Error "Build failed"; exit 1 }

$exe = "$buildDir\Release\FriendSuggestionUI.exe"
Write-Host ""
Write-Host "=== Build successful! ===" -ForegroundColor Green
Write-Host "Executable: $exe"
Write-Host ""
Write-Host "Run it with:"
Write-Host "  & '$exe'"
