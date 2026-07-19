#ifndef GSX_INTEGRATOR_CLIENT_SIMBRIEFCLIENT_H
#define GSX_INTEGRATOR_CLIENT_SIMBRIEFCLIENT_H

#include <string>
#include <QtCore/QObject>
#include <QtNetwork/QNetworkAccessManager>
#include "../../domain/model/FlightPlan.h"

struct AutomationStatus;
struct AutomationSettings;
class QNetworkReply;

class SimbriefClient final : public QObject
{
    Q_OBJECT

public:
    explicit SimbriefClient(AutomationStatus* status, const AutomationSettings* settings, QObject* parent = nullptr);
    ~SimbriefClient() override = default;

    void Poll();
    void Reset();
    [[nodiscard]] bool Reload();

private:
    bool FetchData();
    void SetStatus(FlightPlanStatus status);
    void OnHttpFinished();
    [[nodiscard]] bool HasHttpError() const;
    void ApplyFlightPlan(const FlightPlan& flightPlan);
    void ClearResponse();

    AutomationStatus* automationStatus_;
    const AutomationSettings* settings_;
    FlightPlanStatus status_ = FlightPlanStatus::Idle;
    QNetworkAccessManager network_;
    QNetworkReply* reply_ = nullptr;
    bool pending_ = false;
    int lastError_ = 0;
    std::string responseBody_;
};

#endif //GSX_INTEGRATOR_CLIENT_SIMBRIEFCLIENT_H
