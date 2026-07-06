param(
    [Parameter(Mandatory)]
    [string]$DeploymentDir,

    [ValidateSet('Debug', 'Release', 'RelWithDebInfo')]
    [string]$Configuration = 'Release',

    [double]$MaximumSizeMiB = 0,

    [int]$MaximumFileCount = 130
)

$ErrorActionPreference = 'Stop'

$root = [IO.Path]::GetFullPath($DeploymentDir)
$isDebug = $Configuration -eq 'Debug'
$suffix = if ($isDebug) { 'd' } else { '' }
if ($MaximumSizeMiB -le 0) {
    $MaximumSizeMiB = if ($isDebug) { 250 } else { 45 }
}

$requiredPaths = @(
    'gsx-integrator-client.exe',
    'SimConnect.dll',
    "Qt6Core$suffix.dll",
    "Qt6Network$suffix.dll",
    "Qt6WebSockets$suffix.dll",
    "Qt6Qml$suffix.dll",
    "Qt6Quick$suffix.dll",
    "platforms/qwindows$suffix.dll",
    "tls/qschannelbackend$suffix.dll",
    'qml/QtQuick/qmldir',
    'qml/QtQuick/Controls/Basic/qmldir',
    'qml/QtQuick/Layouts/qmldir'
)
if (-not $isDebug) {
    $requiredPaths += @(
        'msvcp140.dll',
        'vcruntime140.dll',
        'vcruntime140_1.dll'
    )
}
$forbiddenPaths = @(
    'D3Dcompiler_47.dll',
    'dxcompiler.dll',
    'dxil.dll',
    'opengl32sw.dll',
    'vc_redist.x64.exe',
    'Qt6Pdf.dll',
    'Qt6Pdfd.dll',
    'Qt6Svg.dll',
    'Qt6Svgd.dll',
    'Qt6VirtualKeyboard.dll',
    'Qt6VirtualKeyboardd.dll',
    'iconengines',
    'imageformats',
    'networkinformation',
    'platforminputcontexts'
)

$missing = $requiredPaths | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $root $_))
}
if ($missing) {
    throw "Deployment is missing required paths:`n$($missing -join "`n")"
}

$unexpected = $forbiddenPaths | Where-Object {
    Test-Path -LiteralPath (Join-Path $root $_)
}
if ($unexpected) {
    throw "Deployment contains unused paths:`n$($unexpected -join "`n")"
}

$files = @(Get-ChildItem -LiteralPath $root -File -Recurse)
$totalBytes = ($files | Measure-Object Length -Sum).Sum
$totalMiB = [math]::Round($totalBytes / 1MB, 2)

if ($totalMiB -gt $MaximumSizeMiB) {
    throw "Deployment is $totalMiB MiB; maximum allowed is $MaximumSizeMiB MiB."
}
if ($files.Count -gt $MaximumFileCount) {
    throw "Deployment contains $($files.Count) files; maximum allowed is $MaximumFileCount."
}

$exeSize = (Get-Item -LiteralPath (Join-Path $root 'gsx-integrator-client.exe')).Length
if ($exeSize -lt 100KB) {
    throw "Executable is suspiciously small: $exeSize bytes."
}

Write-Host "Deployment verified: $($files.Count) files, $totalMiB MiB."
