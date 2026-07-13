#include "IntegratorRuntime.h"

#include "sim/SessionReadiness.h"
#include "../infrastructure/aircraft/AircraftFactory.h"
#include "../infrastructure/logging/LogMacros.h"
#include "../infrastructure/gsx/GsxAircraftProfile.h"
#include "../infrastructure/gsx/GsxRemoteStateReducer.h"

IntegratorRuntime::IntegratorRuntime(QObject* parent)
    : QObject(parent),
      pluginClient_(&varGateway_),
      gsxService_(&varGateway_),
      gsxMenu_(&gsxRemoteClient_, &gsxRemoteState_, &settings_, &qtLogger_, &pluginClient_),
      stateMachine_(&status_, &settings_, &gsxService_, &gsxMenu_, &qtLogger_),
      simbriefClient_(&status_, &settings_, this)
{
    dispatchTimer_.setInterval(kDispatchIntervalMs);
    connect(&dispatchTimer_, &QTimer::timeout, this, &IntegratorRuntime::OnDispatchTimer);

    reconnectTimer_.setInterval(kReconnectIntervalMs);
    connect(&reconnectTimer_, &QTimer::timeout, this, &IntegratorRuntime::TryConnect);
}

IntegratorRuntime::~IntegratorRuntime()
{
    this->Shutdown();
}

void IntegratorRuntime::Setup()
{
    LOG_INFO("Setting up GSX Integrator...");

    TryConnect();

    connect(&gsxRemoteClient_, &GsxRemoteApiClient::SnapshotReceived,
            &gsxRemoteClient_, [this](const QJsonObject& s)
            {
                GsxRemoteStateReducer::ApplySnapshot(gsxRemoteState_, s);
                gsxMenu_.OnSnapshot();
            });

    connect(&gsxRemoteClient_, &GsxRemoteApiClient::PatchReceived,
            &gsxRemoteClient_, [this](const QString& p, const QJsonValue& v)
            {
                const std::string path = p.toStdString();
                GsxRemoteStateReducer::ApplyPatch(gsxRemoteState_, path, v);
                if (path == "/menu" || path == "/menuShown")
                {
                    gsxMenu_.OnMenuChanged();
                }
            });

    gsxRemoteClient_.Start();

    dispatchTimer_.start();
}

bool IntegratorRuntime::IsSimOnMenu()
{
    return varGateway_.GetAVar("CAMERA STATE", "Number") == 12.0;
}

void IntegratorRuntime::OnSimOpen(const char* appName)
{
    simVersion_ = SimVersionDetect::FromAppName(appName ? appName : "");

    const char* label =
        simVersion_ == SimVersion::Msfs2024
            ? "MSFS 2024"
            : simVersion_ == SimVersion::Msfs2020
            ? "MSFS 2020"
            : "Unknown";

    LOG_INFO("Connected simulator: '%s' (%s)", appName ? appName : "?", label);
}

void IntegratorRuntime::TryConnect()
{
    if (simConnect_.IsConnected())
    {
        return;
    }

    LOG_INFO("Opening SimConnect...");
    if (!simConnect_.Open("GSX Integrator"))
    {
        reconnectTimer_.start();

        emit Updated();

        return;
    }

    reconnectTimer_.stop();

    simConnect_.SetOnOpen([this](const char* appName) { OnSimOpen(appName); });

    simConnect_.SetVarManager(&varGateway_);
    varGateway_.Attach(simConnect_.Handle());
    simConnect_.SetOnQuit([this] { HandleDisconnected(); });

    if (!simConnect_.SubscribeOneSecond([this] { Update(); }))
    {
        LOG_ERROR("Failed to subscribe to 1sec SimConnect event.");


        return;
    }

    if (!simConnect_.SubscribeFourSeconds([this] { UpdateSlow(); }))
    {
        LOG_ERROR("Failed to subscribe to 4sec SimConnect event.");

        HandleDisconnected();

        return;
    }

    if (!simConnect_.SubscribeSimRunning([this](const bool running) { OnSimRunningChanged(running); }))
    {
        LOG_ERROR("Failed to subscribe to 'Sim' SimConnect system event.");

        HandleDisconnected();

        return;
    }

    if (!simConnect_.SubscribeToPause([this](const unsigned flag) { OnPauseChanged(flag); }))
    {
        LOG_ERROR("Failed to subscribe to 'Pause_EX1' SimConnect event.");

        HandleDisconnected();

        return;
    }

    pluginClient_.Setup();

    varGateway_.SetFastRefresh("L:FSDT_GSX_JETWAY");
    varGateway_.SetFastRefresh("L:FSDT_GSX_STAIRS");

    LOG_INFO("GSX Integrator connected to the simulator.");

    emit Updated();
}

