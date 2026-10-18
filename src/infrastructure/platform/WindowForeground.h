#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_WINDOWFOREGROUND_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_WINDOWFOREGROUND_H

#ifdef _WIN32
#include <windows.h>
#endif
#include <QtCore/QString>
#include <QtGui/QWindow>

namespace WindowForeground
{
    inline constexpr auto kActivationTokenVariable = "XDG_ACTIVATION_TOKEN";

    inline void Bring(QWindow* window, const QString& activationToken = {})
    {
        if (!window)
        {
            return;
        }

        if (!activationToken.isEmpty())
        {
            qputenv(kActivationTokenVariable, activationToken.toUtf8());
        }

        window->showNormal();
        window->raise();
        window->requestActivate();

#ifdef _WIN32
        const auto hwnd = reinterpret_cast<HWND>(window->winId());
        if (!hwnd)
        {
            return;
        }

        if (IsIconic(hwnd))
        {
            ShowWindow(hwnd, SW_RESTORE);
        }

        HWND foreground = GetForegroundWindow();
        const DWORD foregroundThread = GetWindowThreadProcessId(foreground, nullptr);
        const DWORD thisThread = GetCurrentThreadId();
        const bool attach = foregroundThread != 0 && foregroundThread != thisThread;

        if (attach)
        {
            AttachThreadInput(thisThread, foregroundThread, TRUE);
        }

        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);

        if (attach)
        {
            AttachThreadInput(thisThread, foregroundThread, FALSE);
        }
#endif
    }
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_WINDOWFOREGROUND_H
