param(
    [ValidateSet('Debug', 'Release')]
    [string]$Config = 'Release',
    [int]$ToggleEvent = 0
)

$ErrorActionPreference = 'Stop'

$exe = Join-Path $PSScriptRoot "..\build\$($Config.ToLowerInvariant())\bin\gsx-integrator-client.exe"
if (-not (Test-Path -LiteralPath $exe)) {
    throw "Build it first: .\build.ps1 -Config $Config"
}

$env:GSXI_PROBE = '1'
$env:GSXI_NO_UPDATES = '1'

if ($ToggleEvent -gt 0) {
    $env:GSXI_PROBE_TOGGLE = "$ToggleEvent"
    Write-Host "Will fire PMDG SDK event $ToggleEvent once, after a main AC bus goes live"
}
else {
    Remove-Item Env:\GSXI_PROBE_TOGGLE -ErrorAction SilentlyContinue
}

$folder = Join-Path $env:LOCALAPPDATA 'brunofgmag\gsx-integrator-client\probe'
Write-Host "Probe readings land in $folder"

& $exe
