#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_WINDOWFOREGROUND_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_WINDOWFOREGROUND_H

#include <windows.h>
#include <QtGui/QWindow>

namespace WindowForeground
{
    inline void Bring(QWindow* window)
    {
        if (!window)
        {
            return;
        }

        window->showNormal();
        window->raise();
        window->requestActivate();

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
    }
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_WINDOWFOREGROUND_H
