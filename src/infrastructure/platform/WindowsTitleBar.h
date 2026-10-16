#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DARKTITLEBAR_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DARKTITLEBAR_H

#ifdef _WIN32
#include <dwmapi.h>
#endif
#include <QtGui/QWindow>

namespace WindowsTitleBar
{
    inline void Apply(const QWindow* window, const bool dark)
    {
#ifdef _WIN32
        if (!window)
        {
            return;
        }

        const auto hwnd = reinterpret_cast<HWND>(window->winId());
        const BOOL value = dark ? TRUE : FALSE;
        (void)DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
#else
        (void)window;
        (void)dark;
#endif
    }
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DARKTITLEBAR_H
