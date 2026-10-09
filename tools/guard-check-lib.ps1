Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function New-Finding
{
    param([string]$File, [int]$Line, [string]$Symbol, [string]$Message)

    return [pscustomobject]@{ File = $File; Line = $Line; Symbol = $Symbol; Message = $Message }
}

function Invoke-GuardCheck
{
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Description,
        [Parameter(Mandatory)][scriptblock]$Detect,
        [Parameter(Mandatory)][scriptblock]$PlantFixture,
        [string]$Root,
        [object[]]$Allowlist = @()
    )

    if (-not $Root)
    {
        $Root = Split-Path -Parent $PSScriptRoot
    }
    $Root = (Resolve-Path -LiteralPath $Root).Path

    $fixtureRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("guard-$Name-" + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $fixtureRoot -Force | Out-Null
    try
    {
        $planted = @(& $PlantFixture $fixtureRoot)
        $selfHits = @(& $Detect $fixtureRoot)
        foreach ($symbol in $planted)
        {
            if (-not ($selfHits | Where-Object { $_.Symbol -eq $symbol }))
            {
                Write-Host "SELF-TEST FAILED [$Name]: detector no longer flags planted '$symbol'."
                Write-Host "The guard is asleep. Fix the detector before trusting a green run."
                exit 2
            }
        }
    }
    finally
    {
        Remove-Item -Recurse -Force -LiteralPath $fixtureRoot -ErrorAction SilentlyContinue
    }

    $found = @(& $Detect $Root)
    $allowSymbols = @{}
    foreach ($entry in $Allowlist)
    {
        $allowSymbols[$entry.Symbol] = $entry.Reason
    }

    $regressions = @($found | Where-Object { -not $allowSymbols.ContainsKey($_.Symbol) })
    $foundSymbols = @{}
    foreach ($hit in $found)
    {
        $foundSymbols[$hit.Symbol] = $true
    }
    $stale = @($Allowlist | Where-Object { -not $foundSymbols.ContainsKey($_.Symbol) })

    if ($regressions.Count -eq 0 -and $stale.Count -eq 0)
    {
        $baseline = if ($Allowlist.Count -gt 0) { " ($($Allowlist.Count) baselined)" } else { "" }
        Write-Host "[$Name] OK - $Description$baseline"
        exit 0
    }

    Write-Host "[$Name] FAILED - $Description"
    Write-Host ""
    foreach ($hit in ($regressions | Sort-Object File, Line))
    {
        Write-Host ("  {0}:{1}: {2}" -f $hit.File, $hit.Line, $hit.Message)
    }
    if ($regressions.Count -gt 0)
    {
        Write-Host ""
        Write-Host "  $($regressions.Count) unexpected finding(s). Either fix the code, or - if intended - add the symbol to the allowlist with a reason."
    }
    foreach ($entry in $stale)
    {
        Write-Host "  STALE ALLOWLIST: '$($entry.Symbol)' is no longer a finding. Remove it from $Name.ps1 so the baseline keeps shrinking."
    }
    exit 1
}

function Get-StructMembers
{
    param([string[]]$Lines, [string]$StructName)

    $memberPattern = '^\s*([A-Za-z_][\w:<>, ]*?)\s+([a-z]\w*)\s*(?:=[^;(]*)?;\s*$'
    $members = @()
    $depth = 0
    $inside = $false
    for ($i = 0; $i -lt $Lines.Length; $i++)
    {
        $line = $Lines[$i]
        if (-not $inside)
        {
            if ($line -match "^\s*struct\s+$StructName\b")
            {
                $inside = $true
                $depth += ([regex]::Matches($line, '\{')).Count - ([regex]::Matches($line, '\}')).Count
            }
            continue
        }

        if ($depth -eq 1)
        {
            $memberMatch = [regex]::Match($line, $memberPattern)
            if ($memberMatch.Success)
            {
                $members += [pscustomobject]@{
                    Type = $memberMatch.Groups[1].Value
                    Name = $memberMatch.Groups[2].Value
                    Line = $i + 1
                }
            }
        }

        $depth += ([regex]::Matches($line, '\{')).Count - ([regex]::Matches($line, '\}')).Count
        if ($depth -le 0)
        {
            break
        }
    }

    return $members
}

function Get-FunctionBody
{
    param([string[]]$Lines, [string]$FunctionName)

    $body = @()
    $depth = 0
    $entered = $false
    $inside = $false
    foreach ($line in $Lines)
    {
        if (-not $inside)
        {
            if ($line -notmatch "\b$FunctionName\s*\(")
            {
                continue
            }
            $inside = $true
        }

        $body += $line
        $depth += ([regex]::Matches($line, '\{')).Count - ([regex]::Matches($line, '\}')).Count
        if ($depth -gt 0)
        {
            $entered = $true
        }
        if ($entered -and $depth -le 0)
        {
            break
        }
    }

    return ($body -join "`n")
}
