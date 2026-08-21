#include "IntegratorRuntime.h"

#include "../infrastructure/probe/ProbeLog.h"
#include "sim/SessionReadiness.h"
#include "../infrastructure/aircraft/AircraftFactory.h"
#include "../infrastructure/aircraft/AircraftRegistry.h"
#include "../infrastructure/logging/LogMacros.h"
#include "../infrastructure/gsx/GsxAircraftProfile.h"
#include "../infrastructure/pmdg/PmdgOptions.h"
#include "../infrastructure/gsx/GsxRemoteStateReducer.h"

namespace
{
    bool NeedsRefuelingFix(const std::filesystem::path& cfg)
    {
        const std::optional<int> refueling = GsxAircraftProfile::ReadRefueling(cfg);

        return !refueling.has_value() || *refueling != 0;
    }

    enum class TickMode
    {
        Idle,
        ObserveOnly,
        Driving
    };

    TickMode ResolveTickMode(const bool automationEnabled, const bool gsxAvailable)
    {
        if (automationEnabled && gsxAvailable)
        {
            return TickMode::Driving;
        }

        return probe::IsOn() ? TickMode::ObserveOnly : TickMode::Idle;
    }
}

IntegratorRuntime::IntegratorRuntime(QObject* parent)
    : QObject(parent),
      pluginClient_(&bridgeClient_),
      gsxService_(&varGateway_, &gsxRemoteState_),
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

    if (probe::IsOn())
    {
        LOG_INFO("Probe mode is on: readings go to %s",
                 qUtf8Printable(probe::Location()));
    }

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
                if (GsxRemoteStateReducer::ApplyPatch(gsxRemoteState_, path, v) == GsxPatchOutcome::Unknown
                    && unknownPatchPaths_.insert(path).second)
                {
                    LOG_WARN("GSX published an unknown path: %s", path.c_str());
                }
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
    return varGateway_.GetAVar("CAMERA STATE", "Number", 0.0) == 12.0;
}

void IntegratorRuntime::OnSimOpen(const char* appName)
{
    simVersion_ = SimVersionDetect::FromAppName(appName ? appName : "");

    LOG_INFO("Connected simulator: '%s' (%s)", appName ? appName : "?", SimVersionLabel(simVersion_));
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

    if (!SubscribeSimEvents())
    {
        HandleDisconnected();

        return;
    }

    bridgeClient_.Setup();
    pluginClient_.Setup();

    varGateway_.SetFastRefresh("FSDT_GSX_JETWAY");
    varGateway_.SetFastRefresh("FSDT_GSX_STAIRS");

    LOG_INFO("GSX Integrator connected to the simulator.");

    emit Updated();
}

bool IntegratorRuntime::SubscribeSimEvents()
{
    if (!simConnect_.SubscribeOneSecond([this] { Update(); }))
    {
        LOG_ERROR("Failed to subscribe to 1sec SimConnect event.");

        return false;
    }

    if (!simConnect_.SubscribeFourSeconds([this] { UpdateSlow(); }))
    {
        LOG_ERROR("Failed to subscribe to 4sec SimConnect event.");

        return false;
    }

    if (!simConnect_.SubscribeSimRunning([this](const bool running) { OnSimRunningChanged(running); }))
    {
        LOG_ERROR("Failed to subscribe to 'Sim' SimConnect system event.");

        return false;
    }

    if (!simConnect_.SubscribeToPause([this](const unsigned flag) { OnPauseChanged(flag); }))
    {
        LOG_ERROR("Failed to subscribe to 'Pause_EX1' SimConnect event.");

        return false;
    }

    return true;
}

void IntegratorRuntime::HandleDisconnected()
{
    LOG_INFO("Lost connection to the simulator.");

    simVersion_ = SimVersion::Unknown;

    varGateway_.Detach();
    simConnect_.Close();

    OnSessionEnd();

    reconnectTimer_.start();

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

        return;
    }

    bridgeClient_.Poll();
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
    ++pauseEvents_;
    pauseFlags_ = flag;
}

