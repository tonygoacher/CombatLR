# CombatLR

CombatLR is a GPS-based combat royale / laser-tag system built around ESP32 game units and a companion .NET MAUI mobile application. The game units handle infrared firing and hit detection, audio, game state and display output. The mobile app is used to prepare game plans, work with site maps, discover and configure units, and control games.

This README describes the repository as it currently exists. Refer to the individual project files for implementation and build details.

## Repository layout

| Path | Purpose |
| --- | --- |
| `Firmware/` | ESP32-S3 firmware, built with PlatformIO and Arduino. Includes game state, timing and zones, GPS/map conversion, networking, weapons, pickups, audio, and display/graphics. |
| `CombatLR.mobile/` | .NET MAUI mobile app for site maps, game-plan editing, player/team setup, device discovery and game control. |
| `CombatlrMapBuilder/` | Windows/WPF tool for calibrating a site map and exporting the runtime map package. |
| `CombatLR_GpsSimulator/` | Windows/WPF tool for supplying simulated NMEA GPS data during firmware development and testing. |
| `Electronics/`, `DocsAndIdeas/` | Supporting hardware material, notes and design ideas. |

The obsolete `MapBuilder/` project has been removed. `CombatlrMapBuilder/` is the current map-building application.

## Firmware

The firmware project is in `Firmware/`. Its PlatformIO configuration is `Firmware/platformio.ini`, with the `esp32s3_n16r8` environment for the ESP32-S3 and LittleFS.

Build from the firmware directory with PlatformIO CLI installed:

```sh
cd Firmware
pio run
```

This documents the configured build entry point; it does not imply that a build or hardware test was run as part of this README update.

## Mobile application

The MAUI project is in `CombatLR.mobile/`. Open `CombatLR.mobile.slnx` in a compatible Visual Studio installation. The project targets .NET 10 for Android and may include additional targets according to the operating system and installed workloads.

A command-line build can be attempted with the .NET SDK and required workloads installed:

```sh
dotnet build CombatLR.mobile/CombatLR.mobile.csproj
```

## Site-map calibration

The current map-building tool is `CombatlrMapBuilder/`. Its calibration uses four image control points, A, B, C and D, each associated with a GPS coordinate. The tool fits an affine transform between source-image pixels and local geographic east/north coordinates.

The selected playfield is exported as a **1280 × 960 RGB565 BMP** together with `map.json`. The JSON contains the source-image crop, playfield geographic coordinates, metres-per-pixel information and the four calibration reference points. Firmware consumes this metadata to relate map pixels to geographic positions. Orientation is derived from the calibration transform; a manually entered rotation angle is not the current calibration workflow.

See [the CombatlrMapBuilder project guide](CombatlrMapBuilder/README.md) for tool-specific notes.

## Network ports

The current mobile and firmware source uses:

- **UDP 4210** for device discovery
- **TCP 4211** for commands
- **FTP 21** for file transfer

Keep mobile and firmware protocol definitions in sync when changing message fields, commands or identifier limits.

## GPS simulator

The `CombatLR_GpsSimulator/` project is a development utility for sending simulated NMEA GPS data to an ESP32 over a serial connection. See its [project guide](CombatLR_GpsSimulator/README.md) for usage details.

## Development workflow

For changes that cross the mobile/firmware boundary:

1. Inspect the relevant code on both sides of the protocol.
2. Agree the expected behaviour and edge cases.
3. Keep changes focused and preserve unrelated behaviour.
4. Build the affected project and run available tests.
5. Review the diff and verify hardware-dependent behaviour on real devices.

Do not assume that a successful desktop build verifies GPS reception, radio communication, infrared operation or other hardware-dependent behaviour.

## Reporting problems

Include the affected component, steps to reproduce, expected and actual behaviour, relevant logs, and whether the problem was reproduced on hardware. Do not include credentials, private keys, Wi-Fi passwords or other secrets.
