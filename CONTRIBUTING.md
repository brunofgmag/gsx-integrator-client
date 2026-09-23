# Contributing

The contribution that helps most right now is support for a new aircraft. The automation, the UI and the GSX integration are done; the aircraft list is what limits who can use the app. For anything else, open an issue first so we can talk it over before you write code.

## Building

You need:

- Visual Studio 2022 with the "Desktop development with C++" workload
- Qt 6.8 or newer, kit `msvc2022_64`
- The MSFS 2024 SDK, with `MSFS2024_SDK` or `MSFS_SDK` set
- CMake 3.21 or newer

MSVC 2022 x64 is the only supported toolchain. MinGW configures, but only as an experimental cross-compile path.

```powershell
.\build.ps1 -Config Release
.\run-tests.ps1
```

`build.ps1 -RunTests` builds and tests in one go. `run-tests.ps1 -Filter <name>` runs a single test.

The CMake configure step installs the git hooks (or run `git config core.hooksPath .githooks` yourself). The commit-msg hook enforces Conventional Commits. The pre-commit hook runs a full Release build with tests whenever a commit touches `src/`, `tests/`, `cmake/`, `tools/` or the build files.

## Code layout

MVVM over a layered core:

```
src/
├── domain/          business rules, no Qt, no SimConnect
├── application/     runtime, integrator service, snapshots and commands
├── infrastructure/  adapters: SimConnect, GSX, SimBrief, CommBus, aircraft
├── viewmodel/       Qt objects the UI binds to
└── qml/             views only: binding, layout, animation
```

The turnaround workflow lives in the domain and works through interfaces. It asks the active `Aircraft` what state the airplane is in and tells it what fuel and load to set. An aircraft adapter translates those calls into one airplane's LVars and SimVars. Adding an aircraft should not touch the domain, the workflow or the UI.

## Adding an aircraft

Start by reading the adapter closest to your airplane. They solve loading in different ways:

- `tfdi/TfdiMd11.cpp` drives the airplane's own EFB. The setters stage targets and a slow rule writes them to the EFB LVars.
- `ifly/IFly737Max.cpp` lets the GSX truck fill the native tanks and only writes payload into the native stations.
- `fenix/FenixA32x.cpp` talks to the Fenix EFB through its GraphQL client.
- `pmdg/Pmdg777.cpp` writes fuel and payload through the PMDG tablet over the shared CommBus bridge.

All of them live under `src/infrastructure/aircraft/`.

### 1. Write the adapter

Create `YourAircraft.h` and `YourAircraft.cpp` in `src/infrastructure/aircraft/<vendor>/`, one lowercase folder per vendor (`fss/` holds both the 727 and the E-Jets, `pmdg/` both the 737 and the 777). The class implements `Aircraft` from `src/domain/ports/Aircraft.h`.

The interface falls into four groups. Every method with a default body can be left alone.

- Planned figures: `IsFlightPlanLoaded`, `GetPlannedFuelKg`, `GetPlannedZfwKg`, `GetPlannedPassengers`, `GetEmptyZfwKg`. What the airplane's own systems know about the flight. Override `RequiresEfbFlightPlan` to return true if the turnaround should wait at "Waiting for flight plan" until the pilot loads the plan in the aircraft.
- Current figures: `GetCurrentFuelKg`, `SetCurrentFuelKg`, `GetCurrentZfwKg`, `SetCurrentZfwKg`. The workflow calls the setters while GSX refuels and boards.
- State: `IsPowered`, `IsEngineRunning`, `IsParkingBrakeSet`, `IsHeldInPlace`, `IsReadyToPush`, `IsReadyToDeboard`, `IsCargoVariant`, `SupportsStairsOrJetways`, `CompletesPushbackViaInterruptMenu`, `OnLoadingStarted`, `ConsumeSmartSwitch`.
- Loading and ground equipment: `GetRefuelMethod` and `GetBoardMethod` say how the airplane wants to be loaded. Chocks, ground power and doors are optional and off by default: implement `SupportsChocksControl`/`SetChocks`, `SupportsGroundPowerControl`/`SetGroundPower`/`GetGroundPowerStatus` or `CloseAllDoors` only if the airplane drives them itself.

#### Refuel and board methods

These decide who owns fuel and payload during the GSX service:

- `Self`: the airplane loads everything at once through its own systems, like the MD-11's EFB or the A340's MCDU uplink. The client hands over the target once, and the progress bar follows the counter GSX animates.
- `Client`: the client writes the value tick by tick, at the rate from the settings. The iFly boards this way, because its `SetCurrentZfwKg` is what actually fills the payload stations. Anything else would send it out empty.
- `Gsx` (fuel only): the truck pumps the native tanks, as on the iFly. The client writes nothing, `SetCurrentFuelKg` can stay a no-op, and the progress bar mirrors the real fuel quantity.

There is no `Gsx` for boarding because GSX never loads payload itself. Either the airplane or the client has to write the weight.

The weight setters can use whatever surface the airplane offers. The MD-11 turns `SetCurrentZfwKg` into an EFB target. The iFly splits the value over the native payload stations in proportion to the default station loads from `flight_model.cfg`, so the CG lands somewhere sensible. The FSS 727 sets fuel as a fraction of tank capacity, the only surface it has, and spreads payload over the cargo stations in proportion to the capacities its EFB uses, leaving the crew seats alone. If you think the airplane manages its own weights, check it in the sim first.