void IntegratorRuntime::HandleDisconnected()
{
    LOG_INFO("Lost connection to the simulator.");

    simVersion_ = SimVersion::Unknown;

    varGateway_.Detach();
    simConnect_.Close();

    OnSessionEnd();

    emit Updated();

    emit SimulatorQuit();
}

void IntegratorRuntime::OnDispatchTimer()
{
    if (!simConnect_.IsConnected())
    {
        return;
    }

    if (!simConnect_.Dispatch())
    {
        HandleDisconnected();
    }
}

void IntegratorRuntime::OnSimRunningChanged(const bool running)
{
    if (running && !isSessionActive_)
    {
        OnFlightStart();
    }
    else if (!running && isSessionActive_)
    {
        OnSessionEnd();
    }
}

void IntegratorRuntime::OnPauseChanged(const unsigned flag)
{
    pauseFlags_ = flag;
}

void IntegratorRuntime::Update()
{
    if (IsSimOnMenu() && isSessionActive_)
    {
        OnSessionEnd();

        return;
    }

    const auto emitOnExit = qScopeGuard([this] { emit Updated(); });

    if (!IsSessionReady() || IsSessionPaused())
    {
        return;
    }

    simbriefClient_.Poll();

    const bool gsxOk = gsxService_.IsAvailable();
    status_.gsxAvailable = gsxOk;

    if (!status_.enabled || !gsxOk)
    {
        return;
    }

    ResolveAircraft();

    if (!aircraft_)
    {
        return;
    }

    stateMachine_.AttachAircraft(aircraft_.get());
    gsxMenu_.OnMenuChanged();
    stateMachine_.Tick();
    aircraft_->OnTick();
}

void IntegratorRuntime::UpdateSlow()
{
    if (!IsSessionActive() || !aircraft_ || !status_.enabled || !IsSessionReady() || IsSessionPaused())
    {
        return;
    }

    aircraft_->OnSlowTick();

    CheckGsxProfile();
}

void IntegratorRuntime::Shutdown()
{
    LOG_INFO("Shutting down GSX Integrator...");

    dispatchTimer_.stop();
    reconnectTimer_.stop();

    gsxRemoteClient_.Stop();

    if (simConnect_.IsConnected())
    {
        pluginClient_.Shutdown();
    }

    varGateway_.Detach();
    simConnect_.Close();
    gsxMenu_.Reset();

    aircraft_.reset();
}

void IntegratorRuntime::ResetSession()
{
    status_.Reset();
    gsxService_.Reset();
    gsxMenu_.Reset();
    stateMachine_.Reset();
    simbriefClient_.Reset();
}

void IntegratorRuntime::OnFlightStart()
{
    LOG_INFO("Flight started. Initializing session...");

    isSessionActive_ = true;

    aircraft_.reset();
    gsxProfileRoots_.clear();
    gsxProfileCfgs_.clear();
    gsxProfileConflict_ = false;
    gsxProfileFlagsMissing_ = false;

    ResetSession();

    MaybeAutoStart();

    emit Updated();
}

void IntegratorRuntime::OnSessionEnd()
{
    LOG_INFO("Session ended. Cleaning up...");

    isSessionActive_ = false;

    aircraft_.reset();
    gsxProfileRoots_.clear();
    gsxProfileCfgs_.clear();
    gsxProfileConflict_ = false;
    gsxProfileFlagsMissing_ = false;

    ResetSession();

    emit Updated();
}

