param(
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo')]
    [string]$Config = 'Debug',

    [string]$Filter = '',

    [switch]$Reconfigure
)

$ErrorActionPreference = 'Stop'

if ($Filter -and ($Filter -like '*\*' -or $Filter -like '*/*' -or $Filter -like '*.cpp' -or $Filter -like '*.ps1'))
{
    $fileName = Split-Path -Leaf $Filter
    Write-Host "==> Detectado caminho de arquivo no filtro: $fileName"

    if ($fileName -eq 'tst_state_machine.cpp')
    {
        $Filter = 'turnaround-workflow'
    } elseif ($fileName -like '*_state.cpp')
    {
        $stateName = $fileName -replace '^tst_', '' -replace '_state\.cpp$', '' -replace '_', '-'
        $Filter = "turnaround-state-$stateName"
    } elseif ($fileName -like 'tst_*.cpp')
    {
        $Filter = $fileName -replace '^tst_', '' -replace '\.cpp$', '' -replace '_', '-'
    } elseif ($fileName -like 'check-*.ps1')
    {
        $Filter = $fileName -replace '\.ps1$', ''
    } else
    {
        Write-Host "==> Arquivo não é um teste C++ reconhecido. Rodando todos os testes."
        $Filter = ''
    }

    if ($Filter)
    {
        Write-Host "==> Mapeado para o teste CTest: $Filter"
    }
}

$cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeCmd)
{
    $cmake = $cmakeCmd.Source
} else
{
    $candidates = @(
        "$env:ProgramFiles\CMake\bin\cmake.exe",
        "$env:LOCALAPPDATA\Programs\CLion\bin\cmake\win\x64\bin\cmake.exe"
    )
    $cmake = $candidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
}
if (-not $cmake)
{ throw 'cmake.exe não encontrado. Instale o CMake ou o CLion.'
}

$ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
if (-not (Test-Path -LiteralPath $ctest))
{ throw 'ctest.exe não encontrado ao lado do cmake.'
}

if (-not $env:QT_ROOT_DIR)
{
    $kit = Get-ChildItem -LiteralPath 'C:\Qt' -Directory -ErrorAction SilentlyContinue |
        ForEach-Object {
            $p = Join-Path $_.FullName 'msvc2022_64'
            if (Test-Path "$p\bin\windeployqt.exe")
            { Get-Item $p
            }
        } |
        Sort-Object { [version]$_.Parent.Name } -Descending |
        Select-Object -First 1
    if ($kit)
    { $env:QT_ROOT_DIR = $kit.FullName
    }
}

$preset  = $Config.ToLowerInvariant()
$buildDir = Join-Path $PSScriptRoot "build/$preset"

$env:MSBUILDDISABLENODEREUSE = '1'

& (Join-Path $PSScriptRoot 'tools/remove-locked-build-outputs.ps1') -Directory $buildDir

function Get-OutdatedGenerateInputs
{
    param([string]$Directory)

    $stamp = Join-Path $Directory 'CMakeFiles/generate.stamp'
    $depend = "$stamp.depend"
    if (-not (Test-Path -LiteralPath $stamp) -or -not (Test-Path -LiteralPath $depend))
    {
        return @('CMakeFiles/generate.stamp ausente')
    }

    $stampTime = [System.IO.File]::GetLastWriteTimeUtc($stamp)
    $outdated = @()
    foreach ($entry in [System.IO.File]::ReadAllLines($depend))
    {
        $path = $entry.Trim()
        if (-not $path -or $path.StartsWith('#'))
        {
            continue
        }

        if (-not [System.IO.File]::Exists($path) -or [System.IO.File]::GetLastWriteTimeUtc($path) -gt $stampTime)
        {
            $outdated += $path
        }
    }

    return $outdated
}

if ($Reconfigure)
{
    Write-Host "==> Configurando preset '$preset', forçado por -Reconfigure..."
    $mustConfigure = $true
} else
{
    $outdatedInputs = @(Get-OutdatedGenerateInputs $buildDir)
    $mustConfigure = $outdatedInputs.Count -gt 0
    if ($mustConfigure)
    {
        Write-Host "==> Configurando preset '$preset', porque $($outdatedInputs.Count) entrada(s) pedem:"
        foreach ($entry in ($outdatedInputs | Select-Object -First 5))
        {
            Write-Host "    $entry"
        }
    } else
    {
        Write-Host "==> Preset '$preset' já configurado: nada mais novo que generate.stamp."
    }
}

if ($mustConfigure)
{
    & $cmake --preset $preset
    if ($LASTEXITCODE -ne 0)
    { exit $LASTEXITCODE
    }
}

$filterIsGuard = $Filter -like 'check-*' -and
    (Test-Path -LiteralPath (Join-Path $PSScriptRoot "tools/$Filter.ps1"))

if ($filterIsGuard)
{
    Write-Host "==> $Filter é uma guarda e não compila nada: nenhum alvo a construir."
} else
{
    if ($Filter)
    {
        if ($Filter -like 'turnaround-state-*')
        {
            $stateName = $Filter -replace '^turnaround-state-', ''
            $targetToBuild = "gsxi-turnaround-$stateName-state-tests"
        } else
        {
            $targetToBuild = "gsxi-$Filter-tests"
        }
        Write-Host "==> Compilando apenas o alvo de teste correspondente: $targetToBuild ($Config)..."
        & $cmake --build --preset $preset --target $targetToBuild --parallel -- '-nodeReuse:false'
    } else
    {
        Write-Host "==> Compilando todos os alvos ($Config)..."
        & $cmake --build --preset $preset --parallel -- '-nodeReuse:false'
    }
    if ($LASTEXITCODE -ne 0)
    { exit $LASTEXITCODE
    }
}

Write-Host "`n==> Rodando testes..."
$ctestArgs = @(
    '--test-dir', $buildDir
    '-C', $Config
    '--output-on-failure'
    '-j', [Environment]::ProcessorCount
)
if ($Filter)
{ $ctestArgs += '-R', $Filter
}

& $ctest @ctestArgs
exit $LASTEXITCODE
