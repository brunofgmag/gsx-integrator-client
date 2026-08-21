param([string]$Root)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'guard-check-lib.ps1')

$detect = {
    param([string]$root)

    $header = Join-Path $root 'src/infrastructure/gsx/GsxRemoteState.h'
    if (-not (Test-Path -LiteralPath $header))
    {
        return @()
    }

    $headerPath = (Resolve-Path -LiteralPath $header).Path
    $reducer = Join-Path $root 'src/infrastructure/gsx/GsxRemoteStateReducer.cpp'
    $reducerPath = if (Test-Path -LiteralPath $reducer) { (Resolve-Path -LiteralPath $reducer).Path } else { '' }

    $fieldPattern = '^\s{4}(?!return)[A-Za-z_][\w:]*(?:<[^>]+>)?\s+([a-z]\w*)\s*(?:=[^;]*)?;\s*$'

    $readerCorpus = ''
    foreach ($line in [System.IO.File]::ReadAllLines($header))
    {
        if (-not [regex]::IsMatch($line, $fieldPattern))
        {
            $readerCorpus += $line + "`n"
        }
    }

    $srcDir = Join-Path $root 'src'
    foreach ($file in Get-ChildItem -LiteralPath $srcDir -Recurse -Include *.h, *.cpp -File)
    {
        if ($file.FullName -eq $headerPath -or $file.FullName -eq $reducerPath)
        {
            continue
        }
        $text = [System.IO.File]::ReadAllText($file.FullName)
        if ($text -notmatch 'GsxRemote')
        {
            continue
        }
        $readerCorpus += $text + "`n"
    }

    $relative = 'src/infrastructure/gsx/GsxRemoteState.h'
    $lines = [System.IO.File]::ReadAllLines($header)
    $findings = @()
    $struct = ''
    for ($i = 0; $i -lt $lines.Length; $i++)
    {
        $structMatch = [regex]::Match($lines[$i], '^\s*struct\s+([A-Za-z_]\w*)\s*$')
        if ($structMatch.Success)
        {
            $struct = $structMatch.Groups[1].Value
            continue
        }

        $fieldMatch = [regex]::Match($lines[$i], $fieldPattern)
        if (-not $fieldMatch.Success)
        {
            continue
        }

        $field = $fieldMatch.Groups[1].Value
        if ([regex]::IsMatch($readerCorpus, "(\.|->)$field\b"))
        {
            continue
        }

        $findings += New-Finding -File $relative -Line ($i + 1) -Symbol "$struct::$field" `
            -Message "$struct::$field - parsed by the reducer and never read outside it"
    }

    return $findings
}

$plant = {
    param([string]$dir)

    $gsxDir = Join-Path $dir 'src/infrastructure/gsx'
    New-Item -ItemType Directory -Path $gsxDir -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $gsxDir 'GsxRemoteState.h') -Encoding UTF8 -Value @(
        'struct GsxRemoteState'
        '{'
        '    int plantedUnreadField = 0;'
        '};'
    )
    return 'GsxRemoteState::plantedUnreadField'
}

$allowlist = @()

Invoke-GuardCheck -Name 'check-remote-state-field-unread' `
    -Description 'no GsxRemoteState field is parsed without a reader outside the reducer' `
    -Detect $detect -PlantFixture $plant -Root $Root -Allowlist $allowlist