void IntegratorRuntime::ResolveAircraft()
{
    if (aircraft_)
    {
        return;
    }

    aircraft_ = DetectAircraft(&varGateway_, &status_);
    if (aircraft_)
    {
        status_.aircraftSupported = true;
        gsxProfileRoots_ = GsxAircraftProfile::ProfileRootsFor(aircraft_->GetName());
        gsxProfileFlagsMissing_ = GsxAircraftProfile::FlagsMissingProfile(aircraft_->GetName());
        CheckGsxProfile();
    }
}

void IntegratorRuntime::CheckGsxProfile()
{
    if (gsxProfileRoots_.empty())
    {
        gsxProfileCfgs_.clear();
        gsxProfileConflict_ = false;
        return;
    }

    gsxProfileCfgs_ = GsxAircraftProfile::FindCfgs(gsxProfileRoots_);

    bool conflict = gsxProfileCfgs_.empty() && gsxProfileFlagsMissing_;
    for (const auto& cfg : gsxProfileCfgs_)
    {
        const std::optional<int> refueling = GsxAircraftProfile::ReadRefueling(cfg);
        if (!refueling.has_value() || *refueling != 0)
        {
            conflict = true;
            if (!gsxProfileConflict_)
            {
                LOG_WARN("GSX profile '%s' does not set 'refueling = 0'; the fuel truck will not connect.",
                         cfg.string().c_str());
            }
        }
    }

    gsxProfileConflict_ = conflict;
}

bool IntegratorRuntime::CanFixGsxProfile() const
{
    return gsxProfileConflict_ && !gsxProfileCfgs_.empty();
}

bool IntegratorRuntime::FixGsxProfile()
{
    if (gsxProfileCfgs_.empty())
    {
        return false;
    }

    for (const auto& cfg : gsxProfileCfgs_)
    {
        const std::optional<int> refueling = GsxAircraftProfile::ReadRefueling(cfg);
        if (refueling.has_value() && *refueling == 0)
        {
            continue;
        }

        if (!GsxAircraftProfile::WriteRefueling(cfg, 0))
        {
            return false;
        }

        LOG_INFO("GSX profile '%s' updated: refueling = 0.", cfg.string().c_str());
    }

    CheckGsxProfile();

    emit Updated();

    return true;
}

bool IntegratorRuntime::IsSessionReady()
{
    const double camera = varGateway_.GetAVar("CAMERA STATE", "Number");

    if (simVersion_ != SimVersion::Msfs2024)
    {
        return SessionReadiness::Evaluate(simVersion_, camera, 0.0, 0.0);
    }

    return SessionReadiness::Evaluate(
        simVersion_,
        camera,
        varGateway_.GetAVar("IS AIRCRAFT", "Number"),
        varGateway_.GetAVar("IS AVATAR", "Number")
    );
}

QString IntegratorRuntime::GetAircraftName() const
{
    return aircraft_ ? QString::fromUtf8(aircraft_->GetName()) : QString();
}

bool IntegratorRuntime::IsAircraftRefuelByGsx() const
{
    return aircraft_ && aircraft_->RefuelMethod() == RefuelBy::Gsx;
}

bool IntegratorRuntime::IsAircraftRefuelBySelf() const
{
    return aircraft_ && aircraft_->RefuelMethod() == RefuelBy::Self;
}

void IntegratorRuntime::SetAutomationEnabled(const bool enabled)
{
    if (status_.enabled == enabled)
    {
        return;
    }

    status_.enabled = enabled;

    emit Updated();
}

void IntegratorRuntime::ConfirmLoading()
{
    LOG_INFO("Loading confirmed: requesting refueling.");

    stateMachine_.ConfirmLoading();

    emit Updated();
}

void IntegratorRuntime::MaybeAutoStart()
{
    if (!settings_.autoStartFlow || !IsConnected() || status_.enabled)
    {
        return;
    }

    LOG_INFO("Auto-start enabled: starting automation flow.");

    SetAutomationEnabled(true);
}

void IntegratorRuntime::ApplySettings(const AutomationSettings& settings)
{
    settings_ = settings;

    MaybeAutoStart();

    emit Updated();
}

bool IntegratorRuntime::ReloadSimbrief()
{
    if (!IsConnected() || !isSessionActive_ || settings_.simbriefPilotId <= 0)
    {
        return false;
    }

    const bool started = simbriefClient_.Reload();

    emit Updated();

    return started;
}
