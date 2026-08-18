param([string]$Root)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'guard-check-lib.ps1')

$detect = {
    param([string]$root)

    $infraDir = Join-Path $root 'src/infrastructure'
    if (-not (Test-Path -LiteralPath $infraDir))
    {
        return @()
    }

    $gatewayHeaders = @(Get-ChildItem -LiteralPath $infraDir -Recurse -Filter *Gateway.h -File)
    $gatewayPaths = @{}
    foreach ($header in $gatewayHeaders)
    {
        $gatewayPaths[$header.FullName] = $true
    }

    $callerCorpus = ''
    $srcDir = Join-Path $root 'src'
    foreach ($file in Get-ChildItem -LiteralPath $srcDir -Recurse -Include *.h, *.cpp -File)
    {
        if ($gatewayPaths.ContainsKey($file.FullName))
        {
            continue
        }
        $callerCorpus += [System.IO.File]::ReadAllText($file.FullName) + "`n"
    }

    $findings = @()
    foreach ($header in $gatewayHeaders)
    {
        $relative = $header.FullName.Substring($root.Length).TrimStart('\', '/')
        $lines = [System.IO.File]::ReadAllLines($header.FullName)
        $iface = [System.IO.Path]::GetFileNameWithoutExtension($header.Name)
        for ($i = 0; $i -lt $lines.Length; $i++)
        {
            $classMatch = [regex]::Match($lines[$i], '^\s*(?:class|struct)\s+([A-Za-z_]\w*)\b(?!.*;)')
            if ($classMatch.Success)
            {
                $iface = $classMatch.Groups[1].Value
            }
            $methodMatch = [regex]::Match($lines[$i], '\b([A-Za-z_]\w*)\s*\([^()]*\)\s*(?:const\s*)?(?:noexcept\s*)?=\s*0\s*;')
            if (-not $methodMatch.Success)
            {
                continue
            }
            $method = $methodMatch.Groups[1].Value
            $escaped = [regex]::Escape($method)
            $callPattern = "(?:[.>]\s*|[=(,{}~;!&|+\-*/%?]\s*|\breturn\s+|\belse\s+|^\s*)$escaped\s*\("
            if ([regex]::IsMatch($callerCorpus, $callPattern, [System.Text.RegularExpressions.RegexOptions]::Multiline))
            {
                continue
            }
            $findings += New-Finding -File $relative -Line ($i + 1) -Symbol "$iface::$method" `
                -Message "$iface::$method - gateway method with no caller anywhere in src/ (dead surface)"
        }
    }

    return $findings
}

$plant = {
    param([string]$dir)

    $gatewayDir = Join-Path $dir 'src/infrastructure/plant'
    New-Item -ItemType Directory -Path $gatewayDir -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $gatewayDir 'PlantGateway.h') -Encoding UTF8 -Value @(
        'class PlantGateway'
        '{'
        'public:'
        '    virtual ~PlantGateway() = default;'
        '    [[nodiscard]] virtual int NeverCalledAnywhere() const = 0;'
        '};'
    )
    return 'PlantGateway::NeverCalledAnywhere'
}

$allowlist = @()

Invoke-GuardCheck -Name 'check-infra-gateway-uncalled' `
    -Description 'no infrastructure gateway method is left uncalled across src/' `
    -Detect $detect -PlantFixture $plant -Root $Root -Allowlist $allowlist
