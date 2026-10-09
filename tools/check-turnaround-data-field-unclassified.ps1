param([string]$Root)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'guard-check-lib.ps1')

$relative = 'src/domain/turnaround/TurnaroundData.h'
$nestedStruct = 'CabinServiceProgress'

$detect = {
    param([string]$root)

    $header = Join-Path $root $relative
    if (-not (Test-Path -LiteralPath $header))
    {
        return @(New-Finding -File $relative -Line 0 -Symbol 'TurnaroundData.h' `
            -Message "$relative is missing, so there is nothing to check the restore visitor against")
    }

    $lines = [System.IO.File]::ReadAllLines($header)
    $visitor = Get-FunctionBody -Lines $lines -FunctionName 'VisitFields'
    $outerMembers = @(Get-StructMembers -Lines $lines -StructName 'TurnaroundData')
    $nestedMembers = @(Get-StructMembers -Lines $lines -StructName $nestedStruct)

    $findings = @()
    if ($outerMembers.Count -eq 0)
    {
        $findings += New-Finding -File $relative -Line 0 -Symbol 'TurnaroundData' `
            -Message 'TurnaroundData yields no members; the header changed shape or the guard no longer reads it'
    }
    if ($nestedMembers.Count -eq 0)
    {
        $findings += New-Finding -File $relative -Line 0 -Symbol $nestedStruct `
            -Message "$nestedStruct yields no members; the header changed shape or the guard no longer reads it"
    }

    $holders = @($outerMembers | Where-Object { $_.Type -eq $nestedStruct })
    if ($holders.Count -eq 0 -and $nestedMembers.Count -gt 0)
    {
        $findings += New-Finding -File $relative -Line 0 -Symbol $nestedStruct `
            -Message "no TurnaroundData member holds a $nestedStruct, so its members are classified by nobody"
    }

    foreach ($member in $outerMembers)
    {
        if ($member.Type -eq $nestedStruct -or [regex]::IsMatch($visitor, "&TurnaroundData::$($member.Name)\b"))
        {
            continue
        }
        $findings += New-Finding -File $relative -Line $member.Line -Symbol "TurnaroundData::$($member.Name)" `
            -Message "TurnaroundData::$($member.Name) - member is not named in turnaround::VisitFields, so nothing says whether it returns raw, restarts, is rebuilt or is not saved"
    }

    foreach ($holder in $holders)
    {
        foreach ($inner in $nestedMembers)
        {
            $pair = "&TurnaroundData::$($holder.Name)\s*,\s*&$nestedStruct::$($inner.Name)\b"
            if ([regex]::IsMatch($visitor, $pair))
            {
                continue
            }
            $symbol = "TurnaroundData::$($holder.Name).$($inner.Name)"
            $findings += New-Finding -File $relative -Line $inner.Line -Symbol $symbol `
                -Message "$symbol - nested member is not named in turnaround::VisitFields for this holder"
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
        '    bool namedForFirstHolderOnly = false;'
        '    bool namedForNobody = false;'
        '};'
        'struct TurnaroundData'
        '{'
        '    int classifiedField = 0;'
        '    int plantedUnclassifiedField = 0;'
        '    std::set<std::string> plantedBareMember;'
        '    CabinServiceProgress firstHolder;'
        '    CabinServiceProgress secondHolder;'
        '};'
        'namespace turnaround'
        '{'
        '    template <typename Visitor>'
        '    void VisitFields(Visitor&& visit)'
        '    {'
        '        visit("classifiedField", Field(&TurnaroundData::classifiedField), FieldRestore::Raw);'
        '        visit("firstHolder.namedForFirstHolderOnly", Field(&TurnaroundData::firstHolder, &CabinServiceProgress::namedForFirstHolderOnly), FieldRestore::Raw);'
        '    }'
        '}'
    )
    return @(
        'TurnaroundData::plantedUnclassifiedField'
        'TurnaroundData::plantedBareMember'
        'TurnaroundData::secondHolder.namedForFirstHolderOnly'
        'TurnaroundData::firstHolder.namedForNobody'
        'TurnaroundData::secondHolder.namedForNobody'
    )
}

$allowlist = @()

Invoke-GuardCheck -Name 'check-turnaround-data-field-unclassified' `
    -Description 'every TurnaroundData member, and every nested member of each holder, is named in the restore visitor' `
    -Detect $detect -PlantFixture $plant -Root $Root -Allowlist $allowlist
