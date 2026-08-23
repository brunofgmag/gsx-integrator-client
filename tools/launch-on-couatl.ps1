param(
    [string]$ProcessName = 'couatl64_MSFS2024',
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo')]
    [string]$Config = 'Debug',
    [int]$PollMs = 50,
    [int]$TimeoutSeconds = 900,
    [switch]$IncludeRunning,
    [switch]$NoProbe,
    [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Get-Watched
{
    param([string]$Name)

    try
    {
        return @(Get-Process -Name $Name -ErrorAction Stop)
    } catch
    {
        return @()
    }
}

function Format-Stamp
{
    param([DateTime]$Moment)

    return $Moment.ToString('HH:mm:ss.fff')
}

function Get-Identity
{
    param($Process)

    try
    {
        return "$( $Process.Id )@$( $Process.StartTime.Ticks )"
    } catch
    {
        return "$( $Process.Id )@unknown"
    }
}

$exe = Join-Path $PSScriptRoot "..\build\$( $Config.ToLowerInvariant() )\bin\gsx-integrator-client.exe"
if (-not $DryRun -and -not (Test-Path -LiteralPath $exe))
{
    throw "Build it first: .\build.ps1 -Config $Config. Expected $exe"
}

if (-not $DryRun -and -not $NoProbe -and $Config -ne 'Debug')
{
    throw "The probe only exists in a Debug build (ADR-0027): $Config would run and write nothing, without warning. Use -Config Debug or -NoProbe."
}

$baseline = @{ }
if (-not $IncludeRunning)
{
    foreach ($process in Get-Watched -Name $ProcessName)
    {
        $baseline[(Get-Identity -Process $process)] = $true
    }
}

Write-Host "==> Watching for a new '$ProcessName' every $PollMs ms, up to $TimeoutSeconds s."
if ($baseline.Count -gt 0)
{
    Write-Host "==> Ignoring the $( $baseline.Count ) already running: $( ($baseline.Keys | Sort-Object) -join ', ' ). Load the aircraft to restart it."
}

$deadline = [DateTime]::Now.AddSeconds($TimeoutSeconds)
$target = $null

while ([DateTime]::Now -lt $deadline)
{
    foreach ($process in Get-Watched -Name $ProcessName)
    {
        if (-not $baseline.ContainsKey((Get-Identity -Process $process)))
        {
            $target = $process
            break
        }
    }

    if ($target)
    {
        break
    }

    Start-Sleep -Milliseconds $PollMs
}

if (-not $target)
{
    Write-Host "==> No new '$ProcessName' within $TimeoutSeconds s. Nothing launched."
    exit 1
}

$seen = [DateTime]::Now
$born = $target.StartTime

Write-Host ("==> {0} pid {1} was born at {2}, seen at {3} (+{4} ms)" -f `
        $ProcessName, $target.Id, (Format-Stamp $born), (Format-Stamp $seen), [int]($seen - $born).TotalMilliseconds)

if ($DryRun)
{
    Write-Host "==> Dry run: would launch $exe now."
    exit 0
}

if (-not $NoProbe)
{
    $env:GSXI_PROBE = '1'
    $env:GSXI_NO_UPDATES = '1'
    Write-Host "==> Probe readings land in $( Join-Path $env:LOCALAPPDATA 'brunofgmag\gsx-integrator-client\probe' )"
}

$client = Start-Process -FilePath $exe -PassThru

Write-Host ("==> Client pid {0} was born at {1}, {2} ms after the Couatl. The window closes around 5 s." -f `
        $client.Id, (Format-Stamp $client.StartTime), [int]($client.StartTime - $born).TotalMilliseconds)
