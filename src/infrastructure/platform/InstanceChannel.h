#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_INSTANCECHANNEL_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_INSTANCECHANNEL_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtNetwork/QLocalServer>

inline constexpr int kInstanceChannelTimeoutMs = 1000;

[[nodiscard]] QString InstanceChannelName();
[[nodiscard]] bool NotifyRunningInstance(const QString& name, bool show, const QString& activationToken = {},
                                         int timeoutMs = kInstanceChannelTimeoutMs);

class InstanceServer final : public QObject
{
    Q_OBJECT

public:
    explicit InstanceServer(QObject* parent = nullptr);

    [[nodiscard]] bool Listen(const QString& name);

signals:
    void ShowRequested(const QString& activationToken);

private:
    void OnNewConnection();

    QLocalServer server_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_INSTANCECHANNEL_H
