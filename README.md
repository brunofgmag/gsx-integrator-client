<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="assets/branding/logo-dark.svg">
    <img alt="GSX Integrator" src="assets/branding/logo-light.svg" width="420">
  </picture>
</div>

# GSX Integrator Client

A Windows desktop app that automates GSX Pro ground services in Microsoft Flight Simulator 2024 or 2020 (not tested). It reads your Simbrief flight plan and runs the turnaround for you: refueling and boarding with the numbers you dispatched, then the departure sequence when the aircraft is ready.

The app runs outside the simulator and talks to it through SimConnect. Nothing gets installed inside the sim except the CommBus plugin described below.

## Project status

This is a work in progress, currently in a testing phase. Expect bugs, and expect behavior to change between releases. The recommended way to install the app is the [GSX Integrator Installer](https://github.com/brunofgmag/gsx-integrator-installer), which sets up the client and the CommBus plugin together and updates both when new releases come out. If you would rather do it by hand, extract the release zip anywhere you like and start `gsx-integrator-client.exe`.

## What you need

- Windows 10 or 11
- Microsoft Flight Simulator 2024 or 2020 (not tested on 2020)
- GSX Pro v4.0.6+
- A Simbrief account (some aircraft do not share flight plan data)

## Supported aircraft

| Aircraft                                    | Minimum version | Progressive fuel and load | GSX refueling | Smart switch | Status |
|---------------------------------------------|-----------------|---------------------------|---------------|--------------|--------|
| TFDi Design MD-11 (passenger and freighter) | Any | No | No | INT/RAD switch | Beta |
| iFly 737 MAX 8                              | SP1 | Yes | Yes | Push-to-talk switch | Supported |
| Toliss A340-600 (Aerosoft A346 Pro)         | Any | SimBrief uplink | No | INT/RAD switch | Beta |

Progressive fuel and load means the aircraft's fuel and payload follow the GSX service as it happens: the tanks fill while the truck pumps, and the weight grows as passengers board. Without it, the client applies the final planned figures in one step.

GSX refueling means the GSX fuel truck pumps the fuel itself, at its own pace. The client just follows the tank quantity, which is why the rate in the fuel card reads Auto for these aircraft and the fuel rate setting has no effect on them. If the pump feels slow, GSX has a Fuel Time Acceleration option of its own. Aircraft without it get their fuel written by the client instead.

The iFly needs SP1 or newer because earlier versions lack the built-in GSX integration the client depends on.

The MD-11 and A340 are marked Beta because neither aircraft lets GSX load its fuel and cargo. The client fills them another way, the MD-11 by writing its weights directly and the Toliss through a SimBrief uplink, and both paths have had less testing than the iFly's built-in GSX integration.

The smart switch is the cockpit control you flip to tell the client "go ahead". It works at three moments: at "Requesting fuel" with automatic loading turned off, where it does the same thing as the Start Loading button; during pushback, to confirm the engines started fine; and after a finished turnaround, to start the next one. Where each one is:

- TFDi MD-11: the INT/RAD switch on the captain's audio control panel, center pedestal.
- iFly 737 MAX 8: the R/T-I/C push-to-talk switch on the captain's audio control panel, lower left corner of the pedestal. Flick it to either side and let go.
- Toliss A340-600: the INT/RAD switch on the captain's audio control panel, center pedestal. Flick it to either side; RAD springs back on its own and INT gets flipped back to the middle by the client.

More aircraft will be added over time, and the project is structured so new ones can be added without touching the rest of the app. If you fly something else, the client will connect but will not automate anything.

## Toliss A340-600 setup

The Toliss rejects any fuel or payload written from outside, so instead of writing them directly the client runs a SimBrief uplink through the center MCDU. That takes some one-time setup in the EFB, plus the right GSX profile.

### Toliss EFB

Save your SimBrief ID in the SIMBRIEF OFP tab, then turn on both IGNORE AIRAC/AC TYPE MISMATCH and SET PAYLOAD + FUEL TO SIMBRIEF.

The client only counts the aircraft as powered when external power is feeding or the APU is available. Batteries alone do not count, because the MCDUs stay dark on them. Call the GPU from the Toliss EFB or start the APU. The GSX ground power unit does not power this aircraft. Once refueling starts with the fuel hose connected, the client presses the center MCDU keys for you (MENU, ATSU, AOC MENU, FLT INIT), the aircraft pulls its fuel and payload from SimBrief, and the fuel card follows GSX's own pump counter. If the uplink does not land, trigger FLT INIT yourself on any MCDU and the flow continues.

### GSX profile

Community profiles from flightsim.to often ship with `refueling = 1` in their `gsx.cfg`, which makes the fuel truck park, pop a fuel quantity window and drive away without connecting the hose. This aircraft needs `refueling = 0`, so GSX waits for the aircraft's own fuel to move, which is what the uplink does. The profile is usually found under `%APPDATA%\Virtuali\Airplanes\aerosoft-a340-600-pro`. The client checks the `gsx.cfg` files there and shows an advisory with a Fix profile button when one is wrong. You can also edit the files yourself. Either way, GSX only picks the change up after you restart Couatl or reload the flight.

## The CommBus plugin

Install the CommBus plugin (`gsx-integrator-commbus`) in your Community folder; the [`gsx-integrator-installer`](https://github.com/brunofgmag/gsx-integrator-installer) does that for you. The client runs without it, but you should use it. The plugin is a small bridge that lets the client activate the GSX menu icon on the MSFS toolbar. Without it, some of GSX's own messages may not show on screen during the turnaround, because the client cannot open the menu for you.

## How to use it

1. Dispatch your flight in Simbrief. (some aircraft require a Simbrief plan)
2. Start the simulator and load your flight at a gate. Cold and dark or powered, either works as long as the engines are off.
3. Start the client. It connects to the sim on its own and detects the aircraft automatically.
4. Enter your Simbrief ID in the settings the first time you run it.
5. Watch the phases in the main window. The client requests refueling and boarding from GSX with the planned figures from your OFP and moves through the turnaround as the aircraft becomes ready.

A few tips:

- Let the client drive the GSX menu. If you click through GSX menus manually mid-turnaround, the two of you will fight over it.
- Planned fuel and ZFW come from your Simbrief OFP, so dispatch before you board, not after.
- If nothing happens after you load in, check that you are flying one of the supported aircraft and that GSX itself is running normally.

## Problems and feedback

Open an issue on GitHub with what you were flying, what you expected, and what happened instead. While the project is in testing, reports from real flights are the most useful thing you can send.
