param(
    [Parameter(Mandatory)]
    [string]$DeploymentDir
)

$ErrorActionPreference = 'Stop'

$root = [IO.Path]::GetFullPath($DeploymentDir)
$exe = Join-Path $root 'gsx-integrator-client.exe'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
    throw "Refusing to prune a directory without gsx-integrator-client.exe: $root"
}

$unusedPaths = @(
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

foreach ($relativePath in $unusedPaths) {
    $path = Join-Path $root $relativePath
    if (Test-Path -LiteralPath $path) {
        Remove-Item -LiteralPath $path -Recurse -Force
    }
}

# windeployqt may create empty directories for optional QML modules.
Get-ChildItem -LiteralPath $root -Directory -Recurse |
    Sort-Object { $_.FullName.Length } -Descending |
    Where-Object { -not (Get-ChildItem -LiteralPath $_.FullName -Force) } |
    Remove-Item -Force