void IntegratorRuntime::ProbeGates()
{
    if (!probe::IsOn())
    {
        return;
    }

    char title[256] = {};
    varGateway_.FetchAircraftName(title, sizeof title);

    probe::Change("gates",
                  QStringLiteral("gate  ready=%1 pauseFlags=%2 pauseEvents=%3 camera=%4 isAircraft=%5 "
                                 "isAvatar=%6 sessionActive=%7 sim=%8 aircraft=%9 title='%10'")
                  .arg(IsSessionReady() ? 1 : 0)
                  .arg(pauseFlags_)
                  .arg(pauseEvents_)
                  .arg(varGateway_.GetAVar("CAMERA STATE", "Number", -1.0))
                  .arg(varGateway_.GetAVar("IS AIRCRAFT", "Number", -1.0))
                  .arg(varGateway_.GetAVar("IS AVATAR", "Number", -1.0))
                  .arg(isSessionActive_ ? 1 : 0)
                  .arg(static_cast<int>(simVersion_))
                  .arg(aircraft_ ? 1 : 0)
                  .arg(QString::fromLatin1(title)));
}

void IntegratorRuntime::Update()
{
    if (IsSimOnMenu() && isSessionActive_)
    {
        OnSessionEnd();

        return;
    }

    const auto emitOnExit = qScopeGuard([this] { emit Updated(); });

    ProbeGates();

    if (!IsSessionReady() || IsSessionPaused())
    {
        return;
    }

    simbriefClient_.Poll();

    const bool gsxOk = gsxService_.IsAvailable();
    status_.gsxAvailable = gsxOk;

    const TickMode mode = ResolveTickMode(status_.enabled, gsxOk);
    if (mode == TickMode::Idle)
    {
        return;
    }

    ResolveAircraft();

    if (!aircraft_)
    {
        return;
    }

    if (mode == TickMode::Driving)
    {
        stateMachine_.AttachAircraft(aircraft_.get());
        gsxMenu_.OnMenuChanged();
        stateMachine_.Tick();
    }

    aircraft_->OnTick();
    probe_.Observe(*aircraft_, varGateway_, GetAircraftProfileId());
}

bool IntegratorRuntime::IsLoadingCargoPhase() const
{
    const TurnaroundPhase phase = GetPhase();

    return phase == TurnaroundPhase::RequestBoarding
        || phase == TurnaroundPhase::Boarding
        || phase == TurnaroundPhase::RequestDeboarding
        || phase == TurnaroundPhase::Deboarding;
}

bool IntegratorRuntime::IsCargoDoorStuck() const
{
    return aircraft_ && IsLoadingCargoPhase() && aircraft_->IsMainDeckCargoDoorStuck();
}

