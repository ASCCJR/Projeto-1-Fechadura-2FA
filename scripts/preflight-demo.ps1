param(
    [int]$BrokerPort = 1884,
    [string]$DeviceId = "bitdoglab_02",
    [switch]$OpenBrowser
)

$ErrorActionPreference = "Stop"

function Resolve-ToolPath {
    param(
        [string[]]$Candidates,
        [string]$CommandName
    )

    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path $candidate)) {
            return $candidate
        }
    }

    if ($CommandName) {
        $cmd = Get-Command $CommandName -ErrorAction SilentlyContinue
        if ($cmd) {
            return $cmd.Source
        }
    }

    return $null
}

function Set-DefineMacro {
    param(
        [string]$Content,
        [string]$Name,
        [string]$Value,
        [bool]$Quoted
    )

    $replacement = if ($Quoted) { "#define $Name `\"$Value`\"" } else { "#define $Name $Value" }
    $regex = "(?m)^\s*#define\s+" + [regex]::Escape($Name) + "\s+.+$"

    if ([regex]::IsMatch($Content, $regex)) {
        return [regex]::Replace($Content, $regex, $replacement, 1)
    }

    $endifRegex = "(?m)^\s*#endif\b.*$"
    if ([regex]::IsMatch($Content, $endifRegex)) {
        return [regex]::Replace($Content, $endifRegex, "$replacement`r`n`$0", 1)
    }

    return ($Content.TrimEnd() + "`r`n" + $replacement + "`r`n")
}

