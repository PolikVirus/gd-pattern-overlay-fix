# Build GDR Pattern Reader - forces VS 2022 and a fresh CMake cache (avoids VS 18 CL crash).
# Usage: powershell -ExecutionPolicy Bypass -File .\build.ps1
#        powershell -ExecutionPolicy Bypass -File .\build.ps1 -Clean

$ErrorActionPreference = "Stop"

if (-not $env:GEODE_SDK) {
    Write-Host "GEODE_SDK is not set. Example:" -ForegroundColor Red
    Write-Host '  [Environment]::SetEnvironmentVariable("GEODE_SDK", "C:\Users\OasenHase\Documents\Geode", "User")'
    exit 1
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

cmd /c "taskkill /IM mspdbsrv.exe /F >nul 2>&1" | Out-Null

function Get-Vs2022Root {
    $candidates = @(
        (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\2022\BuildTools"),
        (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\2022\Community"),
        (Join-Path $env:ProgramFiles "Microsoft Visual Studio\2022\BuildTools"),
        (Join-Path $env:ProgramFiles "Microsoft Visual Studio\2022\Community")
    )
    foreach ($p in $candidates) {
        $vc = Join-Path $p "VC\Auxiliary\Build\vcvars64.bat"
        if ((Test-Path $p) -and (Test-Path $vc)) { return $p }
    }
    return $null
}

$vs2022Instance = Get-Vs2022Root
if (-not $vs2022Instance) {
    Write-Host "Visual Studio 2022 with C++ tools not found (need vcvars64.bat)." -ForegroundColor Red
    exit 1
}

$vcvars = Join-Path $vs2022Instance "VC\Auxiliary\Build\vcvars64.bat"
$cachePath = Join-Path $scriptDir "build\CMakeCache.txt"
$needsClean = $args -contains "-Clean"
if (-not $needsClean -and (Test-Path $cachePath)) {
    $cacheText = Get-Content $cachePath -Raw
    if ($cacheText -match 'Visual Studio 18') {
        Write-Host "CMake cache used Visual Studio 18 (causes CL crash). Deleting build folder." -ForegroundColor Yellow
        $needsClean = $true
    }
}

if ($needsClean) {
    $buildDir = Join-Path $scriptDir "build"
    if (Test-Path $buildDir) {
        Remove-Item -Recurse -Force $buildDir
    }
    Write-Host "Removed build folder (fresh configure)." -ForegroundColor Yellow
}

Write-Host "VS 2022 instance: $vs2022Instance" -ForegroundColor Green
Write-Host "Running geode build (Release)..." -ForegroundColor Green

$env:CMAKE_GENERATOR_INSTANCE = $vs2022Instance
$env:CMAKE_GENERATOR = "Visual Studio 17 2022"
$env:CMAKE_GENERATOR_PLATFORM = "x64"

$geodeSdk = $env:GEODE_SDK
$batchLines = @(
    '@echo off',
    ('call "{0}" >nul' -f $vcvars),
    ('set GEODE_SDK={0}' -f $geodeSdk),
    'set GEODE_DISABLE_PRECOMPILED_HEADERS=ON',
    ('set CMAKE_GENERATOR_INSTANCE={0}' -f $vs2022Instance),
    'set "CMAKE_GENERATOR=Visual Studio 17 2022"',
    'set CMAKE_GENERATOR_PLATFORM=x64',
    'set CL=/MP1',
    'set _CL_=/MP1',
    'geode build --config Release'
)
$batchFile = Join-Path $env:TEMP "gdr-geode-build.cmd"
Set-Content -Path $batchFile -Value ($batchLines -join "`r`n") -Encoding ASCII
cmd /c $batchFile
$exit = $LASTEXITCODE
Remove-Item $batchFile -ErrorAction SilentlyContinue

if ($exit -ne 0) {
    if (Test-Path $cachePath) {
        $line = Get-Content $cachePath | Where-Object { $_ -match 'CMAKE_GENERATOR:INTERNAL' } | Select-Object -First 1
        Write-Host ""
        Write-Host "Build failed (exit $exit). CMake cache says: $line" -ForegroundColor Red
        if ($line -match '18') {
            Write-Host "Still on VS 18. Run:  .\build.ps1 -Clean" -ForegroundColor Yellow
        }
    } else {
        Write-Host ""
        Write-Host "Build failed (exit $exit). See INSTALL.md section 3." -ForegroundColor Red
    }
    exit $exit
}

$buildDir = Join-Path $scriptDir "build"
$geodeFile = Get-ChildItem -Path $buildDir -Filter "oasenhase.gdr_pattern_reader.geode" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
if ($geodeFile) {
    Write-Host ""
    Write-Host "Built: $($geodeFile.FullName)" -ForegroundColor Green
    Write-Host "Copy to: $env:LOCALAPPDATA\GeometryDash\geode\mods\"
} else {
    Write-Host ""
    Write-Host 'Build finished; search build folder for the geode package.' -ForegroundColor Yellow
}
