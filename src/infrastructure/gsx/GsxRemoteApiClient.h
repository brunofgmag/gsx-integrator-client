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

    void SetHandshakeTimeoutForTest(const int ms) { handshakeTimeoutMs_ = ms; }

signals:
    void SnapshotReceived(const QJsonObject& snapshot);
    void PatchReceived(const QString& path, const QJsonValue& value);
    void ResultReceived(bool ok, const QString& errorCode);

private slots:
    void OnConnected();
    void OnDisconnected();
    void OnTextMessage(const QString& text);
    void OnReconnect();
    void OnHandshakeTimeout();

private:
    static quint16 ResolvePort();
    void SendSubscribe() const;
    void ScheduleReconnect();
    void HandleResult(const QJsonObject& msg);

    static constexpr int kHandshakeTimeoutMs = 5000;

    QWebSocket* socket_ = nullptr;
    QTimer* reconnectTimer_ = nullptr;
    QTimer* handshakeTimer_ = nullptr;
    quint16 port_ = 8744;
    bool connected_ = false;
    bool handshakeDone_ = false;
    bool stopping_ = false;
    int backoffMs_ = 1000;
    int handshakeTimeoutMs_ = kHandshakeTimeoutMs;
};

#endif //GSX_INTEGRATOR_CLIENT_GSXREMOTEAPICLIENT_H
