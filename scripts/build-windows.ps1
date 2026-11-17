[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [switch]$Verify,
    [switch]$Benchmark,
    [switch]$Run
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repoRoot = Split-Path -Parent $PSScriptRoot

function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $Program"
    }
}

if ($env:OS -ne 'Windows_NT') {
    throw 'This script requires Windows 10/11 and Visual Studio 2022 with Desktop development with C++.'
}
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw 'Visual Studio Installer was not found. Install Visual Studio 2022 with Desktop development with C++.'
}
$vsRoot = & $vswhere -latest -products '*' -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsRoot) {
    throw 'Visual Studio 2022 C++ tools were not found. Add the Desktop development with C++ workload in Visual Studio Installer.'
}
$vsRoot = ([string]$vsRoot).Trim()
$cmake = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
if (-not (Test-Path -LiteralPath $cmake)) {
    $found = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if (-not $found) {
        throw 'CMake was not found. Add C++ CMake tools for Windows in Visual Studio Installer, or install CMake 3.21+.'
    }
    $cmake = $found.Source
}
$ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
if (-not (Test-Path -LiteralPath $ctest)) { throw "ctest.exe was not found beside $cmake" }

$buildDir = Join-Path $repoRoot 'build\windows'
Invoke-Checked -Program $cmake -Arguments @(
    '-S', $repoRoot, '-B', $buildDir,
    '-G', 'Visual Studio 17 2022', '-A', 'x64',
    "-DCMAKE_GENERATOR_INSTANCE=$vsRoot",
    '-DRUNNER_FETCH_DEPENDENCIES=ON', '-DBUILD_TESTING=ON'
)
Invoke-Checked -Program $cmake -Arguments @('--build', $buildDir, '--config', $Configuration, '--parallel', '2')
Invoke-Checked -Program $ctest -Arguments @('--test-dir', $buildDir, '-C', $Configuration, '--output-on-failure')

$exe = Join-Path $buildDir "bin\$Configuration\EndlessRunner.exe"
if (-not (Test-Path -LiteralPath $exe)) { throw "Built executable was not found: $exe" }
$licenses = Join-Path $buildDir 'licenses'
if (Test-Path -LiteralPath $licenses) {
    Copy-Item -LiteralPath $licenses -Destination (Split-Path -Parent $exe) -Recurse -Force
}
Write-Host "Built successfully: $exe"

Push-Location -LiteralPath $repoRoot
try {
    if ($Verify) {
        $stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
        $capture = Join-Path $repoRoot "artifacts\verification-$stamp.ppm"
        Invoke-Checked -Program $exe -Arguments @('--smoke-test', '--screenshot', $capture)
        Write-Host "Validation screenshot: $capture"
    }
    if ($Benchmark) {
        if ($Configuration -ne 'Release') { throw 'Use Release for report performance measurements.' }
        & (Join-Path $PSScriptRoot 'measure-performance.ps1') -Executable $exe -Configuration $Configuration
    }
    if ($Run) { Invoke-Checked -Program $exe -Arguments @() }
}
finally { Pop-Location }