#### Smart switch

`ConsumeSmartSwitch` reads the cockpit "go ahead" control. It returns true once when the switch leaves its resting position, then false until the switch comes back and moves again. The `SmartSwitch` helper in `src/infrastructure/aircraft/SmartSwitch.h` does this for you: give it the LVars, a predicate for "pressed", and a reset value for a latching switch (the MD-11). A spring-loaded switch needs no reset (the iFly). The state machine polls it once per tick, and the current phase decides what a press means: start loading at "Requesting fuel", confirm engine start during pushback, begin the next flight after the turnaround.

#### Rules

Work the airplane has to do on its own schedule, such as moving doors or committing EFB targets, goes into rules. A rule implements `AircraftRule` from `src/domain/ports/AircraftRule.h`, and the adapter returns its rules from `Rules()`. Put them in a `rules/` folder next to the adapter.

- `Cadence()` picks when it runs: `Fast` every second (the default), `Slow` every four seconds. The MD-11 commits EFB targets on `Slow` because the airplane applies weight changes with a delay.
- `Evaluate` returns `RuleVerdict::Pass()` or `RuleVerdict::Hold(ticks, reason)`. A fast rule that holds keeps the turnaround in its current phase for at most that many ticks, and a pilot action cancels the hold.
- `Act` does the writing through the `VariableWriter` it receives.

`Observe()` on the adapter runs every second before the rules, for polling and cheap housekeeping.

### 2. Read and write through VariableGateway

The adapter reads and writes LVars and SimVars through the `VariableGateway` it gets in its constructor. A read returns its registered default until the first real value arrives from the simulator, so pick defaults that fail safe: a readiness check must say "not ready" while data is missing, never "ready".

Writes computed from a read have the same problem. The iFly won't touch the payload stations until `HasReceivedAVar` confirms `EMPTY WEIGHT` has arrived, because subtracting a default of zero would turn the whole ZFW into payload. Guard any setter that does arithmetic on a sim variable the same way.

### 3. Register the aircraft

Detection registers itself; there is no central list. At the bottom of your `.cpp`, in an anonymous namespace, declare a descriptor and a registration:

```cpp
namespace
{
    std::unique_ptr<Aircraft> CreateYourAircraft(const AircraftContext& context, const AircraftIdentity& identity)
    {
        return std::make_unique<YourAircraft>(context.variableGateway, context.status);
    }

    const AircraftDescriptor kYourAircraftDescriptor{
        "Vendor Type",
        {
            {MatchField::Title, MatchOp::Contains, "Vendor Type"},
            {MatchField::AtcModel, MatchOp::Equals, "TYPE"}
        },
        &CreateYourAircraft, "vendor-type", "TYPE", RefuelBy::Gsx
    };

    [[maybe_unused]] const AircraftRegistration kYourAircraftRegistration{kYourAircraftDescriptor};
}
```

Don't drop the last three arguments. They have defaults, so the code compiles without them, but a test fails: the id and the short code must be non-empty and unique across the registry. The short code also sorts the aircraft in the settings list. `RefuelBy` tells the settings screen whether this profile gets a fuel rate control, with the same meaning as `GetRefuelMethod`.

Matching is case-insensitive against the `TITLE` and `ATC MODEL` sim vars, using `Equals`, `StartsWith` or `Contains`. An ATC MODEL match scores 4, a title match 2, and the highest score wins; a tie goes to the name that sorts first. To tell variants apart, use the creator, the way the MD-11 detects its freighter.

The creator gets an `AircraftContext` with the `VariableGateway`, the `AutomationStatus`, the shared CommBus bridge and the GSX gateway. Most aircraft only need the gateway. One that speaks CommBus borrows `context.commBusBridge` instead of opening its own SimConnect connection, as `Pmdg777.cpp` does to reach the PMDG tablet.

If the airplane needs `refueling = 0` in its GSX profile, add it to `kProfileFolders` in `src/infrastructure/gsx/GsxAircraftProfile.cpp`, or a profile with `refueling = 1` goes unnoticed and the GSX truck fills the tanks behind the client's back.

### 4. Add the files to the build

List every new file in `cmake/Sources.cmake`. The lists are explicit; there is no glob. Aircraft code compiles straight into the app target, never into a static library, because registration is a static object nobody references, and the MSVC linker silently drops it from a library: the aircraft builds, runs and is never detected.

The same goes for tests. In `cmake/Tests.cmake`, add the files to `gsxi-aircraft-detection-tests` and `gsxi-runtime-integrator-service-tests`, which compile every aircraft, and give the aircraft a test target of its own like `gsxi-tfdi-md11-tests`.

### 5. Test it

Unit-test everything that doesn't need the sim, under `tests/`. `run-tests.ps1` runs the suite locally and CI runs it on every pull request.

Then load the aircraft in the sim, check the log for the detection line, and fly a full turnaround: refuel, board, push.

## Conventions

- C++20.
- Collaborators are passed as non-owning raw pointers.
- The domain knows nothing about Qt or SimConnect. The `LOG_INFO`/`LOG_WARN`/`LOG_ERROR` macros are for infrastructure and application code only.
- Commits follow Conventional Commits.
