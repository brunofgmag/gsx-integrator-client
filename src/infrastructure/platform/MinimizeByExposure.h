#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_MINIMIZEBYEXPOSURE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_MINIMIZEBYEXPOSURE_H

#include <chrono>

#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtGui/QWindow>

inline constexpr std::chrono::milliseconds kMinimizeFocusGrace{250};

class MinimizeByExposure final : public QObject
{
    Q_OBJECT

public:
    explicit MinimizeByExposure(QWindow* window, std::chrono::milliseconds focusGrace = kMinimizeFocusGrace);

    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void Minimized();

private:
    [[nodiscard]] bool HadFocusJustNow() const;

    QWindow* window_;
    std::chrono::milliseconds focusGrace_;
    QElapsedTimer sinceFocusLost_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_MINIMIZEBYEXPOSURE_H
