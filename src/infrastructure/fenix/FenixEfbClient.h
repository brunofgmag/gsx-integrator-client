#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXEFBCLIENT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXEFBCLIENT_H

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <QtCore/QByteArray>
#include <QtCore/QJsonValue>
#include "FenixEfbGateway.h"

class QNetworkAccessManager;
class QNetworkReply;

class FenixEfbClient final : public FenixEfbGateway
{
public:
    FenixEfbClient();
    ~FenixEfbClient() override;

    void Subscribe(const std::string& name) override;
    void Poll() override;
    [[nodiscard]] bool IsAvailable() const override;
    [[nodiscard]] double GetNumber(const std::string& name, double defaultValue) const override;
    [[nodiscard]] std::string GetString(const std::string& name, const std::string& defaultValue) const override;
    [[nodiscard]] std::vector<bool> GetBoolArray(const std::string& name) const override;
    void SetFloat(const std::string& name, double value) override;
    void SetBool(const std::string& name, bool value) override;
    void SetString(const std::string& name, const std::string& value) override;
    void RequestLoadsheet(const std::string& type) override;

    [[nodiscard]] static QByteArray BuildValuesQuery(const std::vector<std::string>& names);
    [[nodiscard]] static std::optional<std::map<std::string, QJsonValue>> ParseValuesResponse(
        const QByteArray& body, const std::vector<std::string>& names);
    [[nodiscard]] static QByteArray BuildWriteMutation(const std::string& operation,
                                                       const std::string& typeName,
                                                       const std::string& name,
                                                       const QJsonValue& value);

private:
    QNetworkAccessManager& EnsureNetwork();
    void SendMutation(const std::string& operation, const std::string& typeName,
                      const std::string& name, const QJsonValue& value);
    void OnPollFinished();
    void RegisterPollResult(bool succeeded);

    std::unique_ptr<QNetworkAccessManager> network_;
    QNetworkReply* pollReply_ = nullptr;
    std::vector<std::string> subscriptions_;
    std::map<std::string, QJsonValue> values_;
    bool available_ = false;
    int consecutivePollFailures_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXEFBCLIENT_H
