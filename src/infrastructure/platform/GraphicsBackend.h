#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GRAPHICSBACKEND_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GRAPHICSBACKEND_H

#include <QtCore/QString>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>

namespace GraphicsBackend
{
    inline QSGRendererInterface::GraphicsApi ToApi(const QString& name)
    {
        if (name == QLatin1String("d3d12"))
        {
            return QSGRendererInterface::Direct3D12;
        }
        if (name == QLatin1String("opengl"))
        {
            return QSGRendererInterface::OpenGL;
        }
        if (name == QLatin1String("vulkan"))
        {
            return QSGRendererInterface::Vulkan;
        }
        if (name == QLatin1String("d3d11"))
        {
            return QSGRendererInterface::Direct3D11;
        }

        return QSGRendererInterface::Software;
    }

    inline QString ToName(const QSGRendererInterface::GraphicsApi api)
    {
        switch (api)
        {
        case QSGRendererInterface::Software:
            return QStringLiteral("software");
        case QSGRendererInterface::OpenGL:
            return QStringLiteral("opengl");
        case QSGRendererInterface::Vulkan:
            return QStringLiteral("vulkan");
        case QSGRendererInterface::Direct3D11:
            return QStringLiteral("d3d11");
        case QSGRendererInterface::Direct3D12:
            return QStringLiteral("d3d12");
        default:
            return {};
        }
    }

    inline bool OverriddenByEnvironment()
    {
        return qEnvironmentVariableIsSet("QSG_RHI_BACKEND")
            || qEnvironmentVariableIsSet("QT_QUICK_BACKEND");
    }

    inline void Apply(const QString& name)
    {
        if (OverriddenByEnvironment())
        {
            return;
        }

        QQuickWindow::setGraphicsApi(ToApi(name));
    }
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_GRAPHICSBACKEND_H
