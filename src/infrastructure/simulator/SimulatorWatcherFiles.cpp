#include "SimulatorWatcherFiles.h"

namespace
{
    constexpr auto kScriptTemplate = R"script(#!/usr/bin/env bash
set -u
shopt -s nocasematch

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
client="${GSXI_CLIENT:-$here/@CLIENT@}"
proc_root="${GSXI_PROC_ROOT:-/proc}"
poll="${GSXI_POLL_SECONDS:-2}"
read -r -a simulators <<<"${GSXI_SIMULATORS:-}"
client_names=(gsx-integrator-client @CLIENT@)

if [[ ${#simulators[@]} -eq 0 ]]; then
    echo "GSXI_SIMULATORS is empty" >&2
    exit 1
fi

running() {
    local cmdline pid arg base name
    local -a args
    for cmdline in "$proc_root"/[0-9]*/cmdline; do
        pid="${cmdline%/cmdline}"
        pid="${pid##*/}"
        [[ $pid == "$$" ]] && continue
        args=()
        mapfile -d '' -t args 2>/dev/null <"$cmdline"
        for arg in "${args[@]}"; do
            arg="${arg//\"/}"
            base="${arg##*[/\\]}"
            for name in "$@"; do
                [[ $base == "$name" ]] && return 0
            done
        done
    done
    return 1
}

program_running() {
    local cmdline pid arg base name
    for cmdline in "$proc_root"/[0-9]*/cmdline; do
        pid="${cmdline%/cmdline}"
        pid="${pid##*/}"
        [[ $pid == "$$" ]] && continue
        arg=""
        IFS= read -r -d '' arg 2>/dev/null <"$cmdline"
        arg="${arg//\"/}"
        base="${arg##*[/\\]}"
        for name in "$@"; do
            [[ $base == "$name" ]] && return 0
        done
    done
    return 1
}

client_running() {
    program_running "${client_names[@]}" && return 0
    [[ -n $(find "$proc_root" -maxdepth 2 -path "$proc_root/[0-9]*/exe" -ilname '*/gsx-integrator-client' -print -quit 2>/dev/null) ]]
}

echo "Watching for ${simulators[*]}"
client_pid=""
active=0
while true; do
    if running "${simulators[@]}"; then
        if (( ! active )); then
            active=1
            if [[ ! -x $client ]]; then
                echo "Simulator started, but the client at $client is not executable"
            elif client_running; then
                echo "Simulator started and the client is already running"
            else
                setsid "$client" --tray >/dev/null 2>&1 &
                client_pid=$!
                echo "Simulator started, started the client as pid $client_pid"
            fi
        fi
    elif (( active )); then
        active=0
        if [[ -n $client_pid ]]; then
            kill -- "-$client_pid" 2>/dev/null || kill "$client_pid" 2>/dev/null
            echo "Simulator closed, stopped the client at pid $client_pid"
            client_pid=""
        else
            echo "Simulator closed"
        fi
    fi
    sleep "$poll"
done
)script";

    constexpr auto kUnitTemplate = R"unit([Unit]
Description=Start GSX Integrator with the simulator

[Service]
Type=simple
Environment="GSXI_SIMULATORS=@SIMULATORS@"
ExecStart="@SCRIPT@"
Restart=on-failure
RestartSec=5

[Install]
WantedBy=default.target
)unit";

    QString EscapeUnitValue(const QString& value)
    {
        QString escaped = value;
        escaped.replace(u'%', QLatin1String("%%"));
        escaped.replace(u'$', QLatin1String("$$"));
        escaped.replace(u'\\', QLatin1String("\\\\"));
        escaped.replace(u'"', QLatin1String("\\\""));

        return escaped;
    }
}

SimulatorWatcherFiles WatcherFilesFor(const QString& scriptPath, const QStringList& processNames,
                                      const QString& clientOverride)
{
    SimulatorWatcherFiles files;
    files.script = QString::fromLatin1(kScriptTemplate).replace(QLatin1String("@CLIENT@"),
                                                                 QLatin1String(kClientAppImageName));
    files.unit = QString::fromLatin1(kUnitTemplate)
                     .replace(QLatin1String("@SIMULATORS@"), EscapeUnitValue(processNames.join(u' ')))
                     .replace(QLatin1String("@SCRIPT@"), EscapeUnitValue(scriptPath));
    if (!clientOverride.isEmpty())
    {
        files.unit.replace(QLatin1String("ExecStart="),
                           QStringLiteral("Environment=\"GSXI_CLIENT=%1\"\nExecStart=").arg(EscapeUnitValue(clientOverride)));
    }

    return files;
}
