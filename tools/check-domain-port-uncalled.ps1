param([string]$Root)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'guard-check-lib.ps1')

$detect = {
    param([string]$root)

    $portsDir = Join-Path $root 'src/domain/ports'
    if (-not (Test-Path -LiteralPath $portsDir))
    {
        return @()
    }

    $callerCorpus = ''
    $domainDir = Join-Path $root 'src/domain'
    if (Test-Path -LiteralPath $domainDir)
    {
        foreach ($file in Get-ChildItem -LiteralPath $domainDir -Recurse -Include *.h, *.cpp -File)
        {
            if ($file.FullName -like (Join-Path $portsDir '*'))
            {
                continue
            }
            $callerCorpus += [System.IO.File]::ReadAllText($file.FullName) + "`n"
        }
    }

    $findings = @()
    foreach ($header in Get-ChildItem -LiteralPath $portsDir -Filter *.h -File)
    {
        $relative = $header.FullName.Substring($root.Length).TrimStart('\', '/')
        $lines = [System.IO.File]::ReadAllLines($header.FullName)
        $iface = [System.IO.Path]::GetFileNameWithoutExtension($header.Name)
        for ($i = 0; $i -lt $lines.Length; $i++)
        {
            $line = $lines[$i]
            $classMatch = [regex]::Match($line, '^\s*(?:class|struct)\s+([A-Za-z_]\w*)\b(?!.*;)')
            if ($classMatch.Success)
            {
                $iface = $classMatch.Groups[1].Value
            }
            $methodMatch = [regex]::Match($line, '\b([A-Za-z_]\w*)\s*\([^()]*\)\s*(?:const\s*)?(?:noexcept\s*)?=\s*0\s*;')
            if (-not $methodMatch.Success)
            {
                continue
            }
            $method = $methodMatch.Groups[1].Value
            if ([regex]::IsMatch($callerCorpus, "[.>]\s*$method\s*\("))
            {
                continue
            }
            $findings += New-Finding -File $relative -Line ($i + 1) -Symbol "$iface::$method" `
                -Message "$iface::$method - pure port method with no caller in src/domain/ (dead at the seam)"
        }
    }

    return $findings
}

$plant = {
    param([string]$dir)

    $portsDir = Join-Path $dir 'src/domain/ports'
    New-Item -ItemType Directory -Path $portsDir -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $portsDir 'PlantPort.h') -Encoding UTF8 -Value @(
        'class PlantPort'
        '{'
        'public:'
        '    virtual ~PlantPort() = default;'
        '    [[nodiscard]] virtual bool NeverCalledByDomain() const = 0;'
        '};'
    )
    return 'PlantPort::NeverCalledByDomain'
}

$allowlist = @()

Invoke-GuardCheck -Name 'check-domain-port-uncalled' `
    -Description 'no pure domain-port method is left uncalled by the domain' `
    -Detect $detect -PlantFixture $plant -Root $Root -Allowlist $allowlist
