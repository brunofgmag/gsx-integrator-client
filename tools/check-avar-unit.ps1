param([string]$Root)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'guard-check-lib.ps1')

$script:KnownUnits = @('kg', 'pounds', 'gallons', 'number', 'bool', 'percent', 'percent over 100', 'volts', 'knots')

function Get-StringConstants
{
    param([string]$Text)

    $map = @{}
    foreach ($hit in [regex]::Matches($Text, '(?m)^[ \t]*(?:inline\s+)?constexpr\s+auto\s+(\w+)\s*=\s*"([^"]*)"\s*;'))
    {
        $map[$hit.Groups[1].Value] = $hit.Groups[2].Value
    }

    return $map
}

function Add-DerivedNames
{
    param([hashtable]$Map, [string]$Text)

    foreach ($hit in [regex]::Matches($Text, '(?m)^[ \t]*(?:inline\s+)?std::string\s+(\w+)\s*\([^)]*\)\s*\{\s*return\s+(?:std::string\s*\(\s*)?([A-Za-z_]\w*)'))
    {
        $alias = $hit.Groups[2].Value
        if ($Map.ContainsKey($alias))
        {
            $Map[$hit.Groups[1].Value] = $Map[$alias]
        }
    }

    foreach ($hit in [regex]::Matches($Text, 'const\s+std::string\s+(\w+)\s*=\s*([A-Za-z_]\w*)'))
    {
        $alias = $hit.Groups[2].Value
        if ($Map.ContainsKey($alias))
        {
            $Map[$hit.Groups[1].Value] = $Map[$alias]
        }
    }

    return $Map
}

