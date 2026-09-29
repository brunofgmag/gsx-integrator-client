import QtQuick

QtObject {
    id: root

    property int phase: 0
    property int skipCalls: 0
    property int lastDelta: 0
    property bool longPhase: root.phase % 2 !== 0

    readonly property bool connected: true
    readonly property bool debugToolsAvailable: true
    readonly property bool canStartLoading: false
    readonly property bool canStartFlow: true
    readonly property bool canRestartFlow: true
    readonly property bool canReloadSimbrief: true
    readonly property bool enabled: true
    readonly property bool gsxAvailable: true
    readonly property bool aircraftSupported: true
    readonly property bool cargoAircraft: false
    readonly property bool inDeboardingPhase: false
    readonly property bool simbriefReady: true
    readonly property bool simbriefError: false
    readonly property bool gsxProfileConflict: false
    readonly property bool pmdgOptionsConflict: false
    readonly property bool fuelRequestStalled: root.longPhase
    readonly property bool fuelPlanOverCapacity: false
    readonly property bool fuelDidNotStay: false
    readonly property bool engineConfirmationBlocked: false
    readonly property bool doorsHoldingPushback: false
    readonly property bool serviceInterrupted: false
    readonly property bool cargoDoorStuck: root.longPhase
    readonly property bool servicesStalled: root.longPhase

    readonly property double fuelProgress: 0.5
    readonly property double boardingProgress: 0.5
    readonly property double deboardingProgress: 0

    readonly property string commandError: ""
    readonly property string commandErrorLabel: "Error"
    readonly property string simbriefFailureText: ""
    readonly property string holdCountdownText: root.longPhase ? "Hold 00:30" : ""
    readonly property string stateText: root.longPhase
        ? "Waiting for the ground power unit to be connected and the chocks placed"
        : "Boarding"
    readonly property string nextPhaseText: root.longPhase ? "Next: Departure" : ""
    readonly property string phaseCounterText: "3 / 24"
    readonly property string phaseTip: root.longPhase
        ? "The turnaround is holding until every door is closed, the jet bridge is retracted, the ground crew is clear of the aircraft and the pilot confirms the pushback."
        : "Boarding."
    readonly property string cargoDoorAdvisoryText: "The cargo door is stuck open."
    readonly property string servicesAdvisoryText: "A service has not answered in a while."

    readonly property string simLabel: "Sim"
    readonly property string simStatusText: "Connected"
    readonly property string gsxLabel: "GSX Pro"
    readonly property string gsxStatusText: "Ready"
    readonly property string aircraftLabel: "Aircraft"
    readonly property string aircraftNameText: "Test 737"
    readonly property string turnaroundModeLabel: "Turnaround"
    readonly property string turnaroundModeText: "Auto"
    readonly property string loadingModeLabel: "Loading"
    readonly property string loadingModeText: "Manual"
    readonly property string turnaroundStateLabel: "State"
    readonly property string fuelCardLabel: "Fuel"
    readonly property string fuelProgressText: "50 %"
    readonly property string loadedFuelLabel: "Loaded"
    readonly property string loadedFuelText: "1 000 KG"
    readonly property string targetFuelLabel: "Target"
    readonly property string targetFuelText: "2 000 KG"
    readonly property string fuelRateLabel: "Rate"
    readonly property string fuelRateText: "10 KG/S"
    readonly property string paxCardLabel: "Boarding"
    readonly property string paxProgressText: "50 %"
    readonly property string paxLabel: "Pax"
    readonly property string paxCountText: "80 / 160"
    readonly property string targetZfwLabel: "ZFW"
    readonly property string targetZfwText: "60 000 KG"
    readonly property string simbriefCardLabel: "SimBrief"
    readonly property string simbriefStatusText: "Ready"
    readonly property string plannedFuelLabel: "Fuel"
    readonly property string plannedFuelText: "2 000 KG"
    readonly property string plannedZfwLabel: "ZFW"
    readonly property string plannedZfwText: "60 000 KG"
    readonly property string plannedPaxLabel: "Pax"
    readonly property string plannedPaxText: "160"
    readonly property string reloadSimbriefLabel: "Reload"
    readonly property string startFlowLabel: "Start flow"
    readonly property string startLoadingLabel: "Start loading"
    readonly property string restartFlowLabel: "Restart flow"
    readonly property string confirmRestartLabel: "Confirm restart"
    readonly property string dismissAdvisoryLabel: "Dismiss"
    readonly property string gsxProfileAdvisoryText: ""
    readonly property string gsxProfileActionLabel: ""
    readonly property string pmdgOptionsAdvisoryText: ""
    readonly property string pmdgOptionsActionLabel: ""
    readonly property string fuelRequestAdvisoryText: "The fuel request has not been answered yet."
    readonly property string fuelPlanAdvisoryText: ""
    readonly property string fuelStayAdvisoryText: ""
    readonly property string engineConfirmationAdvisoryText: ""
    readonly property string openDoorAdvisoryText: ""
    readonly property string serviceInterruptedAdvisoryText: ""

    function debugSkipPhase(delta) {
        root.skipCalls++
        root.lastDelta = delta
        root.phase += delta
    }

    function startFlow() {}
    function startLoading() {}
    function restartFlow() {}
    function reloadSimbrief() {}
    function fixGsxProfile() {}
    function fixPmdgOptions() {}
    function dismissFuelStayAdvisory() {}
}
