#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SHOWWINDOWMESSAGEFILTER_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SHOWWINDOWMESSAGEFILTER_H

#include <windows.h>
#include <QtCore/QAbstractNativeEventFilter>
#include <QtGui/QWindow>
#include "WindowForeground.h"

class ShowWindowMessageFilter final : public QAbstractNativeEventFilter
{
public:
    static UINT MessageId()
    {
        static const UINT id = RegisterWindowMessageW(L"gsx-integrator-client.show");
        return id;
    }

    explicit ShowWindowMessageFilter(QWindow* window)
        : window_(window)
    {
    }

    bool nativeEventFilter(const QByteArray&, void* message, qintptr*) override
    {
        if (const auto* msg = static_cast<MSG*>(message);
            msg->message == MessageId() && window_ != nullptr)
        {
            WindowForeground::Bring(window_);
        }
        return false;
    }

private:
    QWindow* window_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SHOWWINDOWMESSAGEFILTER_H
