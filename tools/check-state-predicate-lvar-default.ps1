param([string]$Root)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'guard-check-lib.ps1')

$detect = {
    param([string]$root)

    $srcDir = Join-Path $root 'src'
    if (-not (Test-Path -LiteralPath $srcDir))
    {
        return @()
    }

    $findings = @()
    foreach ($file in Get-ChildItem -LiteralPath $srcDir -Recurse -Include *.cpp, *.h -File)
    {
        $relative = $file.FullName.Substring($root.Length).TrimStart('\', '/')
        $lines = [System.IO.File]::ReadAllLines($file.FullName)
        $depth = 0
        $pending = ''
        $currentFn = ''
        for ($i = 0; $i -lt $lines.Length; $i++)
        {
            $line = $lines[$i]

            if ($depth -eq 0)
            {
                $sigMatches = [regex]::Matches($line, '\b([A-Za-z_]\w*)\s*\(')
                if ($sigMatches.Count -gt 0)
                {
                    $pending = $sigMatches[$sigMatches.Count - 1].Groups[1].Value
                }
            }

            $prevDepth = $depth
            $opens = ([regex]::Matches($line, '\{')).Count
            $closes = ([regex]::Matches($line, '\}')).Count
            $depth += $opens - $closes
            if ($depth -lt 0)
            {
                $depth = 0
            }

            if ($prevDepth -eq 0 -and $depth -gt 0)
            {
                $currentFn = $pending
            }
            elseif ($depth -eq 0)
            {
                $currentFn = ''
            }

            if ($currentFn -notmatch '^(Is|Are|Has|Can)[A-Z0-9]')
            {
                continue
            }

            $lvarMatch = [regex]::Match($line, 'GetLVar\(\s*"(FSDT_GSX_[^"]*)"\s*\)')
            if (-not $lvarMatch.Success)
            {
                continue
            }
            $lvar = $lvarMatch.Groups[1].Value
            $findings += New-Finding -File $relative -Line ($i + 1) -Symbol "$currentFn::$lvar" `
                -Message "$currentFn reads $lvar with GetLVar and no second argument - the 0.0 default is unsafe in a state predicate"
        }
    }

    return $findings
}

$plant = {
    param([string]$dir)

    $srcDir = Join-Path $dir 'src'
    New-Item -ItemType Directory -Path $srcDir -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $srcDir 'Planted.cpp') -Encoding UTF8 -Value @(
        'bool IsPlantedReady()'
        '{'
        '    return GetLVar("FSDT_GSX_PLANTED") > 0.5;'
        '}'
    )
    return 'IsPlantedReady::FSDT_GSX_PLANTED'
}

$allowlist = @()

Invoke-GuardCheck -Name 'check-state-predicate-lvar-default' `
    -Description 'no Is/Are/Has/Can predicate reads an FSDT_GSX LVar without a conservative default' `
    -Detect $detect -PlantFixture $plant -Root $Root -Allowlist $allowlist
