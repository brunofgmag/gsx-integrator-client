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
        $planted = & $PlantFixture $fixtureRoot
        $selfHits = @(& $Detect $fixtureRoot)
        if (-not ($selfHits | Where-Object { $_.Symbol -eq $planted }))
        {
            Write-Host "SELF-TEST FAILED [$Name]: detector no longer flags planted '$planted'."
            Write-Host "The guard is asleep. Fix the detector before trusting a green run."
            exit 2
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
