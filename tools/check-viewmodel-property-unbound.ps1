param([string]$Root)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'guard-check-lib.ps1')

$detect = {
    param([string]$root)

    $viewmodelDir = Join-Path $root 'src/viewmodel'
    if (-not (Test-Path -LiteralPath $viewmodelDir))
    {
        return @()
    }

    $referenceCorpus = ''
    $qmlDir = Join-Path $root 'src/qml'
    if (Test-Path -LiteralPath $qmlDir)
    {
        foreach ($file in Get-ChildItem -LiteralPath $qmlDir -Recurse -Include *.qml -File)
        {
            $referenceCorpus += [System.IO.File]::ReadAllText($file.FullName) + "`n"
        }
    }
    $mainCpp = Join-Path $root 'src/main.cpp'
    if (Test-Path -LiteralPath $mainCpp)
    {
        $referenceCorpus += [System.IO.File]::ReadAllText($mainCpp) + "`n"
    }

    $findings = @()
    foreach ($header in Get-ChildItem -LiteralPath $viewmodelDir -Filter *.h -File)
    {
        $relative = $header.FullName.Substring($root.Length).TrimStart('\', '/')
        $lines = [System.IO.File]::ReadAllLines($header.FullName)
        $viewModel = [System.IO.Path]::GetFileNameWithoutExtension($header.Name)
        for ($i = 0; $i -lt $lines.Length; $i++)
        {
            $classMatch = [regex]::Match($lines[$i], '^\s*(?:class|struct)\s+([A-Za-z_]\w*)\b(?!.*;)')
            if ($classMatch.Success)
            {
                $viewModel = $classMatch.Groups[1].Value
            }
            $propMatch = [regex]::Match($lines[$i], 'Q_PROPERTY\(\s*\S+\s+([A-Za-z_]\w*)\s+(?:READ|MEMBER)\b')
            if (-not $propMatch.Success)
            {
                continue
            }
            $prop = $propMatch.Groups[1].Value
            if ([regex]::IsMatch($referenceCorpus, "\b$prop\b"))
            {
                continue
            }
            $findings += New-Finding -File $relative -Line ($i + 1) -Symbol "$viewModel::$prop" `
                -Message "$viewModel::$prop - Q_PROPERTY bound by no .qml and not by main.cpp (dead read model)"
        }
    }

    return $findings
}

$plant = {
    param([string]$dir)

    $viewmodelDir = Join-Path $dir 'src/viewmodel'
    $qmlDir = Join-Path $dir 'src/qml'
    New-Item -ItemType Directory -Path $viewmodelDir -Force | Out-Null
    New-Item -ItemType Directory -Path $qmlDir -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $dir 'src/main.cpp') -Encoding UTF8 -Value 'int main() { return 0; }'
    Set-Content -LiteralPath (Join-Path $qmlDir 'Idle.qml') -Encoding UTF8 -Value 'Item { }'
    Set-Content -LiteralPath (Join-Path $viewmodelDir 'PlantViewModel.h') -Encoding UTF8 -Value @(
        'class PlantViewModel'
        '{'
        '    Q_PROPERTY(bool plantedUnbound READ IsPlantedUnbound CONSTANT)'
        '};'
    )
    return 'PlantViewModel::plantedUnbound'
}

$allowlist = @()

Invoke-GuardCheck -Name 'check-viewmodel-property-unbound' `
    -Description 'no viewmodel Q_PROPERTY is left bound by nothing on screen' `
    -Detect $detect -PlantFixture $plant -Root $Root -Allowlist $allowlist
