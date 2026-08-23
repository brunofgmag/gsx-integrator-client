param(
    [int]$Port = 0,
    [string[]]$Send = @(),
    [int]$Seconds = 0,
    [string]$Out = '',
    [switch]$NoSubscribe
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-CouatlPort
{
    param([int]$Override)

    if ($Override -gt 0)
    {
        return $Override
    }

    $ini = Join-Path $env:APPDATA 'Virtuali\CouatlAddons.ini'
    if (Test-Path -LiteralPath $ini)
    {
        foreach ($line in [System.IO.File]::ReadAllLines($ini))
        {
            $match = [regex]::Match($line, '^\s*remote_server_port\s*=\s*(\d+)\s*$')
            if ($match.Success)
            {
                $value = [int]$match.Groups[1].Value
                if ($value -gt 0 -and $value -lt 65536)
                {
                    return $value
                }
            }
        }
    }

    return 8744
}

function ConvertTo-CommandFrame
{
    param([string]$Spec)

    $trimmed = $Spec.Trim()
    if ($trimmed.StartsWith('{'))
    {
        $parsed = $trimmed | ConvertFrom-Json
        $frame = [ordered]@{ type = 'command'; verb = $parsed.verb }
        if ($parsed.PSObject.Properties.Name -contains 'args')
        {
            $frame['args'] = $parsed.args
        }

        return ($frame | ConvertTo-Json -Depth 20 -Compress)
    }

    return (@{ type = 'command'; verb = $trimmed } | ConvertTo-Json -Compress)
}

function Write-Frame
{
    param([string]$Direction, [string]$Text, [string]$Path)

    $now = [DateTime]::Now
    $arrow = if ($Direction -eq 'in')
    { '<-'
    } else
    { '->'
    }
    Write-Host ("{0} {1} {2}" -f $now.ToString('HH:mm:ss.fff'), $arrow, $Text)

    if ($Path)
    {
        $record = [ordered]@{
            at = $now.ToString('yyyy-MM-ddTHH:mm:ss.fff')
            dir = $Direction
            text = $Text
        }
        Add-Content -LiteralPath $Path -Value ($record | ConvertTo-Json -Compress) -Encoding UTF8
    }
}

$port = Resolve-CouatlPort -Override $Port
$uri = [Uri]"ws://127.0.0.1:$port"

if ($Out)
{
    $outDir = Split-Path -Parent $Out
    if ($outDir -and -not (Test-Path -LiteralPath $outDir))
    {
        New-Item -ItemType Directory -Path $outDir -Force | Out-Null
    }
}

Write-Host "==> Tapping the GSX Remote API at $uri"
if ($Out)
{
    Write-Host "==> Frames land in $Out"
}

$socket = [System.Net.WebSockets.ClientWebSocket]::new()
$cts = [System.Threading.CancellationTokenSource]::new()

try
{
    [void]$socket.ConnectAsync($uri, $cts.Token).GetAwaiter().GetResult()
    Write-Host "==> Connected. Ctrl+C stops the tap."

    if (-not $NoSubscribe)
    {
        $subscribe = @{ type = 'subscribe'; channels = @('state', 'prompts', 'toasts') } | ConvertTo-Json -Compress
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($subscribe)
        [void]$socket.SendAsync(
                [System.ArraySegment[byte]]::new($bytes),
                [System.Net.WebSockets.WebSocketMessageType]::Text,
                $true,
                $cts.Token).GetAwaiter().GetResult()
        Write-Frame -Direction 'out' -Text $subscribe -Path $Out
    }

    foreach ($spec in $Send)
    {
        $frame = ConvertTo-CommandFrame -Spec $spec
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($frame)
        [void]$socket.SendAsync(
                [System.ArraySegment[byte]]::new($bytes),
                [System.Net.WebSockets.WebSocketMessageType]::Text,
                $true,
                $cts.Token).GetAwaiter().GetResult()
        Write-Frame -Direction 'out' -Text $frame -Path $Out
    }

    $deadline = if ($Seconds -gt 0)
    { [DateTime]::Now.AddSeconds($Seconds)
    } else
    { [DateTime]::MaxValue
    }
    $buffer = [byte[]]::new(65536)
    $message = [System.Text.StringBuilder]::new()

    while ($socket.State -eq [System.Net.WebSockets.WebSocketState]::Open -and [DateTime]::Now -lt $deadline)
    {
        $receive = $socket.ReceiveAsync([System.ArraySegment[byte]]::new($buffer), $cts.Token)
        while (-not $receive.AsyncWaitHandle.WaitOne(200))
        {
            if ([DateTime]::Now -ge $deadline)
            {
                $cts.Cancel()
                break
            }
        }

        if ($cts.IsCancellationRequested)
        {
            break
        }

        $result = $receive.GetAwaiter().GetResult()
        if ($result.MessageType -eq [System.Net.WebSockets.WebSocketMessageType]::Close)
        {
            Write-Host "==> The server closed the socket: $( $result.CloseStatus ) $( $result.CloseStatusDescription )"
            break
        }

        [void]$message.Append([System.Text.Encoding]::UTF8.GetString($buffer, 0, $result.Count))
        if ($result.EndOfMessage)
        {
            Write-Frame -Direction 'in' -Text $message.ToString() -Path $Out
            [void]$message.Clear()
        }
    }
} finally
{
    if ($socket.State -eq [System.Net.WebSockets.WebSocketState]::Open)
    {
        try
        {
            [void]$socket.CloseAsync(
                    [System.Net.WebSockets.WebSocketCloseStatus]::NormalClosure,
                    'tap done',
                    [System.Threading.CancellationToken]::None).GetAwaiter().GetResult()
        } catch
        {
        }
    }
    $socket.Dispose()
    $cts.Dispose()
}