void IntegratorRuntime::UpdateSlow()
{
    if (!IsSessionActive() || !aircraft_ || !status_.enabled || !IsSessionReady() || IsSessionPaused())
    {
        return;
    }

    aircraft_->OnSlowTick();

    gsxService_.ReassertTakeovers();
    CheckGsxProfile();
    CheckPmdgOptions();
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

    bridgeClient_.Shutdown();

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

void IntegratorRuntime::ClearFlightState()
{
    aircraft_.reset();
    gsxProfile_.Reset();
    pmdgOptions_.Reset();

    ResetSession();
}

void IntegratorRuntime::OnFlightStart()
{
    LOG_INFO("Flight started. Initializing session...");

    isSessionActive_ = true;

    RestartFlow();
}

void IntegratorRuntime::OnSessionEnd()
{
    LOG_INFO("Session ended. Cleaning up...");

    isSessionActive_ = false;

    ClearFlightState();

    emit Updated();
}

void IntegratorRuntime::ResolveAircraft()
{
    if (aircraft_)
    {
        return;
    }

    aircraft_ = DetectAircraft(&varGateway_, &status_, &bridgeClient_, &aircraftDescriptor_);
    if (aircraft_)
    {
        status_.aircraftSupported = true;
        gsxProfile_.roots = GsxAircraftProfile::ProfileRootsFor(aircraftDescriptor_->name);
        gsxProfile_.flagsMissing = GsxAircraftProfile::FlagsMissingProfile(aircraftDescriptor_->name);
        pmdgOptions_.ini = PmdgOptions::PathFor(aircraftDescriptor_->name).value_or(std::filesystem::path{});
        CheckGsxProfile();
        CheckPmdgOptions();
        emit Updated();
    }
}

void IntegratorRuntime::CheckGsxProfile()
{
    if (gsxProfile_.roots.empty())
    {
        gsxProfile_.cfgs.clear();
        gsxProfile_.conflict = false;
        return;
    }

    gsxProfile_.cfgs = GsxAircraftProfile::FindCfgs(gsxProfile_.roots);

    const bool wasConflicting = gsxProfile_.conflict;

    bool conflict = gsxProfile_.cfgs.empty() && gsxProfile_.flagsMissing;
    for (const auto& cfg : gsxProfile_.cfgs)
    {
        if (NeedsRefuelingFix(cfg))
        {
            conflict = true;
            if (!wasConflicting)
            {
                LOG_WARN("GSX profile '%s' does not set 'refueling = 0'; the fuel truck will not connect.",
                         cfg.string().c_str());
            }
        }
    }

    gsxProfile_.conflict = conflict;
}

IntegratorSnapshot IntegratorRuntime::Snapshot() const
{
    IntegratorSnapshot snapshot;
    snapshot.connected = IsConnected();
    snapshot.sessionActive = IsSessionActive();
    snapshot.automationEnabled = status_.enabled;
    snapshot.gsxAvailable = status_.gsxAvailable;
    snapshot.aircraftSupported = status_.aircraftSupported;
    snapshot.canToggleAutomation = snapshot.connected;
    snapshot.canStartLoading = snapshot.connected
        && status_.enabled
        && GetPhase() == TurnaroundPhase::RequestFuel
        && !settings_.autoStartLoading
        && !IsLoadingConfirmed();
    snapshot.canReloadSimbrief = snapshot.connected
        && snapshot.sessionActive
        && settings_.simbriefPilotId > 0
        && GetPhase() <= TurnaroundPhase::WaitingFlightPlan;
    snapshot.aircraftName = GetAircraftName().toStdString();
    snapshot.aircraftProfileId = GetAircraftProfileId();
    snapshot.refuelByGsx = IsAircraftRefuelByGsx();
    snapshot.refuelBySelf = IsAircraftRefuelBySelf();
    snapshot.cargoAircraft = IsAircraftCargoVariant();
    snapshot.efbFlightPlan = AircraftRequiresEfbFlightPlan();
    snapshot.gsxProfileConflict = HasGsxProfileConflict();
    snapshot.gsxProfileFixable = CanFixGsxProfile();
    snapshot.pmdgOptionsConflict = HasPmdgOptionsConflict();
    snapshot.pmdgOptionsFixable = CanFixPmdgOptions();
    snapshot.cargoDoorStuck = IsCargoDoorStuck();
    snapshot.phase = GetPhase();
    snapshot.flightPlanStatus = status_.flightPlanStatus;
    snapshot.simbriefRefusal = gsxService_.GetSimbriefRefusal();
    snapshot.fuelProgress = status_.fuelProgress;
    snapshot.boardingProgress = status_.boardingProgress;
    snapshot.deboardingProgress = status_.deboardingProgress;
    snapshot.plannedFuelKg = status_.plannedFuelKg;
    snapshot.loadedFuelKg = status_.loadedFuelKg;
    snapshot.plannedZfwKg = status_.plannedZfwKg;
    snapshot.plannedPax = status_.plannedPassengers;
    snapshot.boardedPax = status_.boardedPassengers;
    snapshot.targetFuelKg = status_.targetFuelKg;
    snapshot.targetZfwKg = status_.targetZfwKg;
    snapshot.targetPax = status_.targetPassengers;
    snapshot.delayTicksRemaining = GetDelayTicksRemaining();
    snapshot.autoWeightUnit = static_cast<int>(GetAutoWeightUnit());

    return snapshot;
}

void IntegratorRuntime::CheckPmdgOptions()
{
    if (pmdgOptions_.ini.empty())
    {
        pmdgOptions_.conflict = false;
        return;
    }

    const std::optional<bool> enabled = PmdgOptions::ReadDataBroadcast(pmdgOptions_.ini);
    const bool conflict = enabled.has_value() && !*enabled;

    if (conflict && !pmdgOptions_.conflict)
    {
        LOG_WARN("'%s' has no '[SDK] EnableDataBroadcast=1'; the client cannot read the aircraft state.",
                 pmdgOptions_.ini.string().c_str());
    }

    pmdgOptions_.conflict = conflict;
}

bool IntegratorRuntime::CanFixPmdgOptions() const
{
    return pmdgOptions_.conflict && !pmdgOptions_.ini.empty();
}

bool IntegratorRuntime::FixPmdgOptions()
{
    if (!CanFixPmdgOptions() || !PmdgOptions::EnableDataBroadcast(pmdgOptions_.ini))
    {
        return false;
    }

    LOG_INFO("'%s' updated: [SDK] EnableDataBroadcast = 1. Reload the flight to apply it.",
             pmdgOptions_.ini.string().c_str());

    CheckPmdgOptions();

    emit Updated();

    return true;
}

bool IntegratorRuntime::CanFixGsxProfile() const
{
    return gsxProfile_.conflict && !gsxProfile_.cfgs.empty();
}

bool IntegratorRuntime::FixGsxProfile()
{
    if (gsxProfile_.cfgs.empty())
    {
        return false;
    }

    for (const auto& cfg : gsxProfile_.cfgs)
    {
        if (!NeedsRefuelingFix(cfg))
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
    const double camera = varGateway_.GetAVar("CAMERA STATE", "Number", 0.0);

    if (simVersion_ != SimVersion::Msfs2024)
    {
        return SessionReadiness::Evaluate(simVersion_, camera, 0.0, 0.0);
    }

    return SessionReadiness::Evaluate(
        simVersion_,
        camera,
        varGateway_.GetAVar("IS AIRCRAFT", "Number", 0.0),
        varGateway_.GetAVar("IS AVATAR", "Number", 0.0)
    );
}

QString IntegratorRuntime::GetAircraftName() const
{
    return aircraftDescriptor_ ? QString::fromUtf8(aircraftDescriptor_->name) : QString();
}

std::string IntegratorRuntime::GetAircraftProfileId() const
{
    return aircraft_ && aircraftDescriptor_ ? aircraftDescriptor_->id : std::string();
}

bool IntegratorRuntime::IsAircraftRefuelByGsx() const
{
    return aircraft_ && aircraft_->GetRefuelMethod() == RefuelBy::Gsx;
}

bool IntegratorRuntime::IsAircraftRefuelBySelf() const
{
    return aircraft_ && aircraft_->GetRefuelMethod() == RefuelBy::Self;
}

bool IntegratorRuntime::IsAircraftCargoVariant() const
{
    return aircraft_ && aircraft_->IsCargoVariant();
}

bool IntegratorRuntime::AircraftRequiresEfbFlightPlan() const
{
    return aircraft_ && aircraft_->RequiresEfbFlightPlan();
}

WeightUnit IntegratorRuntime::GetAutoWeightUnit() const
{
    return weight::ResolveAutoWeightUnit(settings_.simbriefPilotId > 0,
                                         status_.flightPlanStatus == FlightPlanStatus::Ready,
                                         status_.simbriefUnit,
                                         aircraft_ ? aircraft_->GetNativeWeightUnit() : std::nullopt);
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

void IntegratorRuntime::RestartFlow()
{
    LOG_INFO("Restarting turnaround flow.");

    ClearFlightState();

    MaybeAutoStart();

    emit Updated();
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