function Get-PreferredIPv4 {
    $wifi = Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue |
        Where-Object {
            $_.InterfaceAlias -like "*Wi-Fi*" -and
            $_.IPAddress -notlike "169.254*" -and
            $_.IPAddress -ne "127.0.0.1"
        } |
        Select-Object -First 1

    if ($wifi) {
        return $wifi.IPAddress
    }

    $fallback = Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue |
        Where-Object {
            $_.IPAddress -notlike "169.254*" -and
            $_.IPAddress -ne "127.0.0.1"
        } |
        Select-Object -First 1

    if ($fallback) {
        return $fallback.IPAddress
    }

    throw "Nao foi possivel detectar um IPv4 valido para o host."
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$configLocal = Join-Path $repoRoot "configura_local.h"
$configExample = Join-Path $repoRoot "configura_local.example.h"
$secretsLocal = Join-Path $repoRoot "secrets.local.h"
$secretsExample = Join-Path $repoRoot "secrets.local.example.h"
$mosqConf = Join-Path $repoRoot "mosquitto.local.conf"

$results = New-Object System.Collections.Generic.List[object]

if (-not (Test-Path $secretsLocal) -and (Test-Path $secretsExample)) {
    Copy-Item $secretsExample $secretsLocal -Force
}
$results.Add([pscustomobject]@{ Step = "secrets.local.h"; Status = if (Test-Path $secretsLocal) { "OK" } else { "FAIL" }; Detail = $secretsLocal })

if (-not (Test-Path $configLocal)) {
    if (Test-Path $configExample) {
        Copy-Item $configExample $configLocal -Force
    } else {
        @(
            "#ifndef CONFIGURA_LOCAL_H",
            "#define CONFIGURA_LOCAL_H",
            "",
            "#define DEVICE_ID \"bitdoglab_02\"",
            "#define MQTT_BROKER_IP \"127.0.0.1\"",
            "#define MQTT_BROKER_PORT 1884",
            "",
            "#endif // CONFIGURA_LOCAL_H"
        ) | Set-Content -Path $configLocal -Encoding utf8
    }
}

$hostIp = Get-PreferredIPv4
$configText = Get-Content $configLocal -Raw
$configText = Set-DefineMacro -Content $configText -Name "DEVICE_ID" -Value $DeviceId -Quoted $true
$configText = Set-DefineMacro -Content $configText -Name "MQTT_BROKER_IP" -Value $hostIp -Quoted $true
$configText = Set-DefineMacro -Content $configText -Name "MQTT_BROKER_PORT" -Value $BrokerPort -Quoted $false
Set-Content -Path $configLocal -Value $configText -Encoding utf8
$results.Add([pscustomobject]@{ Step = "configura_local.h"; Status = "OK"; Detail = "$hostIp:$BrokerPort" })

if (-not (Test-Path $mosqConf)) {
    @(
        "# Broker local sem permissao de administrador",
        "listener $BrokerPort 0.0.0.0",
        "allow_anonymous true",
        "persistence false",
        "log_dest stdout"
    ) | Set-Content -Path $mosqConf -Encoding utf8
} else {
    $mosqText = Get-Content $mosqConf -Raw
    if ($mosqText -match "(?m)^\s*listener\s+") {
        $mosqText = [regex]::Replace($mosqText, "(?m)^\s*listener\s+.*$", "listener $BrokerPort 0.0.0.0", 1)
    } else {
        $mosqText = "listener $BrokerPort 0.0.0.0`r`n" + $mosqText
    }
    if ($mosqText -notmatch "(?m)^\s*allow_anonymous\s+") {
        $mosqText += "`r`nallow_anonymous true"
    }
    if ($mosqText -notmatch "(?m)^\s*persistence\s+") {
        $mosqText += "`r`npersistence false"
    }
    if ($mosqText -notmatch "(?m)^\s*log_dest\s+") {
        $mosqText += "`r`nlog_dest stdout"
    }
    Set-Content -Path $mosqConf -Value $mosqText -Encoding utf8
}
$results.Add([pscustomobject]@{ Step = "mosquitto.local.conf"; Status = "OK"; Detail = $mosqConf })

$mosqExe = Resolve-ToolPath -Candidates @("C:\Program Files\Mosquitto\mosquitto.exe") -CommandName "mosquitto"
$pubExe = Resolve-ToolPath -Candidates @("C:\Program Files\Mosquitto\mosquitto_pub.exe") -CommandName "mosquitto_pub"
$subExe = Resolve-ToolPath -Candidates @("C:\Program Files\Mosquitto\mosquitto_sub.exe") -CommandName "mosquitto_sub"

$listener = Get-NetTCPConnection -State Listen -LocalPort $BrokerPort -ErrorAction SilentlyContinue
if (-not $listener) {
    if (-not $mosqExe) {
        $results.Add([pscustomobject]@{ Step = "broker_1884"; Status = "FAIL"; Detail = "mosquitto.exe nao encontrado" })
    } else {
        Start-Process -FilePath $mosqExe -ArgumentList @("-c", $mosqConf, "-v") -WindowStyle Minimized
        for ($i = 0; $i -lt 10; $i++) {
            Start-Sleep -Milliseconds 400
            $listener = Get-NetTCPConnection -State Listen -LocalPort $BrokerPort -ErrorAction SilentlyContinue
            if ($listener) { break }
        }
    }
}

if ($listener) {
    $results.Add([pscustomobject]@{ Step = "broker_1884"; Status = "OK"; Detail = "listener ativo" })
} elseif (-not ($results | Where-Object { $_.Step -eq "broker_1884" })) {
    $results.Add([pscustomobject]@{ Step = "broker_1884"; Status = "FAIL"; Detail = "listener nao ativo" })
}

$mqttOk = $false
if ($pubExe -and $subExe) {
    $topic = "copilot/preflight/" + (Get-Random)
    $job = Start-Job -ScriptBlock {
        param($subPath, $port, $subTopic)
        & $subPath -h 127.0.0.1 -p $port -t $subTopic -C 1 -W 5 -v
    } -ArgumentList $subExe, $BrokerPort, $topic

    Start-Sleep -Milliseconds 300
    & $pubExe -h 127.0.0.1 -p $BrokerPort -t $topic -m ok | Out-Null

    Wait-Job $job -Timeout 8 | Out-Null
    $out = (Receive-Job $job -ErrorAction SilentlyContinue) -join "`n"
    Remove-Job $job -Force
    $mqttOk = $out -match ($topic + " ok")
}

$results.Add([pscustomobject]@{ Step = "mqtt_loopback"; Status = if ($mqttOk) { "OK" } else { "FAIL" }; Detail = "127.0.0.1:$BrokerPort" })

$nodeRedPort = 1880
$nodeListener = Get-NetTCPConnection -State Listen -LocalPort $nodeRedPort -ErrorAction SilentlyContinue
if (-not $nodeListener) {
    $nodeRedCmd = Join-Path $env:APPDATA "npm\node-red.cmd"
    if (Test-Path $nodeRedCmd) {
        Start-Process -FilePath $nodeRedCmd -WindowStyle Minimized
    } else {
        $nr = Get-Command node-red -ErrorAction SilentlyContinue
        if ($nr) {
            Start-Process -FilePath $nr.Source -WindowStyle Minimized
        }
    }

    for ($i = 0; $i -lt 12; $i++) {
        Start-Sleep -Milliseconds 500
        $nodeListener = Get-NetTCPConnection -State Listen -LocalPort $nodeRedPort -ErrorAction SilentlyContinue
        if ($nodeListener) { break }
    }
}

$results.Add([pscustomobject]@{ Step = "node_red_1880"; Status = if ($nodeListener) { "OK" } else { "FAIL" }; Detail = "http://127.0.0.1:1880/" })

if ($OpenBrowser) {
    Start-Process "http://127.0.0.1:1880/"
    Start-Process "http://127.0.0.1:1880/ui"
}

""
"Preflight summary"
$results | Format-Table -AutoSize

$failed = $results | Where-Object { $_.Status -ne "OK" }
if ($failed) {
    exit 1
}

exit 0
