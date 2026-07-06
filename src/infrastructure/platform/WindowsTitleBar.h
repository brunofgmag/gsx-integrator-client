#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DARKTITLEBAR_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DARKTITLEBAR_H

#include <dwmapi.h>
#include <QtGui/QWindow>

namespace WindowsTitleBar
{
    inline void Apply(const QWindow* window, const bool dark)
    {
        if (!window)
        {
            return;
        }

        const auto hwnd = reinterpret_cast<HWND>(window->winId());
        const BOOL value = dark ? TRUE : FALSE;
        (void)DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    }
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DARKTITLEBAR_H
