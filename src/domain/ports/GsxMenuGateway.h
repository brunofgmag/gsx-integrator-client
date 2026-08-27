#ifndef GSX_INTEGRATOR_CLIENT_GSXMENUGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_GSXMENUGATEWAY_H

class GsxMenuGateway
{
public:
    virtual ~GsxMenuGateway() = default;

    virtual void CallJetway() = 0;
    virtual void CallStairs() = 0;
    virtual void RepositionAircraft() = 0;
    virtual void RequestSimbriefLoad() = 0;
    virtual void RequestBoarding() = 0;
    virtual void RequestDeboarding() = 0;
    virtual void RequestPushback() = 0;
    virtual void OpenPushbackPanel() = 0;
    virtual void RequestRefueling() = 0;
    virtual void CompleteRefuel() = 0;
    virtual void CompleteBoarding() = 0;
    virtual void ToggleGpu() = 0;
    virtual void RequestCatering() = 0;
    virtual void RequestLavatory() = 0;
    virtual void RequestWater() = 0;
    virtual void RequestCleaning() = 0;

    [[nodiscard]] virtual bool ConfirmGoodEngines() = 0;
    [[nodiscard]] virtual bool CompletePushback() = 0;

    virtual void DisableGsxMenu() = 0;

    virtual void OnTurnaroundTurned() = 0;
    virtual void OnPushbackStarted() = 0;
};

#endif //GSX_INTEGRATOR_CLIENT_GSXMENUGATEWAY_H
