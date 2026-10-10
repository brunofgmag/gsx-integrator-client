param([string]$Root)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'guard-check-lib.ps1')

$relative = 'src/domain/turnaround/TurnaroundData.h'

$detect = {
    param([string]$root)

    $header = Join-Path $root $relative
    if (-not (Test-Path -LiteralPath $header))
    {
        return @(New-Finding -File $relative -Line 0 -Symbol 'TurnaroundData.h' `
            -Message "$relative is missing, so there are no fields to check for readers")
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

    $lines = [System.IO.File]::ReadAllLines($header)
    $findings = @()
    foreach ($structName in @('TurnaroundData', 'CabinServiceProgress'))
    {
        $members = @(Get-StructMembers -Lines $lines -StructName $structName)
        if ($members.Count -eq 0)
        {
            $findings += New-Finding -File $relative -Line 0 -Symbol $structName `
                -Message "$structName yields no members; the header changed shape or the guard no longer reads it"
            continue
        }

        foreach ($member in $members)
        {
            if ([regex]::IsMatch($referenceCorpus, "\b$($member.Name)\b"))
            {
                continue
            }
            $findings += New-Finding -File $relative -Line $member.Line -Symbol "$structName::$($member.Name)" `
                -Message "$structName::$($member.Name) - field referenced only inside its own header (dead working state)"
        }
    }

    return $findings
}

$plant = {
    param([string]$dir)

    $turnaroundDir = Join-Path $dir 'src/domain/turnaround'
    New-Item -ItemType Directory -Path $turnaroundDir -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $turnaroundDir 'TurnaroundData.h') -Encoding UTF8 -Value @(
        'struct CabinServiceProgress'
        '{'
        '    bool plantedNestedUnreadField = false;'
        '};'
        'struct TurnaroundData'
        '{'
        '    int plantedUnreadField = 0;'
        '};'
    )
    return @(
        'TurnaroundData::plantedUnreadField'
        'CabinServiceProgress::plantedNestedUnreadField'
    )
}

$allowlist = @()

Invoke-GuardCheck -Name 'check-turnaround-data-field-unread' `
    -Description 'no TurnaroundData or CabinServiceProgress field is left referenced only in its own header' `
    -Detect $detect -PlantFixture $plant -Root $Root -Allowlist $allowlist
