# Contributing

Thanks for your interest in the project. The contribution that helps most right now is support for a new aircraft. The automation core, the UI and the GSX integration are in place; what limits who can use the app is the list of supported aircraft. If you want to change something outside aircraft support, open an issue first so we can talk about it before you write code.

## Building the project

You need:

- Visual Studio 2022 with the "Desktop development with C++" workload
- Qt 6.8 or newer, kit `msvc2022_64` (MinGW is not supported)
- The MSFS 2024 SDK, with `MSFS2024_SDK` or `MSFS_SDK` set in the environment
- CMake 3.21 or newer

```powershell
.\build.ps1 -Config Release
.\run-tests.ps1
```

After cloning, run the CMake configure once (or `git config core.hooksPath .githooks`) so the git hooks take effect. The pre-commit hook checks Conventional Commits and runs a Release build with tests before every commit.

## How the code is organized

The app follows MVVM with a layered core:

```
src/
├── domain/          business rules, no Qt, no SimConnect
├── application/     runtime, integrator service, snapshots and commands
├── infrastructure/  adapters: SimConnect, GSX, SimBrief, CommBus, aircraft
├── viewmodel/       Qt objects the UI binds to
└── qml/             views only: binding, layout, animation
```

The turnaround workflow lives in the domain and drives everything through interfaces. It asks the active `Aircraft` what state the plane is in and tells it what fuel and load to set. An aircraft adapter is the piece that translates those questions into the LVars and SimVars of one specific airplane. You should not need to touch the domain, the workflow or the UI to add an aircraft.

## Adding an aircraft

There are two reference implementations under `src/infrastructure/aircraft/`, and they show two different styles. `TfdiMd11.cpp` drives the airplane's own EFB: the setters store targets and `OnSlowTick` commits them to the EFB LVars. `IFly737Max.cpp` rides on top of GSX: the truck fills the native tanks on its own, and the adapter only writes payload, straight into the native stations. Read the one closer to your airplane side by side with this section.

### 1. Create the adapter

Add `YourAircraft.h` and `YourAircraft.cpp` under `src/infrastructure/aircraft/`, implementing the `Aircraft` interface from `src/domain/ports/Aircraft.h`. The interface has three groups of methods:

- Planned figures: `IsFlightPlanLoaded`, `GetPlannedFuelKg`, `GetPlannedZfwKg`, `GetPlannedPassengers` and `GetEmptyZfwKg`. These report what the airplane's own systems know about the flight.
- Current figures: `GetCurrentFuelKg`, `SetCurrentFuelKg`, `GetCurrentZfwKg` and `SetCurrentZfwKg`. The workflow calls the setters while GSX refuels and boards.
- State and capabilities: `IsPowered`, `IsEngineRunning`, `IsParkingBrakeSet`, `IsReadyToPush`, `IsReadyToDeboard`, plus `SupportsProgressiveFuel`, `SupportsProgressiveLoad`, `SupportsStairsOrJetways` and `IsRefueledExternally`, which tell the workflow how the airplane wants to be loaded.

`IsRefueledExternally` changes who owns the fuel. Return true when something other than the client fills the tanks during the GSX service, as on the iFly, where GSX pumps the native tanks directly. The workflow then stops simulating a fill rate: it mirrors `GetCurrentFuelKg` for the progress display, finishes when GSX reports the service complete, and `SetCurrentFuelKg` can be a no-op. Return false, and the workflow writes the fuel itself, in one step or ramping at the rate from the settings.

The weight setters have the same freedom. The MD-11 turns `SetCurrentZfwKg` into an EFB target; the iFly writes the value into the native payload stations, split in proportion to the default station loads from `flight_model.cfg` so the CG lands somewhere sensible. Use whatever surface the airplane gives you, and if you believe the airplane manages its own weights, test it in the sim first.

`ConsumeSmartSwitch` is the cockpit "go ahead" control. Implement it as an edge detector: return true once when the switch leaves its resting position, then false until it comes back and moves again. A spring-loaded switch needs no write-back (iFly); a latching one can be reset by writing the rest value (MD-11). The state machine polls it once per tick, and the active phase decides what a press means: start loading while the turnaround holds at "Requesting fuel", confirm the engine start during pushback, begin the next flight after the turnaround ends.

`OnSlowTick` runs periodically. Use it for polling and cheap housekeeping, the way the MD-11 uses it to commit EFB targets.

### 2. Talk to the sim through VariableGateway

The adapter reads and writes LVars and SimVars through the `VariableGateway` it receives in the constructor. One thing to know before you write predicates: a variable read returns its registered default until the first value actually arrives from the simulator. Pick defaults so your predicates fail safe during that window. A readiness check should read "not ready" while data is still missing, never "ready".

The same window bites writes that are computed from a read. The iFly refuses to touch the payload stations until `HasReceivedAVar` confirms `EMPTY WEIGHT` has arrived, because subtracting a default of zero would turn the entire ZFW into payload. If your setter does arithmetic on a sim variable, guard it the same way.

### 3. Register the aircraft

Detection is self-registered; there is no central list to edit. At the bottom of your `.cpp`, in an anonymous namespace, declare a descriptor and a registration:

```cpp
namespace
{
    std::unique_ptr<Aircraft> CreateYourAircraft(VariableGateway* variableGateway,
                                                 AutomationStatus* status,
                                                 const AircraftIdentity& identity)
    {
        return std::make_unique<YourAircraft>(variableGateway, status);
    }

    const AircraftDescriptor kYourAircraftDescriptor{
        "Vendor Type",
        {
            {MatchField::Title, MatchOp::Contains, "Vendor Type"},
            {MatchField::AtcModel, MatchOp::Equals, "TYPE"}
        },
        &CreateYourAircraft
    };

    [[maybe_unused]] const AircraftRegistration kYourAircraftRegistration{kYourAircraftDescriptor};
}
```

Matching is case insensitive and runs against the `TITLE` and `ATC MODEL` sim vars with `Equals`, `StartsWith` and `Contains`. An ATC MODEL match scores 4 points, a title match scores 2, and the descriptor with the highest score wins. Use the factory callback to tell variants apart, the way the MD-11 detects its freighter.

### 4. Register the files in the build

Add both files to `cmake/Sources.cmake`. The source list is explicit on purpose; there is no GLOB. Aircraft code compiles directly into the app target. Do not create a static library for it.

### 5. Test it

Spawn in the aircraft and check the log for the detection line, then fly a full turnaround: refuel, board, push. Logic that does not depend on the sim belongs in unit tests under `tests/`; `run-tests.ps1` runs them locally and CI runs them again on every pull request.

## Conventions

C++17. Collaborators are passed as non-owning raw pointers. The domain stays free of Qt and SimConnect. The `LOG_INFO`/`LOG_WARN`/`LOG_ERROR` macros belong to infrastructure and application code only. Commits follow Conventional Commits, enforced by the pre-commit hook.
