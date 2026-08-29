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

This is a work in progress, currently in a testing phase. Expect bugs, and expect behavior to change between releases. The recommended way to install is the [GSX Integrator Installer](https://github.com/brunofgmag/gsx-integrator-installer), which sets up the client and the CommBus plugin together and keeps both updated. If you prefer to do it by hand, extract the release zip anywhere and start `gsx-integrator-client.exe`.

## What you need

- Windows 10 or 11
- Microsoft Flight Simulator 2024 or 2020 (not tested on 2020)
- GSX Pro v4.0.19+
- A Simbrief account
- The CommBus plugin, which the in-sim EFB app and some aircraft depend on: the PMDG 777 and 737 need version 0.2.0 or newer

## Supported aircraft

| Aircraft                                    | Minimum version | Fuel                  | Payload               | Smart switch | Chocks & GPU | Status |
|---------------------------------------------|-----------------|-----------------------|-----------------------|--------------|--------------|--------|
| TFDi Design MD-11 (passenger and freighter) | Any | Client (at once)      | Client (at once)      | INT/RAD switch | Chocks + GPU | Supported |
| iFly 737 MAX 8                              | SP1 | GSX (progressive)     | Client (progressive)  | Push-to-talk switch | GPU only | Supported |
| Toliss A340-600                             | Any | MCDU uplink (at once) | MCDU uplink (at once) | INT/RAD switch | GPU only (visual) | Beta |
| Fenix A319 / A320 / A321                    | Any | Client (progressive)  | Client (progressive)  | INT/RAD switch | Chocks + GPU | Supported |
| PMDG 777-300ER / F / -200ER / -200LR        | Any | Client (progressive)  | Client (progressive)  | MIC/INT switch | Chocks + GPU | Supported |
| PMDG 737-800 / BBJ2 / BCF / BDSF            | Any | Client (progressive)  | Client (progressive)  | R/T-I/C switch | Chocks + GPU | Supported |
| JustFlight Avro RJ70 / RJ85 / RJ100 (incl. QT) | Any | GSX (progressive) | GSX (progressive)     | R/T-INT switch | Chocks + GPU | Beta |

Every aircraft gets the same progress bars during refueling and boarding; the Fuel and Payload columns say how each one loads. On the Fenix, the PMDG 777 and 737, and the Avro RJ, fuel goes in at the rate set in the fuel card while the GSX hose is connected, and passengers and cargo follow GSX's boarding. On the freighters the whole payload goes in as main-deck cargo. On the iFly the GSX truck pumps the tanks at its own pace, so the rate in the fuel card reads Auto; if it feels slow, GSX has a Fuel Time Acceleration option. The MD-11 and the A340 load fuel and payload in one step, and the progress bars follow GSX. GSX ships its own automation for the PMDG that types fuel and payload into the FMC; the client turns it off so the two never fight over the numbers.

The iFly needs SP1 or newer because earlier versions lack the built-in GSX integration the client depends on.

Two aircraft are still Beta: the Toliss A340, because the aircraft itself has stability problems that get in the way of testing, and the Avro RJ, because it is a recent addition and needs more flights.

The Chocks & GPU column says what the "Call GPU & chocks" settings do on each aircraft. When they are on, the client asks GSX for a ground power unit at the gate (and again after landing, if enabled) and sends it away before pushback. Chocks + GPU means the client also places and removes the aircraft's chocks; GPU only means it leaves the chocks alone. The Fenix brings its own GPU, so the client drives that one through the EFB instead of calling the GSX truck. On the A340 the GSX unit is cosmetic: it parks beside the aircraft but does not feed it power, so start the GPU from the Toliss EFB or use the APU, as the setup section below explains.

The smart switch is the cockpit control you flip to tell the client "go ahead". It works at three moments: at "Requesting fuel" with automatic loading turned off, where it does the same thing as the Start Loading button; during pushback, to confirm the engines started fine; and after a finished turnaround, to start the next one. Where each one is:

- TFDi MD-11: the INT/RAD switch on the captain's audio control panel, center pedestal.
- iFly 737 MAX 8: the R/T-I/C push-to-talk switch on the captain's audio control panel, lower left corner of the pedestal. Flick it to either side and let go.
- Toliss A340-600: the INT/RAD switch on the captain's audio control panel, center pedestal. Flick it to either side; RAD springs back on its own and the client flips INT back to the middle.
- Fenix A319/A320/A321: the INT/RAD switch on the captain's audio control panel, center pedestal. Flip it down to INT and the client puts it back in the middle.
- PMDG 777: the MIC/INT switch on either pilot's audio control panel, center pedestal. Push it down to INT; it springs back on its own. The up position is your radio push-to-talk and the client leaves it alone, so transmitting on VATSIM never triggers anything.
- PMDG 737: the R/T-I/C switch on the captain's audio control panel, center pedestal. Flick it to R/T and let go. The client ignores the I/C side, which latches where you leave it. R/T is also your radio transmit position, so talking on VATSIM with this switch, rather than a joystick button, counts as a go-ahead.
- JustFlight Avro RJ: the R/T-INT rocker on the captain's audio control panel. Flick it to INT. The R/T side is your radio transmit and the client ignores it, so transmitting on VATSIM never triggers anything.

More aircraft are on the way. If you fly something else, the client connects but does not automate anything.

## Toliss A340-600 setup

The Toliss rejects fuel and payload written from outside, so the client runs a SimBrief uplink through the center MCDU instead. That takes some one-time setup in the EFB, plus the right GSX profile.

### Toliss EFB

Save your SimBrief ID in the SIMBRIEF OFP tab, then turn on both IGNORE AIRAC/AC TYPE MISMATCH and SET PAYLOAD + FUEL TO SIMBRIEF.

The client only counts the aircraft as powered when external power is feeding or the APU is available; batteries alone leave the MCDUs dark. Call the GPU from the Toliss EFB or start the APU (the GSX ground power unit does not power this aircraft). Once refueling starts with the fuel hose connected, the client presses the center MCDU keys for you (MENU, ATSU, AOC MENU, FLT INIT) and the aircraft pulls its fuel and payload from SimBrief. If the uplink does not land, trigger FLT INIT yourself on any MCDU and the flow continues.

### GSX profile

Community profiles from flightsim.to often ship with `refueling = 1` in their `gsx.cfg`, which makes the fuel truck park, pop a fuel quantity window and drive away without connecting the hose. This aircraft needs `refueling = 0`. The profile usually lives under `%APPDATA%\Virtuali\Airplanes\aerosoft-a340-600-pro`; the client checks the `gsx.cfg` files there and shows an advisory with a Fix profile button when one is wrong. GSX only picks the change up after you restart Couatl or reload the flight.

## PMDG 777 setup

The client reads the aircraft through the PMDG SDK broadcast, which is off from the factory. Open `777_Options.ini` under `%APPDATA%\Microsoft Flight Simulator 2024\WASM\MSFS2024\pmdg-aircraft-<variant>\work\` (one file per installed variant: `77w`, `77f`, `77l`, `77er`) and make sure it has:

```ini
[SDK]
EnableDataBroadcast=1
```

Edit it with the sim closed, or the aircraft rewrites the file on exit. Without the flag the client stays at "Waiting aircraft".

Fuel and payload go through the CommBus plugin, so on this aircraft it is required rather than recommended. The turnaround waits at "Waiting for flight plan" until you import your SimBrief OFP on the tablet's flight plan page or into the FMC.

## PMDG 737 setup

The client reads this aircraft through the same PMDG SDK broadcast as the 777, off from the factory here too. The 737 keeps one options file for the whole family: open `737_Options.ini` under `%APPDATA%\Microsoft Flight Simulator 2024\WASM\MSFS2024\pmdg-aircraft-738\work\` and make sure it has:

```ini
[SDK]
EnableDataBroadcast=1
```

Edit it with the sim closed, or the aircraft rewrites the file on exit. Without the flag the client stays at "Waiting aircraft" and shows an advisory with an Enable broadcast button that writes the line for you. Reload the flight after you apply it.

Fuel and payload go through the CommBus plugin, so on this aircraft it is required rather than recommended. On the BCF and BDSF the whole payload goes in as main deck cargo, and that door runs on hydraulics: with the electric pumps off, the open command sits armed and the door moves once pressure arrives.

## The CommBus plugin

Install the CommBus plugin (`gsx-integrator-commbus`) in your Community folder; the [`gsx-integrator-installer`](https://github.com/brunofgmag/gsx-integrator-installer) does that for you. The plugin is a small bridge between the client and the parts of the sim only a WASM module can reach. It carries the client's app on the EFB tablet in MSFS 2024, where you follow the turnaround and give the go-ahead without leaving the cockpit, and it lets the client open the GSX panel on the MSFS toolbar. On the PMDG aircraft it also carries the fuel and payload writes: without the plugin, version 0.2.0 or newer, the 777 and the 737 will not refuel or board. Other aircraft run without it; the one menu you would then have to open by hand is the pushback menu, where you pick where the tug leaves you.

## GSX settings worth changing

Four settings on the GSX Settings page are worth a visit before your first flight.

Turn Ignore Time on, in the Simulation area beside the SimBrief username. GSX ships with it off, which makes it reject any flight plan whose departure time has already passed, and the turnaround then waits at "Waiting for flight plan" until you dispatch again.

Turn Trust Simbrief passengers number on, in the same area. With it off, some aircraft push their own passenger count to GSX and it boards that number instead of the one you dispatched. With it on, GSX boards the OFP figure.

Leave Assistance Services "Auto" mode off. In Auto mode GSX calls its own services in sequence, which is what the client is already doing. Run both and you have two dispatchers arguing over one menu.

Set the interval between "Waiting for your action" messages to 25 seconds, in the Timings area. The default of 15 seconds is a lot of nagging when the client is opening the doors for you anyway; much longer than 25 and you stop noticing the times GSX is genuinely stuck.

## How to use it

1. Dispatch your flight in Simbrief.
2. Start the simulator and load your flight at a gate. Cold and dark or powered, either works as long as the engines are off.
3. Start the client. It connects to the sim and detects the aircraft on its own.
4. Enter your Simbrief ID in the settings the first time you run it.
5. Watch the phases in the main window. The client requests refueling and boarding from GSX with the planned figures from your OFP and moves through the turnaround as the aircraft becomes ready.

A few tips:

- Let the client drive the GSX menu. If you click through GSX menus manually mid-turnaround, the two of you will fight over it.
- Planned fuel and ZFW come from your Simbrief OFP, so dispatch before you board, not after.
- On the Fenix, the iFly and the Avro RJ, import your SimBrief plan in the aircraft's EFB; the turnaround waits at "Waiting for flight plan" until it is in. On the iFly, use the Balance & Payload page and load only the flight plan, not the weights.
- On the Avro RJ, the client asks GSX for the aircraft's own airstairs; at a jetway stand it boards through the finger instead.
- If nothing happens after you load in, check that you are flying one of the supported aircraft and that GSX itself is running normally.

## Problems and feedback

Open an issue on GitHub with what you were flying, what you expected, and what happened instead. While the project is in testing, reports from real flights are the most useful thing you can send.