function Get-CallArguments
{
    param([string]$Text, [int]$Start)

    $parts = @()
    $current = ''
    $depth = 0
    $inString = $false

    for ($i = $Start; $i -lt $Text.Length; $i++)
    {
        $ch = $Text[$i]

        if ($inString)
        {
            $current += $ch
            if ($ch -eq '"' -and $Text[$i - 1] -ne '\')
            {
                $inString = $false
            }

            continue
        }

        if ($ch -eq '"')
        {
            $inString = $true
            $current += $ch

            continue
        }

        if ($ch -eq '(' -or $ch -eq '[' -or $ch -eq '{')
        {
            $depth++
            if (-not ($depth -eq 1 -and $ch -eq '('))
            {
                $current += $ch
            }

            continue
        }

        if ($ch -eq ')' -or $ch -eq ']' -or $ch -eq '}')
        {
            $depth--
            if ($depth -eq 0)
            {
                return $parts + $current
            }
            $current += $ch

            continue
        }

        if ($ch -eq ',' -and $depth -eq 1)
        {
            $parts += $current
            $current = ''

            continue
        }

        $current += $ch
    }

    return @()
}

function Resolve-Literal
{
    param([string]$Expression, [hashtable]$Local, [hashtable]$Shared)

    $trimmed = $Expression.Trim()

    if ($trimmed -match '^"([^"]*)"')
    {
        return $Matches[1]
    }

    if ($trimmed -match '^(?:std::string\s*\(\s*)?([A-Za-z_][\w:]*)')
    {
        $symbol = ($Matches[1] -split '::')[-1]
        if ($Local.ContainsKey($symbol))
        {
            return $Local[$symbol]
        }
        if ($Shared.ContainsKey($symbol))
        {
            return $Shared[$symbol]
        }
    }

    return $null
}

$detect = {
    param([string]$root)

    $srcDir = Join-Path $root 'src'
    if (-not (Test-Path -LiteralPath $srcDir))
    {
        return @()
    }

    $shared = @{}
    foreach ($header in Get-ChildItem -LiteralPath $srcDir -Recurse -Include *.h -File)
    {
        $headerText = [System.IO.File]::ReadAllText($header.FullName)
        $headerMap = Add-DerivedNames (Get-StringConstants $headerText) $headerText
        foreach ($entry in $headerMap.GetEnumerator())
        {
            $shared[$entry.Key] = $entry.Value
        }
    }

    $findings = @()
    $unitsByName = [System.Collections.Generic.Dictionary[string, object]]::new([System.StringComparer]::Ordinal)
    $firstSite = [System.Collections.Generic.Dictionary[string, object]]::new([System.StringComparer]::Ordinal)

    foreach ($file in Get-ChildItem -LiteralPath $srcDir -Recurse -Include *.cpp, *.h -File)
    {
        $relative = $file.FullName.Substring($root.Length).TrimStart('\', '/')
        $text = [System.IO.File]::ReadAllText($file.FullName)
        $constants = Add-DerivedNames (Get-StringConstants $text) $text

        foreach ($call in [regex]::Matches($text, '\b(GetAVar|SetAVar|HasReceivedAVar)\s*\('))
        {
            $open = $text.IndexOf('(', $call.Index)
            $parts = @(Get-CallArguments $text $open)
            if ($parts.Count -lt 2)
            {
                continue
            }

            $unit = Resolve-Literal $parts[1] $constants $shared
            if (-not $unit)
            {
                continue
            }

            $line = ($text.Substring(0, $call.Index) -split "`n").Count
            $verb = $call.Groups[1].Value

            if ($script:KnownUnits -notcontains $unit)
            {
                $findings += New-Finding -File $relative -Line $line -Symbol "unit '$unit'" `
                    -Message "$verb asks the simulator for '$unit', which is not a unit string this tree knows - SimConnect rejects it and the read keeps the default forever"
            }

            $name = Resolve-Literal $parts[0] $constants $shared
            if (-not $name)
            {
                continue
            }

            if (-not $unitsByName.ContainsKey($name))
            {
                $unitsByName[$name] = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
                $firstSite[$name] = [pscustomobject]@{ File = $relative; Line = $line }
            }

            [void]$unitsByName[$name].Add($unit)
        }
    }

    foreach ($name in $unitsByName.Keys)
    {
        $units = @($unitsByName[$name] | Sort-Object)
        if ($units.Count -lt 2)
        {
            continue
        }

        $joined = $units -join ' and '
        $findings += New-Finding -File $firstSite[$name].File -Line $firstSite[$name].Line `
            -Symbol "$name as $joined" `
            -Message "'$name' is reached as $joined - the gateway keys one slot per name and unit, so these are two slots and the sim converts the wrong one without complaining"
    }

    return $findings
}

$plant = {
    param([string]$dir)

    $srcDir = Join-Path $dir 'src'
    New-Item -ItemType Directory -Path $srcDir -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $srcDir 'Planted.cpp') -Encoding UTF8 -Value @(
        'namespace'
        '{'
        '    constexpr auto kPlantedVar = "PLANTED WEIGHT";'
        '    constexpr auto kPlantedUnit = "kg";'
        '    constexpr auto kPlantedOtherUnit = "furlongs";'
        '}'
        ''
        'double PlantedWeight(VariableReader& variables)'
        '{'
        '    if (!variables.HasReceivedAVar(kPlantedVar, kPlantedUnit))'
        '    {'
        '        return 0.0;'
        '    }'
        ''
        '    return variables.GetAVar(kPlantedVar, kPlantedOtherUnit, 0.0);'
        '}'
    )

    return @("unit 'furlongs'", 'PLANTED WEIGHT as furlongs and kg')
}

$allowlist = @(
    @{
        Symbol = 'FUEL WEIGHT PER GALLON as kg and pounds'
        Reason = 'AvroRj asks in kg and Fss727 in pounds, each from the constant its own file owns; one adapter lives per session, so the two slots never coexist'
    }
    @{
        Symbol = 'PAYLOAD STATION WEIGHT: as kg and pounds'
        Reason = 'IFly737Max writes in kg and Fss727 in pounds, each from the constant its own file owns; one adapter lives per session, so the two slots never coexist'
    }
)

Invoke-GuardCheck -Name 'check-avar-unit' `
    -Description 'every AVar access names a unit the simulator knows, and no variable is reached under two units' `
    -Detect $detect -PlantFixture $plant -Root $Root -Allowlist $allowlist
