#ifndef GSX_INTEGRATOR_CLIENT_GSXMENUGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_GSXMENUGATEWAY_H

class GsxMenuGateway
{
public:
    virtual ~GsxMenuGateway() = default;

    [[nodiscard]] virtual bool CallJetway() = 0;
    [[nodiscard]] virtual bool CallStairs() = 0;
    [[nodiscard]] virtual bool RepositionAircraft() = 0;
    [[nodiscard]] virtual bool RequestSimbriefLoad() = 0;
    [[nodiscard]] virtual bool RequestBoarding() = 0;
    [[nodiscard]] virtual bool RequestDeboarding() = 0;
    [[nodiscard]] virtual bool RequestPushback() = 0;
    [[nodiscard]] virtual bool RequestRefueling() = 0;
    [[nodiscard]] virtual bool ConfirmGoodEngines() = 0;
    [[nodiscard]] virtual bool CompletePushback() = 0;
    [[nodiscard]] virtual bool CompleteRefuel() = 0;

    virtual void DisableGsxMenu() = 0;
};

#endif //GSX_INTEGRATOR_CLIENT_GSXMENUGATEWAY_H
