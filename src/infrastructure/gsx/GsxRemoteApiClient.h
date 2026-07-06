#ifndef GSX_INTEGRATOR_CLIENT_GSXREMOTEAPICLIENT_H
#define GSX_INTEGRATOR_CLIENT_GSXREMOTEAPICLIENT_H

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QString>

class QWebSocket;
class QTimer;

class GsxRemoteApiClient : public QObject
{
    Q_OBJECT
public:
    explicit GsxRemoteApiClient(QObject* parent = nullptr);
    ~GsxRemoteApiClient() override;

    void Start();
    void Stop();

    virtual bool SendCommand(const QString& verb, const QJsonObject& args = {});

signals:
    void SnapshotReceived(const QJsonObject& snapshot);
    void PatchReceived(const QString& path, const QJsonValue& value);
    void ResultReceived(bool ok, const QString& errorCode);

private slots:
    void OnConnected();
    void OnDisconnected();
    void OnTextMessage(const QString& text);
    void OnReconnect();

private:
    static quint16 ResolvePort();
    void SendSubscribe();
    void ScheduleReconnect();

    QWebSocket* socket_ = nullptr;
    QTimer* reconnectTimer_ = nullptr;
    quint16 port_ = 8744;
    bool connected_ = false;
    bool stopping_ = false;
    int backoffMs_ = 1000;
};

#endif //GSX_INTEGRATOR_CLIENT_GSXREMOTEAPICLIENT_H
