param([string]$Root)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'guard-check-lib.ps1')

$detect = {
    param([string]$root)

    $header = Join-Path $root 'src/domain/turnaround/TurnaroundData.h'
    if (-not (Test-Path -LiteralPath $header))
    {
        return @()
    }

    $referenceCorpus = ''
    $srcDir = Join-Path $root 'src'
    foreach ($file in Get-ChildItem -LiteralPath $srcDir -Recurse -Include *.h, *.cpp -File)
    {
        if ($file.FullName -eq (Resolve-Path -LiteralPath $header).Path)
        {
            continue
        }
        $referenceCorpus += [System.IO.File]::ReadAllText($file.FullName) + "`n"
    }

    $relative = 'src/domain/turnaround/TurnaroundData.h'
    $lines = [System.IO.File]::ReadAllLines($header)
    $findings = @()
    for ($i = 0; $i -lt $lines.Length; $i++)
    {
        $fieldMatch = [regex]::Match($lines[$i], '^\s*[A-Za-z_][\w:<>]*\s+([a-z]\w*)\s*=\s*[^;(]*;\s*$')
        if (-not $fieldMatch.Success)
        {
            continue
        }
        $field = $fieldMatch.Groups[1].Value
        if ([regex]::IsMatch($referenceCorpus, "\b$field\b"))
        {
            continue
        }
        $findings += New-Finding -File $relative -Line ($i + 1) -Symbol "TurnaroundData::$field" `
            -Message "TurnaroundData::$field - field referenced only inside its own header (dead working state)"
    }

    return $findings
}

$plant = {
    param([string]$dir)

    $turnaroundDir = Join-Path $dir 'src/domain/turnaround'
    New-Item -ItemType Directory -Path $turnaroundDir -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $turnaroundDir 'TurnaroundData.h') -Encoding UTF8 -Value @(
        'struct TurnaroundData'
        '{'
        '    int plantedUnreadField = 0;'
        '};'
    )
    return 'TurnaroundData::plantedUnreadField'
}

$allowlist = @()

Invoke-GuardCheck -Name 'check-turnaround-data-field-unread' `
    -Description 'no TurnaroundData field is left referenced only in its own header' `
    -Detect $detect -PlantFixture $plant -Root $Root -Allowlist $allowlist
