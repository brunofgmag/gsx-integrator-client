# GSX Integrator Client

A Windows desktop app that automates GSX Pro ground services in Microsoft Flight Simulator 2024 or 2020 (not tested). It reads your Simbrief flight plan and runs the turnaround for you: refueling and boarding with the numbers you dispatched, then the departure sequence when the aircraft is ready.

The app runs outside the simulator and talks to it through SimConnect. Nothing gets installed inside the sim except the CommBus plugin described below.

## Project status

This is a work in progress, currently in a testing phase. Expect bugs, and expect behavior to change between releases. Right now the app ships as a plain folder with an executable; an installer is planned but not built yet. Until then, extract the release zip anywhere you like and start `gsx-integrator-client.exe`.

## What you need

- Windows 10 or 11
- Microsoft Flight Simulator 2024 or 2020 (not tested on 2020)
- GSX Pro v4.0.6+
- A Simbrief account (some aircraft do not share flight plan data)

## Supported aircraft

- TFDi Design MD-11 (passenger and freighter)

That is the only one for now. More aircraft will be added over time, and the project is structured so new ones can be added without touching the rest of the app. If you fly something else, the client will connect but will not automate anything.

## The CommBus plugin

Install the CommBus plugin (`gsx-integrator-commbus`) in your Community folder. The client runs without it, but you should use it: the plugin is a small bridge that lets the client activate the GSX menu icon on the MSFS toolbar, which makes the whole integration much smoother.

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
