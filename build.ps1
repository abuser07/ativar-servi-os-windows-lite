param(
    [string]$Toolchain = $env:W64DEVKIT,
    [switch]$DownloadToolchain
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$src = Join-Path $root 'src'
$dist = Join-Path $root 'dist'
$build = Join-Path $root 'build'
$tools = Join-Path $root 'tools'

function Find-Toolchain {
    $candidates = @()
    if ($Toolchain) {
        $candidates += (Join-Path $Toolchain 'bin')
        $candidates += (Join-Path $Toolchain 'w64devkit\bin')
        $candidates += $Toolchain
    }
    $candidates += (Join-Path $tools 'w64devkit\bin')
    $candidates += (Join-Path $tools 'w64devkit\w64devkit\bin')
    foreach ($candidate in $candidates) {
        $gpp = Join-Path $candidate 'g++.exe'
        if (Test-Path $gpp) { return $candidate }
    }
    $cmd = Get-Command g++.exe -ErrorAction SilentlyContinue
    if ($cmd) { return (Split-Path -Parent $cmd.Source) }
    return $null
}

$bin = Find-Toolchain
if (-not $bin) {
    if ($DownloadToolchain) {
        Write-Host 'Baixando w64devkit (MinGW-w64)...'
        $zip = Join-Path $tools 'w64devkit.zip'
        $dest = Join-Path $tools 'w64devkit'
        if (-not (Test-Path $zip)) {
            Invoke-WebRequest -Uri 'https://github.com/skeeto/w64devkit/releases/download/v2.11.0/w64devkit-x64-2.11.0.zip' -OutFile $zip
        }
        Expand-Archive -Path $zip -DestinationPath $dest -Force
        $bin = Find-Toolchain
    }
}
if (-not $bin) {
    Write-Host 'ERRO: compilador MinGW-w64 (g++.exe) não encontrado.' -ForegroundColor Red
    Write-Host 'Opções:'
    Write-Host '  1) Instale w64devkit e defina $env:W64DEVKIT, ou'
    Write-Host '  2) Extraia w64devkit em tools\w64devkit, ou'
    Write-Host '  3) Adicione g++.exe ao PATH, ou'
    Write-Host '  4) Execute .\build.ps1 -DownloadToolchain'
    exit 1
}

$gpp = Join-Path $bin 'g++.exe'
$windres = Join-Path $bin 'windres.exe'
if (-not (Test-Path $windres)) { $windres = Join-Path $bin 'windres.exe' }

Write-Host "Compilador: $gpp"
Write-Host "windres: $windres"

New-Item -ItemType Directory -Force -Path $build | Out-Null
New-Item -ItemType Directory -Force -Path $dist | Out-Null

$resources = Join-Path $build 'resources.o'
Write-Host 'Compilando recursos...'
Push-Location $src
& $windres --codepage=65001 "-I." "resources.rc" "-o$resources"
$rcCode = $LASTEXITCODE
Pop-Location
if ($rcCode -ne 0) { exit 1 }

$common = @('-std=c++17', '-municode', '-mwindows', '-DUNICODE', '-D_UNICODE',
            '-D_WIN32_WINNT=0x0601', '-DWINVER=0x0601', '-O2', '-finput-charset=UTF-8',
            '-Wall', '-Wextra', '-Wno-cast-function-type')

$objects = @()
foreach ($file in @('main.cpp', 'ui.cpp', 'core.cpp', 'catalog.cpp', 'i18n.cpp')) {
    $obj = Join-Path $build ([System.IO.Path]::GetFileNameWithoutExtension($file) + '.o')
    $objects += $obj
    Write-Host "Compilando $file ..."
    & $gpp @common "-I$src" -c (Join-Path $src $file) "-o$obj"
    if ($LASTEXITCODE -ne 0) { exit 1 }
}

$exe = Join-Path $dist 'WindowsLiteRestore.exe'
Write-Host 'Vinculando executável...'
& $gpp @common "-I$src" $objects $resources "-o$exe" `
    -static -static-libgcc -static-libstdc++ `
    -lcomctl32 -lcomdlg32 -lshell32 -lshlwapi -lole32 -loleaut32 -luuid `
    -luser32 -lgdi32 -ladvapi32 -lmsimg32 -luxtheme
if ($LASTEXITCODE -ne 0) { exit 1 }

# ---------------------------------------------------------------------------
# Instalador (embute o executável e a documentação como recursos)
# ---------------------------------------------------------------------------
Write-Host 'Compilando instalador...'
$installerObj = Join-Path $build 'installer.o'
& $gpp @common "-I$src" -c (Join-Path $src 'installer.cpp') "-o$installerObj"
if ($LASTEXITCODE -ne 0) { exit 1 }

$installerRes = Join-Path $build 'installer-resources.o'
Push-Location $src
& $windres --codepage=65001 "-I." "installer.rc" "-o$installerRes"
$ircCode = $LASTEXITCODE
Pop-Location
if ($ircCode -ne 0) { exit 1 }

$setup = Join-Path $dist 'WindowsLiteRestore-Setup.exe'
& $gpp @common "-I$src" $installerObj (Join-Path $build 'i18n.o') $installerRes "-o$setup" `
    -static -static-libgcc -static-libstdc++ `
    -lcomctl32 -lshell32 -lshlwapi -lole32 -loleaut32 -luuid `
    -luser32 -lgdi32 -ladvapi32
if ($LASTEXITCODE -ne 0) { exit 1 }

Write-Host ''
Write-Host "Executável gerado: $exe" -ForegroundColor Green
$size = (Get-Item $exe).Length
Write-Host "Tamanho: $([math]::Round($size / 1MB, 2)) MB"
Write-Host "Instalador gerado: $setup" -ForegroundColor Green
$setupSize = (Get-Item $setup).Length
Write-Host "Tamanho do instalador: $([math]::Round($setupSize / 1MB, 2)) MB"

# Copia o README para dist, se existir.
$readme = Join-Path $root 'README.md'
if (Test-Path $readme) {
    Copy-Item $readme (Join-Path $dist 'README.md') -Force
}

# Copia a documentação em outros idiomas, se existir.
$docs = Join-Path $root 'docs'
if (Test-Path $docs) {
    $destDocs = Join-Path $dist 'docs'
    New-Item -ItemType Directory -Force -Path $destDocs | Out-Null
    Copy-Item (Join-Path $docs '*.md') $destDocs -Force
}

# Checksum.
$hash = (Get-FileHash -Algorithm SHA256 $exe).Hash
$hash | Out-File (Join-Path $dist 'SHA256.txt') -Encoding ascii
Write-Host "SHA256: $hash"
