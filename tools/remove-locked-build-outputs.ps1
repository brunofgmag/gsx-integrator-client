param(
    [Parameter(Mandatory)]
    [string]$Directory
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $Directory)) { return }

$locked = Get-ChildItem -LiteralPath $Directory -File -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.Extension -in '.exe', '.pdb', '.dll', '.obj', '.lib' } |
    Where-Object {
        try {
            $stream = [IO.File]::Open($_.FullName, 'Open', 'Write', 'None')
            $stream.Close()
            $false
        }
        catch { $true }
    }

foreach ($file in $locked) {
    Remove-Item -LiteralPath $file.FullName -Force -ErrorAction SilentlyContinue
    if (Test-Path -LiteralPath $file.FullName) {
        Write-Host "Still locked after delete: $($file.FullName)"
    }
    else {
        Write-Host "Removed locked build output: $($file.FullName)"
    }
}
