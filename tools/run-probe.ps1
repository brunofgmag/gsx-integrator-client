param(
    [int]$ToggleEvent = 0,
    [int]$ToggleDoor = -1,
    [string]$PressGroundConn = '',
    [string]$SetLVar = ''
)

$ErrorActionPreference = 'Stop'

$exe = Join-Path $PSScriptRoot '..\build\debug\bin\gsx-integrator-client.exe'
if (-not (Test-Path -LiteralPath $exe)) {
    throw "Build it first: .\build.ps1 -Config Debug. The probe only exists in a Debug build: probe::IsOn() returns false wherever NDEBUG is defined, which is every configuration except Debug."
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

if ($ToggleDoor -ge 0) {
    $env:GSXI_PROBE_DOOR = "$ToggleDoor"
    Write-Host "Will toggle PMDG 777 door slot $ToggleDoor once, after the aircraft is powered"
}
else {
    Remove-Item Env:\GSXI_PROBE_DOOR -ErrorAction SilentlyContinue
}

if ($PressGroundConn) {
    $env:GSXI_PROBE_GROUND_CONN = $PressGroundConn
    Write-Host "Will press the PMDG tablet ground_conn button '$PressGroundConn' once, as soon as the tablet answers"
}
else {
    Remove-Item Env:\GSXI_PROBE_GROUND_CONN -ErrorAction SilentlyContinue
}

if ($SetLVar) {
    $env:GSXI_PROBE_SET_LVAR = $SetLVar
    Write-Host "Will write the LVar '$SetLVar' once, as NAME=VALUE or NAME=VALUE@ARM, waiting for the LVar to read ARM"
}
else {
    Remove-Item Env:\GSXI_PROBE_SET_LVAR -ErrorAction SilentlyContinue
}

$folder = Join-Path $env:LOCALAPPDATA 'brunofgmag\gsx-integrator-client\probe'
Write-Host "Probe readings land in $folder"

& $exe
