# Changelog

## [1.19.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.18.0...v1.19.0) (2026-08-25)


* settle five debts found in the in-sim run ([#76](https://github.com/brunofgmag/gsx-integrator-client/issues/76)) ([34c1d1c](https://github.com/brunofgmag/gsx-integrator-client/commit/34c1d1cdcefcf6f0017d01250600bb11787d1256))


### Features

* **pmdg:** log every door command the reconciler sends ([ed50262](https://github.com/brunofgmag/gsx-integrator-client/commit/ed50262a61077fd38ec50158689a2015f81c251b)) ([34c1d1c](https://github.com/brunofgmag/gsx-integrator-client/commit/34c1d1cdcefcf6f0017d01250600bb11787d1256))


### Bug Fixes

* **build:** drop locked outputs and disable msbuild node reuse ([69cc423](https://github.com/brunofgmag/gsx-integrator-client/commit/69cc423d19b03ea3a54e444447c832643d7485cf)) ([34c1d1c](https://github.com/brunofgmag/gsx-integrator-client/commit/34c1d1cdcefcf6f0017d01250600bb11787d1256))
* **qml:** disable an action button by colour instead of opacity ([748c144](https://github.com/brunofgmag/gsx-integrator-client/commit/748c14485683e2dc2fe57a5b1b67bc4a5bd77300)) ([34c1d1c](https://github.com/brunofgmag/gsx-integrator-client/commit/34c1d1cdcefcf6f0017d01250600bb11787d1256))
* **tools:** prune debug artefacts and name what overflows the package ([b80b9b9](https://github.com/brunofgmag/gsx-integrator-client/commit/b80b9b98fa639b1fa0d62c2df9791e93186bf0f6)) ([34c1d1c](https://github.com/brunofgmag/gsx-integrator-client/commit/34c1d1cdcefcf6f0017d01250600bb11787d1256))

## [1.18.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.17.0...v1.18.0) (2026-08-25)


* accept the pilot touch from the EFB app with the phase it saw ([#74](https://github.com/brunofgmag/gsx-integrator-client/issues/74)) ([1a058a7](https://github.com/brunofgmag/gsx-integrator-client/commit/1a058a7c9aa0bdc5fe16125fcaef95fdb31ee750))


### Features

* **app:** guard the pilot touch behind connection, flow and phase stamp ([9ec7765](https://github.com/brunofgmag/gsx-integrator-client/commit/9ec7765ad9ab86546b297afc9e512c52588f6411)) ([1a058a7](https://github.com/brunofgmag/gsx-integrator-client/commit/1a058a7c9aa0bdc5fe16125fcaef95fdb31ee750))
* **efb:** publish the pilot touch button and act on its command ([5bccd63](https://github.com/brunofgmag/gsx-integrator-client/commit/5bccd638cf6b604d4a9cfac50221418c6723f7f1)) ([1a058a7](https://github.com/brunofgmag/gsx-integrator-client/commit/1a058a7c9aa0bdc5fe16125fcaef95fdb31ee750))
* **turnaround:** accept a pilot touch from the EFB app alongside the smart switch ([d9d35bb](https://github.com/brunofgmag/gsx-integrator-client/commit/d9d35bbd51b63314cedb0cfc73e60796c40f5e61)) ([1a058a7](https://github.com/brunofgmag/gsx-integrator-client/commit/1a058a7c9aa0bdc5fe16125fcaef95fdb31ee750))

## [1.17.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.16.0...v1.17.0) (2026-08-25)


* act on the four flow buttons from inside the EFB app ([#72](https://github.com/brunofgmag/gsx-integrator-client/issues/72)) ([08e68bc](https://github.com/brunofgmag/gsx-integrator-client/commit/08e68bc7920e6e6bba8ac2f8465a952e61d7933d))


### Features

* **efb:** decide each flow button permission once and publish it ([2b47dad](https://github.com/brunofgmag/gsx-integrator-client/commit/2b47dad54578d4d3ba302b92a33b020382098d21)) ([08e68bc](https://github.com/brunofgmag/gsx-integrator-client/commit/08e68bc7920e6e6bba8ac2f8465a952e61d7933d))
* **efb:** take the four flow commands back from the EFB app ([f550ef1](https://github.com/brunofgmag/gsx-integrator-client/commit/f550ef145271c62623e520f8b9ce53ceafbd90f9)) ([08e68bc](https://github.com/brunofgmag/gsx-integrator-client/commit/08e68bc7920e6e6bba8ac2f8465a952e61d7933d))


### Bug Fixes

* **efb:** publish a refused command so the EFB app shows the reason ([381fc5d](https://github.com/brunofgmag/gsx-integrator-client/commit/381fc5d0969a7e16002806dec56f204b36e71365)) ([08e68bc](https://github.com/brunofgmag/gsx-integrator-client/commit/08e68bc7920e6e6bba8ac2f8465a952e61d7933d))

## [1.16.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.15.0...v1.16.0) (2026-08-24)


* name the configured mode and whether it runs in the turnaround chip ([#70](https://github.com/brunofgmag/gsx-integrator-client/issues/70)) ([83f6d03](https://github.com/brunofgmag/gsx-integrator-client/commit/83f6d0349875a89407b709e078d71b4ff7e83b24))


### Features

* **efb:** name the configured mode and whether it runs in the turnaround chip ([3f46572](https://github.com/brunofgmag/gsx-integrator-client/commit/3f46572b0afdf7f7d33129918e0b52112e64b4e3)) ([83f6d03](https://github.com/brunofgmag/gsx-integrator-client/commit/83f6d0349875a89407b709e078d71b4ff7e83b24))

## [1.15.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.14.0...v1.15.0) (2026-08-24)


* publish the loading-mode flag the EFB app colours a chip with ([#68](https://github.com/brunofgmag/gsx-integrator-client/issues/68)) ([fea6a02](https://github.com/brunofgmag/gsx-integrator-client/commit/fea6a029fa52e8278f17e1ffe8bb01a6b957046c))


### Features

* **efb:** publish the loading-mode flag the EFB app colours a chip with ([7d56adb](https://github.com/brunofgmag/gsx-integrator-client/commit/7d56adb6da092b87587f4897dbdd8ae5e3a67bd9)) ([fea6a02](https://github.com/brunofgmag/gsx-integrator-client/commit/fea6a029fa52e8278f17e1ffe8bb01a6b957046c))

## [1.14.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.13.0...v1.14.0) (2026-08-24)


* keep the EFB app's view of the client in sync ([#66](https://github.com/brunofgmag/gsx-integrator-client/issues/66)) ([e325616](https://github.com/brunofgmag/gsx-integrator-client/commit/e325616a40697eccf8f734cdad35a9bb1f01c657))


### Features

* **efb:** answer the app and announce the client's departure ([704c864](https://github.com/brunofgmag/gsx-integrator-client/commit/704c8640b6610527601e68676f7e5e44aaf58c6b)) ([e325616](https://github.com/brunofgmag/gsx-integrator-client/commit/e325616a40697eccf8f734cdad35a9bb1f01c657))


### Bug Fixes

* **update:** find the bridge package in every Community folder ([de0addb](https://github.com/brunofgmag/gsx-integrator-client/commit/de0addb1ac1fd4b30727adf4a694eae93f2ab7bf)) ([e325616](https://github.com/brunofgmag/gsx-integrator-client/commit/e325616a40697eccf8f734cdad35a9bb1f01c657))

## [1.13.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.12.0...v1.13.0) (2026-08-24)


* move every operations screen phrase into the view model ([#64](https://github.com/brunofgmag/gsx-integrator-client/issues/64)) ([fa9a366](https://github.com/brunofgmag/gsx-integrator-client/commit/fa9a3662f0a3000c32d1b1365100850291342d0f))


### Features

* **efb:** move every operations screen phrase into the view model ([a02f5c5](https://github.com/brunofgmag/gsx-integrator-client/commit/a02f5c518a074057910279b5a76a27b213eea1b1)) ([fa9a366](https://github.com/brunofgmag/gsx-integrator-client/commit/fa9a3662f0a3000c32d1b1365100850291342d0f))


### Bug Fixes

* **ui:** redraw the operations screen when a display setting changes ([a685192](https://github.com/brunofgmag/gsx-integrator-client/commit/a6851929677ecee0fa0e9431569a3c934c478b8d)) ([fa9a366](https://github.com/brunofgmag/gsx-integrator-client/commit/fa9a3662f0a3000c32d1b1365100850291342d0f))

## [1.12.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.11.0...v1.12.0) (2026-08-24)


* publish the operations snapshot on the CommBus channel ([#62](https://github.com/brunofgmag/gsx-integrator-client/issues/62)) ([2e64738](https://github.com/brunofgmag/gsx-integrator-client/commit/2e647382f33d73c1252c87ce2a7245e38f9ef65b))


### Features

* **efb:** publish the operations snapshot on the CommBus channel ([8b621b5](https://github.com/brunofgmag/gsx-integrator-client/commit/8b621b5530b4e87adc09e05cba929a422ceec11a)) ([2e64738](https://github.com/brunofgmag/gsx-integrator-client/commit/2e647382f33d73c1252c87ce2a7245e38f9ef65b))


### Bug Fixes

* **turnaround:** hold chocks only for aircraft that can remove them ([7c312b6](https://github.com/brunofgmag/gsx-integrator-client/commit/7c312b62e37d1a66adceaf6ffe8d905ca911b8ea)) ([2e64738](https://github.com/brunofgmag/gsx-integrator-client/commit/2e647382f33d73c1252c87ce2a7245e38f9ef65b))

## [1.11.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.10.0...v1.11.0) (2026-08-23)


* let the pilot unlock the pushback gate with the smart switch ([#60](https://github.com/brunofgmag/gsx-integrator-client/issues/60)) ([98c2d88](https://github.com/brunofgmag/gsx-integrator-client/commit/98c2d88b70877e0188ca60707b5cc894d15383a7))
* pay the sim debts the last run closed ([#58](https://github.com/brunofgmag/gsx-integrator-client/issues/58)) ([c8e756e](https://github.com/brunofgmag/gsx-integrator-client/commit/c8e756e1e5a6487a88e52b2becf051567fdf3ec0))


### Features

* **tools:** a versioned tap for the gsx wire ([a2fd227](https://github.com/brunofgmag/gsx-integrator-client/commit/a2fd227c276f26793b65f70a76ff2427f78cc9de)) ([c8e756e](https://github.com/brunofgmag/gsx-integrator-client/commit/c8e756e1e5a6487a88e52b2becf051567fdf3ec0))
* **tools:** launch the client on the couatl that just restarted ([9d7aac8](https://github.com/brunofgmag/gsx-integrator-client/commit/9d7aac8f83fc1616c5f78731953cb67d784d7297)) ([c8e756e](https://github.com/brunofgmag/gsx-integrator-client/commit/c8e756e1e5a6487a88e52b2becf051567fdf3ec0))
* **turnaround:** unlock the pushback gate with the pilot's smart switch ([5ceacbd](https://github.com/brunofgmag/gsx-integrator-client/commit/5ceacbdd5705d72cb6ea5b73310be2e294fa84dc)) ([98c2d88](https://github.com/brunofgmag/gsx-integrator-client/commit/98c2d88b70877e0188ca60707b5cc894d15383a7))
* **ui:** mark a phase the pilot unlocked ([90b4985](https://github.com/brunofgmag/gsx-integrator-client/commit/90b49858c2498f64845f24b8b565e0255535f8bd)) ([98c2d88](https://github.com/brunofgmag/gsx-integrator-client/commit/98c2d88b70877e0188ca60707b5cc894d15383a7))


### Bug Fixes

* a rearmed request keeps its attempts and a stuck menu gets closed ([de88215](https://github.com/brunofgmag/gsx-integrator-client/commit/de88215701e54b110878e935439adbae3a677422)) ([c8e756e](https://github.com/brunofgmag/gsx-integrator-client/commit/c8e756e1e5a6487a88e52b2becf051567fdf3ec0))
* drop the socket that connects and never answers ([123e9bc](https://github.com/brunofgmag/gsx-integrator-client/commit/123e9bc4de461b0bfe0bbfd27a39822090cced07)) ([c8e756e](https://github.com/brunofgmag/gsx-integrator-client/commit/c8e756e1e5a6487a88e52b2becf051567fdf3ec0))
* **i18n:** the pushback prompt names the brake the gate already required ([b8f5714](https://github.com/brunofgmag/gsx-integrator-client/commit/b8f57148585669eb22807444a8e16565539960fb)) ([c8e756e](https://github.com/brunofgmag/gsx-integrator-client/commit/c8e756e1e5a6487a88e52b2becf051567fdf3ec0))
* observe the couatl restart from outside the gate it closes ([9f3999e](https://github.com/brunofgmag/gsx-integrator-client/commit/9f3999e1e9cebb1389d908d02ab63ddbe2e56167)) ([c8e756e](https://github.com/brunofgmag/gsx-integrator-client/commit/c8e756e1e5a6487a88e52b2becf051567fdf3ec0))
* the boarding completion asks again when the first ask is swallowed ([761052c](https://github.com/brunofgmag/gsx-integrator-client/commit/761052c4ec93c939abd0322298fc0836c743fc53)) ([c8e756e](https://github.com/brunofgmag/gsx-integrator-client/commit/c8e756e1e5a6487a88e52b2becf051567fdf3ec0))
* the smart switch counts only the side that springs back ([a00a189](https://github.com/brunofgmag/gsx-integrator-client/commit/a00a189670cfc42b7957642b446169a37370ea37)) ([c8e756e](https://github.com/brunofgmag/gsx-integrator-client/commit/c8e756e1e5a6487a88e52b2becf051567fdf3ec0))
* the smart switch throws away the span a paused sim stretched ([b02320a](https://github.com/brunofgmag/gsx-integrator-client/commit/b02320a66dd7b14ee3737cf02c4b1135bd2a0083)) ([c8e756e](https://github.com/brunofgmag/gsx-integrator-client/commit/c8e756e1e5a6487a88e52b2becf051567fdf3ec0))

## [1.10.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.9.0...v1.10.0) (2026-08-22)


* pay the implementation debts the in-sim session left open ([#56](https://github.com/brunofgmag/gsx-integrator-client/issues/56)) ([756b5ab](https://github.com/brunofgmag/gsx-integrator-client/commit/756b5ab332c8fc991948cdae90764dabbed2426f))


### Features

* **turnaround:** gate pushback on the beacon, the brake and every door ([b40d22f](https://github.com/brunofgmag/gsx-integrator-client/commit/b40d22f7f797270d3fe0da24baab5d397d42a30d)) ([756b5ab](https://github.com/brunofgmag/gsx-integrator-client/commit/756b5ab332c8fc991948cdae90764dabbed2426f))


### Bug Fixes

* **gsx:** distrust vehicle state inherited from before a Couatl restart ([45f21fc](https://github.com/brunofgmag/gsx-integrator-client/commit/45f21fc32f644aceb93ea41b4a5dafda191c23d3)) ([756b5ab](https://github.com/brunofgmag/gsx-integrator-client/commit/756b5ab332c8fc991948cdae90764dabbed2426f))
* **pmdg:** derive each cargo step from the EFB weight echo ([da95358](https://github.com/brunofgmag/gsx-integrator-client/commit/da95358eb34a9765e45f632053da203c8ac07ce2)) ([756b5ab](https://github.com/brunofgmag/gsx-integrator-client/commit/756b5ab332c8fc991948cdae90764dabbed2426f))
* **pmdg:** let the ground connection desire die and release the own stairs ([cfd10b8](https://github.com/brunofgmag/gsx-integrator-client/commit/cfd10b82bdb12082b488562098886a22869671d6)) ([756b5ab](https://github.com/brunofgmag/gsx-integrator-client/commit/756b5ab332c8fc991948cdae90764dabbed2426f))
* **pmdg:** wait one subscription period before the first light-test kick ([3d265ae](https://github.com/brunofgmag/gsx-integrator-client/commit/3d265ae18191681ec9fdfbfecd069b062b7e2644)) ([756b5ab](https://github.com/brunofgmag/gsx-integrator-client/commit/756b5ab332c8fc991948cdae90764dabbed2426f))
* **turnaround:** ask GSX to complete a boarding stalled at one hundred ([923ed5c](https://github.com/brunofgmag/gsx-integrator-client/commit/923ed5c5b4163be7ba1fa0caf84ad5a6ce04dd53)) ([756b5ab](https://github.com/brunofgmag/gsx-integrator-client/commit/756b5ab332c8fc991948cdae90764dabbed2426f))
* **turnaround:** hold the chocks while the parking brake is released ([75ac87f](https://github.com/brunofgmag/gsx-integrator-client/commit/75ac87f3a9c1b17507ffc40cf650d8a7772301b1)) ([756b5ab](https://github.com/brunofgmag/gsx-integrator-client/commit/756b5ab332c8fc991948cdae90764dabbed2426f))
* **turnaround:** warn when GSX takes the fuel request and no truck arrives ([35c9c68](https://github.com/brunofgmag/gsx-integrator-client/commit/35c9c68f57960fb7dcdd05210207aad1dc2cd706)) ([756b5ab](https://github.com/brunofgmag/gsx-integrator-client/commit/756b5ab332c8fc991948cdae90764dabbed2426f))

## [1.9.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.8.1...v1.9.0) (2026-08-22)


* the defects an in-sim session found, starting with an app that would not open ([#52](https://github.com/brunofgmag/gsx-integrator-client/issues/52)) ([ed6957f](https://github.com/brunofgmag/gsx-integrator-client/commit/ed6957f148a785a6f98fc65d79d23a0725ed94de))


### Features

* **probe:** write one arbitrary LVar, armed on a value ([ed6957f](https://github.com/brunofgmag/gsx-integrator-client/commit/ed6957f148a785a6f98fc65d79d23a0725ed94de))


### Bug Fixes

* **gsx:** reload the SimBrief plan on the GSX side too ([ed6957f](https://github.com/brunofgmag/gsx-integrator-client/commit/ed6957f148a785a6f98fc65d79d23a0725ed94de))
* **pmdg:** subscribe the 777 block by second instead of kicking the cockpit ([ed6957f](https://github.com/brunofgmag/gsx-integrator-client/commit/ed6957f148a785a6f98fc65d79d23a0725ed94de))
* **qml:** stop the test module from shadowing the app's QML module ([ed6957f](https://github.com/brunofgmag/gsx-integrator-client/commit/ed6957f148a785a6f98fc65d79d23a0725ed94de))
* **turnaround:** wait past ten minutes for a cabin service still running ([ed6957f](https://github.com/brunofgmag/gsx-integrator-client/commit/ed6957f148a785a6f98fc65d79d23a0725ed94de))
* **ui:** tell a GSX refusal from a ready SimBrief plan on the card ([ed6957f](https://github.com/brunofgmag/gsx-integrator-client/commit/ed6957f148a785a6f98fc65d79d23a0725ed94de))

## [1.8.1](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.8.0...v1.8.1) (2026-08-21)


* publish the release from main with its assets ([#50](https://github.com/brunofgmag/gsx-integrator-client/issues/50)) ([a652ffc](https://github.com/brunofgmag/gsx-integrator-client/commit/a652ffc99d46106ced0446219f9fb62c3d2acf51))

## [1.8.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.7.0...v1.8.0) (2026-08-21)


* pay the debts that only needed code or a decision ([#48](https://github.com/brunofgmag/gsx-integrator-client/issues/48)) ([df5ae1c](https://github.com/brunofgmag/gsx-integrator-client/commit/df5ae1c98c76d650adffda2ab782324eb5afa714))


### Features

* **aircraft:** clear the ground equipment an aircraft parks by itself ([df5ae1c](https://github.com/brunofgmag/gsx-integrator-client/commit/df5ae1c98c76d650adffda2ab782324eb5afa714))
* **aircraft:** read the doors the fleet could not answer ([df5ae1c](https://github.com/brunofgmag/gsx-integrator-client/commit/df5ae1c98c76d650adffda2ab782324eb5afa714))
* **gsx:** observe the state once a tick and report what GSX refuses ([df5ae1c](https://github.com/brunofgmag/gsx-integrator-client/commit/df5ae1c98c76d650adffda2ab782324eb5afa714))
* let the probe watch new variables and put the QML components under test ([df5ae1c](https://github.com/brunofgmag/gsx-integrator-client/commit/df5ae1c98c76d650adffda2ab782324eb5afa714))
* **turnaround:** hold the doors closed once the boarding ends ([df5ae1c](https://github.com/brunofgmag/gsx-integrator-client/commit/df5ae1c98c76d650adffda2ab782324eb5afa714))


### Bug Fixes

* **aircraft:** tell "held in place" from "the pilot set the brake" ([df5ae1c](https://github.com/brunofgmag/gsx-integrator-client/commit/df5ae1c98c76d650adffda2ab782324eb5afa714))
* **ci:** resume, retry and cache the MSFS SDK download ([df5ae1c](https://github.com/brunofgmag/gsx-integrator-client/commit/df5ae1c98c76d650adffda2ab782324eb5afa714))
* **fenix:** believe a closed door only when the reading settles ([df5ae1c](https://github.com/brunofgmag/gsx-integrator-client/commit/df5ae1c98c76d650adffda2ab782324eb5afa714))
* **gsx:** confer the GSX profile of the PMDG family too ([df5ae1c](https://github.com/brunofgmag/gsx-integrator-client/commit/df5ae1c98c76d650adffda2ab782324eb5afa714))
* **pmdg:** describe the smart switch press as deviation from the named neutral ([df5ae1c](https://github.com/brunofgmag/gsx-integrator-client/commit/df5ae1c98c76d650adffda2ab782324eb5afa714))
* **pmdg:** hold the payload trim while the progressive ramp is moving ([df5ae1c](https://github.com/brunofgmag/gsx-integrator-client/commit/df5ae1c98c76d650adffda2ab782324eb5afa714))
* **pmdg:** stop reading a door jammed mid-travel as unknown ([df5ae1c](https://github.com/brunofgmag/gsx-integrator-client/commit/df5ae1c98c76d650adffda2ab782324eb5afa714))
* **pmdg:** tell a live SDK block from a frozen one ([df5ae1c](https://github.com/brunofgmag/gsx-integrator-client/commit/df5ae1c98c76d650adffda2ab782324eb5afa714))

## [1.7.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.6.0...v1.7.0) (2026-08-21)


### Features

* **probe:** instrument the client to answer what only the simulator can ([#45](https://github.com/brunofgmag/gsx-integrator-client/issues/45)) ([a7ba758](https://github.com/brunofgmag/gsx-integrator-client/commit/a7ba75811937961eb2f02e295b18a4189126632b))

## [1.6.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.5.0...v1.6.0) (2026-08-19)


### Features

* **aircraft:** let every aircraft report whether a door is open ([#43](https://github.com/brunofgmag/gsx-integrator-client/issues/43)) ([f648ce7](https://github.com/brunofgmag/gsx-integrator-client/commit/f648ce7e5b187de4d0ab0bc4a1f453778dd9219c))

## [1.5.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.4.0...v1.5.0) (2026-08-18)


### Features

* iFly plan gate, GSX menu retry rework e correções de porta de carga do 737 ([#38](https://github.com/brunofgmag/gsx-integrator-client/issues/38)) ([55a5700](https://github.com/brunofgmag/gsx-integrator-client/commit/55a570054bd3bca35fb97e8cb7728b989a3ab9bb))

## [1.4.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.3.0...v1.4.0) (2026-08-17)


### Features

* let the user choose the graphics backend ([#36](https://github.com/brunofgmag/gsx-integrator-client/issues/36)) ([1d866e3](https://github.com/brunofgmag/gsx-integrator-client/commit/1d866e37e031b7c8cfc34b7ba73ca22fb0082162))

## [1.3.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.2.0...v1.3.0) (2026-08-16)


### Features

* add PMDG 737 family support ([618d71e](https://github.com/brunofgmag/gsx-integrator-client/commit/618d71e24e90e2c860e27136239c7731b9318ec1))
* add PMDG 737 family support ([#34](https://github.com/brunofgmag/gsx-integrator-client/issues/34)) ([d47ad91](https://github.com/brunofgmag/gsx-integrator-client/commit/d47ad91d629a7185d2feb6be0a3ab4b32ec7b077))
* offer to enable the PMDG SDK data broadcast ([9847a60](https://github.com/brunofgmag/gsx-integrator-client/commit/9847a6091e0eadc849f4f22d2a52765382f13b29))

## [1.2.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.1.0...v1.2.0) (2026-07-22)


### Features

* add PMDG 777 ([#30](https://github.com/brunofgmag/gsx-integrator-client/issues/30)) ([837b109](https://github.com/brunofgmag/gsx-integrator-client/commit/837b10923847131cda28bea636aba421b17ec14e))

## [1.1.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v1.0.0...v1.1.0) (2026-07-20)


### Features

* add fenix support ([#27](https://github.com/brunofgmag/gsx-integrator-client/issues/27)) ([0cafe1a](https://github.com/brunofgmag/gsx-integrator-client/commit/0cafe1a441ce8d85f858f07d3cebee9e8879605e))

## [1.0.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v0.11.0...v1.0.0) (2026-07-19)


### ⚠ BREAKING CHANGES

* major client refactor ([#23](https://github.com/brunofgmag/gsx-integrator-client/issues/23))

### Features

* major client refactor ([#23](https://github.com/brunofgmag/gsx-integrator-client/issues/23)) ([6157401](https://github.com/brunofgmag/gsx-integrator-client/commit/6157401836c35eb98977a2eacf56695ca985df6a))

## [0.11.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v0.10.0...v0.11.0) (2026-07-12)


### Features

* add support for toliss a340-600 ([#20](https://github.com/brunofgmag/gsx-integrator-client/issues/20)) ([e53a991](https://github.com/brunofgmag/gsx-integrator-client/commit/e53a9919bb6143a29f98c51407b946d44f1d102d))

## [0.10.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v0.9.0...v0.10.0) (2026-07-11)


### Features

* add ifly 737 ([#17](https://github.com/brunofgmag/gsx-integrator-client/issues/17)) ([0a7117e](https://github.com/brunofgmag/gsx-integrator-client/commit/0a7117e8a2922b38c19c3ab0b6c605cce955a6bc))

## [0.9.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v0.8.0...v0.9.0) (2026-07-10)


### Features

* add tray icon support ([#13](https://github.com/brunofgmag/gsx-integrator-client/issues/13)) ([4b28ae1](https://github.com/brunofgmag/gsx-integrator-client/commit/4b28ae1a072261adc07f6346259843ac4d97ff29))
* auto-update ([c932266](https://github.com/brunofgmag/gsx-integrator-client/commit/c9322664a109405bfc616c56e90d20f2c9dbe1c9))


### Bug Fixes

* broken test ([ad96b47](https://github.com/brunofgmag/gsx-integrator-client/commit/ad96b475a388966bb98e96c9fd59664b62d5445b))
* change windows version on actions runners ([76dc2de](https://github.com/brunofgmag/gsx-integrator-client/commit/76dc2de2183553b184f60c2f71f1988b09dc6b5e))
* fix release-please permissions ([2a33a3f](https://github.com/brunofgmag/gsx-integrator-client/commit/2a33a3f9d99f298cf89aaf3c4bfc9dd10f8591ea))
* update pipelines to handle release-please PRs ([5662865](https://github.com/brunofgmag/gsx-integrator-client/commit/56628659c49614ed6ced72358206d0f8b3f435ba))
* update script ([fcdb78b](https://github.com/brunofgmag/gsx-integrator-client/commit/fcdb78b5b407c7535d4e7fd233a7d8a36b24fa71))

## [0.8.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v0.7.0...v0.8.0) (2026-07-10)


### Features

* add tray icon support ([#13](https://github.com/brunofgmag/gsx-integrator-client/issues/13)) ([4b28ae1](https://github.com/brunofgmag/gsx-integrator-client/commit/4b28ae1a072261adc07f6346259843ac4d97ff29))
* auto-update ([c932266](https://github.com/brunofgmag/gsx-integrator-client/commit/c9322664a109405bfc616c56e90d20f2c9dbe1c9))


### Bug Fixes

* broken test ([ad96b47](https://github.com/brunofgmag/gsx-integrator-client/commit/ad96b475a388966bb98e96c9fd59664b62d5445b))
* change windows version on actions runners ([76dc2de](https://github.com/brunofgmag/gsx-integrator-client/commit/76dc2de2183553b184f60c2f71f1988b09dc6b5e))
* fix release-please permissions ([2a33a3f](https://github.com/brunofgmag/gsx-integrator-client/commit/2a33a3f9d99f298cf89aaf3c4bfc9dd10f8591ea))
* update pipelines to handle release-please PRs ([5662865](https://github.com/brunofgmag/gsx-integrator-client/commit/56628659c49614ed6ced72358206d0f8b3f435ba))
* update script ([fcdb78b](https://github.com/brunofgmag/gsx-integrator-client/commit/fcdb78b5b407c7535d4e7fd233a7d8a36b24fa71))

## [0.7.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v0.6.0...v0.7.0) (2026-07-10)


### Features

* auto-update ([c932266](https://github.com/brunofgmag/gsx-integrator-client/commit/c9322664a109405bfc616c56e90d20f2c9dbe1c9))


### Bug Fixes

* broken test ([ad96b47](https://github.com/brunofgmag/gsx-integrator-client/commit/ad96b475a388966bb98e96c9fd59664b62d5445b))
* change windows version on actions runners ([76dc2de](https://github.com/brunofgmag/gsx-integrator-client/commit/76dc2de2183553b184f60c2f71f1988b09dc6b5e))
* fix release-please permissions ([2a33a3f](https://github.com/brunofgmag/gsx-integrator-client/commit/2a33a3f9d99f298cf89aaf3c4bfc9dd10f8591ea))
* update pipelines to handle release-please PRs ([5662865](https://github.com/brunofgmag/gsx-integrator-client/commit/56628659c49614ed6ced72358206d0f8b3f435ba))
* update script ([fcdb78b](https://github.com/brunofgmag/gsx-integrator-client/commit/fcdb78b5b407c7535d4e7fd233a7d8a36b24fa71))

## [0.6.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v0.5.0...v0.6.0) (2026-07-10)


### Features

* auto-update ([c932266](https://github.com/brunofgmag/gsx-integrator-client/commit/c9322664a109405bfc616c56e90d20f2c9dbe1c9))


### Bug Fixes

* broken test ([ad96b47](https://github.com/brunofgmag/gsx-integrator-client/commit/ad96b475a388966bb98e96c9fd59664b62d5445b))
* change windows version on actions runners ([76dc2de](https://github.com/brunofgmag/gsx-integrator-client/commit/76dc2de2183553b184f60c2f71f1988b09dc6b5e))
* fix release-please permissions ([2a33a3f](https://github.com/brunofgmag/gsx-integrator-client/commit/2a33a3f9d99f298cf89aaf3c4bfc9dd10f8591ea))
* update pipelines to handle release-please PRs ([5662865](https://github.com/brunofgmag/gsx-integrator-client/commit/56628659c49614ed6ced72358206d0f8b3f435ba))
* update script ([fcdb78b](https://github.com/brunofgmag/gsx-integrator-client/commit/fcdb78b5b407c7535d4e7fd233a7d8a36b24fa71))

## [0.5.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v0.4.0...v0.5.0) (2026-07-10)


### Features

* auto-update ([c932266](https://github.com/brunofgmag/gsx-integrator-client/commit/c9322664a109405bfc616c56e90d20f2c9dbe1c9))


### Bug Fixes

* broken test ([ad96b47](https://github.com/brunofgmag/gsx-integrator-client/commit/ad96b475a388966bb98e96c9fd59664b62d5445b))
* change windows version on actions runners ([76dc2de](https://github.com/brunofgmag/gsx-integrator-client/commit/76dc2de2183553b184f60c2f71f1988b09dc6b5e))
* fix release-please permissions ([2a33a3f](https://github.com/brunofgmag/gsx-integrator-client/commit/2a33a3f9d99f298cf89aaf3c4bfc9dd10f8591ea))
* update pipelines to handle release-please PRs ([5662865](https://github.com/brunofgmag/gsx-integrator-client/commit/56628659c49614ed6ced72358206d0f8b3f435ba))
* update script ([fcdb78b](https://github.com/brunofgmag/gsx-integrator-client/commit/fcdb78b5b407c7535d4e7fd233a7d8a36b24fa71))

## [0.4.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v0.3.0...v0.4.0) (2026-07-09)


### Features

* auto-update ([c932266](https://github.com/brunofgmag/gsx-integrator-client/commit/c9322664a109405bfc616c56e90d20f2c9dbe1c9))


### Bug Fixes

* broken test ([ad96b47](https://github.com/brunofgmag/gsx-integrator-client/commit/ad96b475a388966bb98e96c9fd59664b62d5445b))
* change windows version on actions runners ([76dc2de](https://github.com/brunofgmag/gsx-integrator-client/commit/76dc2de2183553b184f60c2f71f1988b09dc6b5e))
* fix release-please permissions ([2a33a3f](https://github.com/brunofgmag/gsx-integrator-client/commit/2a33a3f9d99f298cf89aaf3c4bfc9dd10f8591ea))
* update pipelines to handle release-please PRs ([5662865](https://github.com/brunofgmag/gsx-integrator-client/commit/56628659c49614ed6ced72358206d0f8b3f435ba))
* update script ([fcdb78b](https://github.com/brunofgmag/gsx-integrator-client/commit/fcdb78b5b407c7535d4e7fd233a7d8a36b24fa71))

## [0.3.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v0.2.0...v0.3.0) (2026-07-09)


### Features

* auto-update ([c932266](https://github.com/brunofgmag/gsx-integrator-client/commit/c9322664a109405bfc616c56e90d20f2c9dbe1c9))


### Bug Fixes

* broken test ([ad96b47](https://github.com/brunofgmag/gsx-integrator-client/commit/ad96b475a388966bb98e96c9fd59664b62d5445b))
* change windows version on actions runners ([76dc2de](https://github.com/brunofgmag/gsx-integrator-client/commit/76dc2de2183553b184f60c2f71f1988b09dc6b5e))
* fix release-please permissions ([2a33a3f](https://github.com/brunofgmag/gsx-integrator-client/commit/2a33a3f9d99f298cf89aaf3c4bfc9dd10f8591ea))
* update pipelines to handle release-please PRs ([5662865](https://github.com/brunofgmag/gsx-integrator-client/commit/56628659c49614ed6ced72358206d0f8b3f435ba))

## [0.2.0](https://github.com/brunofgmag/gsx-integrator-client/compare/v0.1.1...v0.2.0) (2026-07-09)


### Features

* auto-update ([c932266](https://github.com/brunofgmag/gsx-integrator-client/commit/c9322664a109405bfc616c56e90d20f2c9dbe1c9))

## [0.1.1](https://github.com/brunofgmag/gsx-integrator-client/compare/v0.1.0...v0.1.1) (2026-07-07)


### Bug Fixes

* broken test ([ad96b47](https://github.com/brunofgmag/gsx-integrator-client/commit/ad96b475a388966bb98e96c9fd59664b62d5445b))
* change windows version on actions runners ([76dc2de](https://github.com/brunofgmag/gsx-integrator-client/commit/76dc2de2183553b184f60c2f71f1988b09dc6b5e))
