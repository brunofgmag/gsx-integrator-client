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

| Aircraft | Minimum version | Progressive fuel and load | GSX refueling | Smart switch |
|----------|-----------------|---------------------------|---------------|--------------|
| TFDi Design MD-11 (passenger and freighter) | Any | No | No | INT/RAD switch |
| iFly 737 MAX 8 | SP1 | Yes | Yes | Push-to-talk switch |

Progressive fuel and load means the aircraft's fuel and payload follow the GSX service as it happens: the tanks fill while the truck pumps, and the weight grows as passengers board. Without it, the client applies the final planned figures in one step.

GSX refueling means the GSX fuel truck pumps the fuel itself, at its own pace. The client just follows the tank quantity, which is why the rate in the fuel card reads Auto for these aircraft and the fuel rate setting has no effect on them. If the pump feels slow, GSX has a Fuel Time Acceleration option of its own. Aircraft without it get their fuel written by the client instead.

The iFly needs SP1 or newer because earlier versions lack the built-in GSX integration the client depends on.

The smart switch is the cockpit control you flip to tell the client "go ahead". It works at three moments: at "Requesting fuel" with automatic loading turned off, where it does the same thing as the Start Loading button; during pushback, to confirm the engines started fine; and after a finished turnaround, to start the next one. Where each one is:

- TFDi MD-11: the INT/RAD switch on the captain's audio control panel, center pedestal.
- iFly 737 MAX 8: the R/T-I/C push-to-talk switch on the captain's audio control panel, lower left corner of the pedestal. Flick it to either side and let go.

More aircraft will be added over time, and the project is structured so new ones can be added without touching the rest of the app. If you fly something else, the client will connect but will not automate anything.

## The CommBus plugin

Install the CommBus plugin (`gsx-integrator-commbus`) in your Community folder; the installer above does that for you. The client runs without it, but you should use it: the plugin is a small bridge that lets the client activate the GSX menu icon on the MSFS toolbar, which makes the whole integration much smoother.

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
