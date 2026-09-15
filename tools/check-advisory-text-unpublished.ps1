param([string]$Root)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'guard-check-lib.ps1')

$detect = {
    param([string]$root)

    $header = Join-Path $root 'src/viewmodel/OperationsViewModel.h'
    $publisher = Join-Path $root 'src/infrastructure/efb/EfbStatePublisher.cpp'
    if (-not (Test-Path -LiteralPath $header) -or -not (Test-Path -LiteralPath $publisher))
    {
        return @()
    }

    $published = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($hit in [regex]::Matches([System.IO.File]::ReadAllText($publisher), '\bGet\w*AdvisoryText\b'))
    {
        [void]$published.Add($hit.Value)
    }

    $findings = @()
    $seen = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    $relative = $header.Substring($root.Length).TrimStart('\', '/')
    $lines = [System.IO.File]::ReadAllLines($header)
    for ($i = 0; $i -lt $lines.Length; $i++)
    {
        foreach ($hit in [regex]::Matches($lines[$i], '\bGet\w*AdvisoryText\b'))
        {
            $getter = $hit.Value
            if ($published.Contains($getter) -or -not $seen.Add($getter))
            {
                continue
            }

            $findings += New-Finding -File $relative -Line ($i + 1) -Symbol $getter `
                -Message "$getter - advisory the window writes and the EFB App never receives; publish it in EfbStatePublisher so both screens read the same sentence"
        }
    }

    return $findings
}

$plant = {
    param([string]$dir)

    $viewmodelDir = Join-Path $dir 'src/viewmodel'
    $efbDir = Join-Path $dir 'src/infrastructure/efb'
    New-Item -ItemType Directory -Path $viewmodelDir -Force | Out-Null
    New-Item -ItemType Directory -Path $efbDir -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $viewmodelDir 'OperationsViewModel.h') -Encoding UTF8 -Value @(
        'class OperationsViewModel'
        '{'
        '    [[nodiscard]] static QString GetPublishedAdvisoryText();'
        '    [[nodiscard]] static QString GetPlantedAdvisoryText();'
        '};'
    )
    Set-Content -LiteralPath (Join-Path $efbDir 'EfbStatePublisher.cpp') -Encoding UTF8 -Value @(
        'void EfbStatePublisher::BuildPayload() const'
        '{'
        '    state.insert(QLatin1String("publishedAdvisoryText"), OperationsViewModel::GetPublishedAdvisoryText());'
        '}'
    )

    return 'GetPlantedAdvisoryText'
}

$allowlist = @()

Invoke-GuardCheck -Name 'check-advisory-text-unpublished' `
    -Description 'every advisory sentence the window writes is published to the EFB App' `
    -Detect $detect -PlantFixture $plant -Root $Root -Allowlist $allowlist
