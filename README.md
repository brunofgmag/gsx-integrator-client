<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="assets/branding/logo-dark.svg">
    <img alt="GSX Integrator" src="assets/branding/logo-light.svg" width="420">
  </picture>
</div>

# GSX Integrator Client

A Windows app that runs your GSX Pro turnaround in Microsoft Flight Simulator. It reads your SimBrief flight plan, asks GSX for refueling and boarding with the numbers you dispatched, and runs the departure sequence once the aircraft is ready.

It runs outside the simulator and talks to it through SimConnect. The only thing that goes into the sim is the CommBus plugin.

The project is in testing. Expect bugs, and expect behavior to change between releases.

## Installing

Use the [GSX Integrator Installer](https://github.com/brunofgmag/gsx-integrator-installer). It installs the client and the CommBus plugin and keeps both up to date. To install by hand, extract the release zip anywhere and run `gsx-integrator-client.exe`.

You need:

- Windows 10 or 11
- Microsoft Flight Simulator 2024 (2020 may work but is not tested)
- GSX Pro 4.0.19 or newer
- A SimBrief account
- The CommBus plugin, version 0.2.0 or newer for the PMDG aircraft

## Supported aircraft

| Aircraft | Minimum version | Fuel | Payload | Smart switch | Chocks & GPU | Status |
|---|---|---|---|---|---|---|
| TFDi Design MD-11 (passenger and freighter) | Any | EFB, at once | EFB, at once | INT/RAD | Chocks + GPU | Supported |
| iFly 737 MAX 8 | SP1 | GSX, progressive | Client, progressive | R/T-I/C | GPU only | Supported |
| Fenix A319 / A320 / A321 | Any | Client, progressive | Client, progressive | INT/RAD | Chocks + GPU | Supported |
| PMDG 777-300ER / F / -200ER / -200LR | Any | Client, progressive | Client, progressive | MIC/INT | Chocks + GPU | Supported |
| PMDG 737-800 / BBJ2 / BCF / BDSF | Any | Client, progressive | Client, progressive | R/T-I/C | Chocks + GPU | Supported |
| ToLiss A340-600 | Any | MCDU uplink, at once | MCDU uplink, at once | INT/RAD | GPU only, visual | Beta |
| JustFlight Avro RJ70 / RJ85 / RJ100 (incl. QT) | Any | GSX, progressive | GSX, progressive | R/T-INT | Chocks + GPU | Beta |
| FSS Boeing 727-200F / 200RE Freighter | Any | Client, progressive | Client, progressive | SERV INT | Chocks + GPU | Beta |
| FSS Embraer E190 / E195 (incl. Freighter) | Any | Client, progressive | Client, progressive | Call RAMP | Chocks + GPU | Beta |

Any other aircraft connects but gets no automation.

The iFly needs SP1 because older versions lack the GSX integration the client relies on. The A340 is in beta because the aircraft itself is unstable enough to get in the way of testing. The Avro RJ and the FSS aircraft are in beta because they are new and need more flights.

### How each aircraft loads

The progress bars look the same everywhere. What differs is who moves the fuel and the weight:

- Client, progressive: fuel flows at the rate set in the fuel card while the GSX hose is connected, and passengers and cargo follow GSX boarding. Freighters take the whole payload as cargo, spread over the main deck and the holds.
- GSX, progressive: the GSX truck fills the tanks at its own pace, so the fuel card rate reads Auto. If it feels slow, GSX has a Fuel Time Acceleration option.
- At once: the MD-11 and the A340 load fuel and payload in one step, and the bars follow GSX.

GSX has its own PMDG automation that types fuel and payload into the FMC. The client turns it off so the two don't fight over the numbers.

### Chocks and GPU

With "Call GPU & chocks" on, the client asks GSX for a ground power unit at the gate and sends it away before pushback. "Call GPU & chocks on arrival" does the same after landing. On "Chocks + GPU" aircraft the client also sets and removes the chocks; on "GPU only" it leaves them alone.

Some aircraft use their own ground power instead of the GSX unit, and the client drives that one:

- The Fenix GPU, through the Fenix EFB.
- The 727 and E-Jet GPU, since neither takes power from GSX. On the 727, the flight engineer's EXT POWER switch stays yours, and that switch is what powers the buses.
- The A340 is the exception: the GSX unit parks next to it but powers nothing, and the client does not drive the ToLiss GPU. Start it from the ToLiss EFB or run the APU.

### The smart switch

The smart switch is a cockpit control you flip to tell the client "go ahead". It does three things:

- At "Requesting fuel", with automatic loading off, it starts loading, same as the Start Loading button.
- During pushback, it confirms the engines started.
- After a finished turnaround, it starts the next one.

| Aircraft | Where | How |
|---|---|---|
| TFDi MD-11 | INT/RAD switch, captain's audio panel, center pedestal | Flip it |
| iFly 737 MAX 8 | R/T-I/C push-to-talk switch, captain's audio panel, lower left of the pedestal | Flick to either side and let go |
| ToLiss A340-600 | INT/RAD switch, captain's audio panel, center pedestal | Flick to either side. RAD springs back; the client returns INT to center |
| Fenix A319/A320/A321 | INT/RAD switch, captain's audio panel, center pedestal | Flip down to INT; the client returns it to center |
| PMDG 777 | MIC/INT switch, either pilot's audio panel, center pedestal | Push down to INT; it springs back. Up is radio transmit and is ignored |
| PMDG 737 | R/T-I/C switch, captain's audio panel, center pedestal | Flick to R/T and let go. I/C latches and is ignored |
| JustFlight Avro RJ | R/T-INT rocker, captain's audio panel | Flick to INT. R/T is radio transmit and is ignored |
| FSS 727 | SERV INT switch, shared by the captain's, first officer's and flight engineer's audio panels | Flip it on; the client turns it back off |
| FSS E190/E195 | Call RAMP button, audio panel | Press and let go; the client clears the call |

On the PMDG 737, R/T is also radio transmit. If you talk on VATSIM with that switch instead of a joystick button, every transmission counts as a go-ahead.

## Setup

### GSX settings

Check four things on the GSX Settings page before your first flight:

- Turn Ignore Time on (Simulation area, next to the SimBrief username). It ships off, and then GSX rejects any flight plan whose departure time has passed. The turnaround sits at "Waiting for flight plan" until you dispatch again.
- Turn Trust Simbrief passengers number on (same area). With it off, some aircraft send their own passenger count and GSX boards that instead of your OFP.
- Leave Assistance Services "Auto" mode off. In Auto mode GSX calls its own services in sequence, which is the client's job. Running both means two dispatchers fighting over one menu.
- Set the interval between "Waiting for your action" messages to 25 seconds (Timings area). The default 15 is a lot of nagging while the client opens the doors for you. Much longer and you stop noticing when GSX really is stuck.

### GSX aircraft profile

On the MD-11, the Fenix, the PMDG aircraft and the A340, the aircraft's GSX profile needs `refueling = 0` in its `gsx.cfg`. Community profiles from flightsim.to often ship with `refueling = 1`, which hands the fuel back to GSX: on some aircraft the truck fills the tanks behind the client's back, and on the A340 it parks, pops a fuel quantity window and drives off without connecting the hose. The A340 also needs the profile to exist, and the client tells you when none is installed. The client checks the profile under `%APPDATA%\Virtuali\Airplanes\` and, when the setting is wrong, shows an advisory with a Fix profile button. GSX picks up the change after you restart it or reload the flight.

### PMDG 777 and 737

The client reads these aircraft through the PMDG SDK data broadcast, which is off by default. The options file lives under `%APPDATA%\Microsoft Flight Simulator 2024\WASM\MSFS2024\<package>\work\`:

- 777: `777_Options.ini`, one per installed variant, in `pmdg-aircraft-77w`, `pmdg-aircraft-77f`, `pmdg-aircraft-77l` and `pmdg-aircraft-77er`
- 737: `737_Options.ini` in `pmdg-aircraft-738`, shared by the whole family

It needs:

```ini
[SDK]
EnableDataBroadcast=1
```

When the line is missing, the client shows an advisory with an Enable broadcast button that writes it for you. Edit the file with the sim closed, or the aircraft overwrites it on exit, then reload the flight.

The CommBus plugin is required on both, because fuel and payload go through it.

On the 737 BCF and BDSF the whole payload goes in as main deck cargo. That door is hydraulic: with the electric pumps off, the open command waits and the door moves once there is pressure.

### ToLiss A340-600

The A340 rejects fuel and payload written from outside, so the client runs a SimBrief uplink through the center MCDU. In the ToLiss EFB:

1. Save your SimBrief ID in the SIMBRIEF OFP tab.
2. Turn on IGNORE AIRAC/AC TYPE MISMATCH.
3. Turn on SET PAYLOAD + FUEL TO SIMBRIEF.

The client counts the aircraft as powered only with external power connected or the APU running; batteries alone leave the MCDUs dark. Once refueling starts and the hose is connected, the client presses MENU, ATSU, AOC MENU and FLT INIT on the center MCDU, and the aircraft pulls fuel and payload from SimBrief. If the uplink doesn't arrive, press FLT INIT yourself on any MCDU and the turnaround carries on.

### The CommBus plugin

The plugin (`gsx-integrator-commbus`) goes in your Community folder, and the installer puts it there. It bridges the client to the parts of the sim that only a WASM module can reach:

- The client's app on the MSFS 2024 EFB tablet, where you follow the turnaround and give the go-ahead from the cockpit.
- Opening the GSX panel on the MSFS toolbar.
- Fuel and payload on the PMDG 777 and 737. Without the plugin they will not refuel or board.

Other aircraft work without it. The one thing you then do by hand is the pushback menu, where you choose where the tug leaves you.

## Flying with it

1. Dispatch your flight in SimBrief.
2. Load the flight at a gate with the engines off, cold and dark or powered.
3. Start the client. It connects and detects the aircraft by itself.
4. The first time, enter your SimBrief ID in the settings.
5. Follow the phases in the main window. The client requests refueling and boarding with your OFP figures and moves on as the aircraft gets ready.

Planned fuel and ZFW come from the OFP, so dispatch before boarding, not after. Let the client drive the GSX menu: clicking through it yourself mid-turnaround puts the two of you in a fight.

Aircraft notes:

- Fenix, iFly, Avro RJ, PMDG 777 and 737: import your SimBrief plan in the aircraft (its EFB, or on the PMDG the tablet's flight plan page or the FMC). The turnaround waits at "Waiting for flight plan" until you do. On the iFly, use the Balance & Payload page and load only the flight plan, not the weights.
- Avro RJ: the client asks GSX for the aircraft's own airstairs. At a jetway stand it boards through the jetway.
- FSS 727: the client loads from your OFP and never reads the tablet, so there is nothing to import. If you do import a plan, make it the same one you dispatched; the client can't see it and won't warn you if they differ. Once loading starts, leave the tablet's fuel player alone: pressing Play drains the tanks to the tablet's figure. During boarding the client opens the main deck door from the cargo door panel when the GSX loader is waiting and closes it when boarding ends. Don't touch that panel while the door moves; cutting its master power halfway freezes the door.
- FSS E190/E195: the client loads from your OFP and never reads the EFB. Don't import a SimBrief plan into the EFB, because it overwrites the loaded fuel and payload the moment boarding ends. The EFB's GSX remote control also opens the GSX menu on its own; if you see that, turn `enableGsxRemoteControl` off in the EFB settings.

If nothing happens after loading in, check that the aircraft is on the list above and that GSX itself is running normally.

## Problems and feedback

Open an issue on GitHub with the aircraft, what you expected and what happened. Reports from real flights are the most useful thing you can send while the project is in testing.
