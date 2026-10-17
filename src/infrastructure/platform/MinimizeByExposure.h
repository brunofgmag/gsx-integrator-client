#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_MINIMIZEBYEXPOSURE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_MINIMIZEBYEXPOSURE_H

#include <QtCore/QObject>
#include <QtGui/QWindow>

class MinimizeByExposure final : public QObject
{
    Q_OBJECT

public:
    explicit MinimizeByExposure(QWindow* window);

    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void Minimized();

private:
    QWindow* window_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_MINIMIZEBYEXPOSURE_H
