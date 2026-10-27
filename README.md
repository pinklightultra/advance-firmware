# picoTracker Advance Firmware

This repository holds source code releases of the picoTracker Advance firmware. The firmware started as a modified version of [LittleGPTracker](https://littlegptracker.com/) but has now diverged in many areas and added alot of new and improved functionality. It implements a user interface similar to the refined track-by-joypad software [*littlesounddj*](http://www.littlesounddj.com/).

Each new stable release is added as a snapshot commit to this repository.

Please start a new Discussion to report a bug or submit a feature request.

##  Modified Firmware Disclaimer

The source code in this repository is provided for informational and educational
purposes for picoTracker Advance owners.

You may use it to build modified ("third-party") firmware for the Advance at your
own risk. Third-party firmware is not recommended, certified, or supported by
xiphonics, and is maintained solely by its publisher. Anyone who builds, installs,
or uses third-party firmware — whether the publisher or any Advance owner — does so
entirely at their own risk and accepts full responsibility for any loss of data,
damage to the Advance, or damage to devices connected to or operated with it.
xiphonics does not certify, or support third-party firmware.


## Firmware Features

* Sample, Synth and MIDI instruments
* MIDI control surfaces
* MIDI clock and transport
* 8 channel mixer
* Master Reverb and Delay effects
* Custom UI colour themes
* Remote UI Display via USB

Per project:
* 8 song channels
* 255 chains
* 255 phrases
* 32 tables
* 64 instruments
* 256 samples

The Advance uses a STM32 microcontroller and has the following hardware:

* Headphone/Lineout
* TRS MIDI In & Out, USB Device and Host MIDI In/Out
* 720x720 4in LCD display
* 64MB of Ram
* MicroSD card slot for project & sample library storage and firmware upgrades 
* USB-C for Device and Host MIDI and charging

## Advance

The picoTracker Advance model was launched in November 2025 and has significantly upgraded hardware over the original RP2040 based "pico v2" model including hi-dpi screen, line-input, internal mic and a speaker. See [here for more information](https://xiphonics.com/products/picotracker).

## MANUAL

Head over to the [User Manual](https://manual.xiphonics.com/)

## Remote UI

The [remote UI client app is available here](https://ui.xiphonics.com/) and its [git repo is here](https://github.com/xiphonics/picotracker_client).

## Get involved!

Discussion of the Advance and its firmware happens on [Discord](https://discord.gg/F9nhkd7qj2)

## Development

See [the development docs](docs/DEV.md).

## Development & Testing

### Doctest C++ Test Suite
The firmware includes a comprehensive unit testing suite using `doctest` verifying all instrument voice pipelines, MIDI routing, phrase sequencing, and table processing:

```bash
cmake -B build -S . -G Ninja -DBUILD_TESTING=ON
cmake --build build --target tests
./build/tests
```
> All **502 test cases and 144,919 assertions** execute in ~2.5 seconds on host systems.

### Automated CI
Continuous integration runs on GitHub Actions on every commit to `main`, validating the build matrix, header dependencies, and test suite execution.
