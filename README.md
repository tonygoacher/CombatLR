# CombatLR — GPS Combat Royale

CombatLR is a **GPS-based combat royale / laser-tag game system**. Players carry ESP32-powered game units that handle weapon firing, infrared hit detection, audio and game-state feedback, while a companion mobile app uses GPS and site maps to configure and control games across a real-world play area.

The system combines physical movement and GPS position with timed game zones, pickups, teams and player state. The ESP32 devices coordinate over a wireless network; the mobile app prepares game plans and manages connected units.

## Repository layout

| Path | Purpose |
| --- | --- |
| `Firmware/` | ESP32 firmware: game/state logic, networking, GPS/map coordinates, zones, weapons, pickups, audio, and display/graphics. |
| `CombatLR.mobile/` | .NET MAUI app for map/game-plan editing, GPS/location handling, device discovery, configuration, and game control. |
| `MapBuilder/` | Windows/WPF site-map preparation tool. |
| `CombatlrMapBuilder/` | MapBuilder project and documentation; check both directories before assuming the projects are identical. |
| `CombatLR_GpsSimulator/` | Windows/WPF tool for testing firmware GPS handling with simulated NMEA data. |
| `Resources/` | Example site maps and game-plan resources. |
| `Electronics/`, `DocsAndIdeas/` | Supporting electronics, notes, and design ideas. |

## System components

### ESP32 game unit

The firmware is in `Firmware/`. Its PlatformIO configuration is `Firmware/platformio.ini` and defines the `esp32s3_n16r8` environment for an ESP32-S3 board using Arduino and LittleFS.

The firmware includes game-state management, GPS/map conversion, zone timing, network commands, IR weapons and hit reception, pickups, audio, and the device display.

Build from the firmware directory with PlatformIO:

```sh
cd Firmware
pio run
```

This is the configured build entry point; a successful build has not been verified as part of this README update.

### Mobile app

The .NET MAUI app in `CombatLR.mobile/` is used to prepare and control games, including site maps, player/team assignment, game settings, and communication with ESP32 units.

Open `CombatLR.mobile.slnx` in a compatible Visual Studio installation. The project targets .NET 10 for Android and conditionally includes other platforms according to the operating system and installed workloads.

### Site-map calibration

The runtime map format carries a source-image crop and four GPS/image control points (A, B, C and D). Firmware uses these points to construct an affine transform between source-image pixels and local geographic east/north coordinates. The crop is then mapped to the exported 1280×960 runtime map. **Orientation is derived from the transform; a manually entered heading or legacy `RotationDegrees` value is not the calibration model.**

The map-building documentation must match the actual editor implementation. In the currently checked-in WPF editor source, the workflow still appears to use the older A/B calibration model; that source/documentation mismatch needs resolving before treating the editor as a verified four-point workflow.

### Network ports

The current mobile and firmware source defines UDP discovery on port **4210**, TCP command communication on port **4211**, and FTP on port **21**. Keep the mobile and firmware protocol definitions in sync when changing commands, fields, or identifier limits.

## Development workflow

For changes crossing the mobile/firmware boundary:

1. Inspect both sides of the protocol and the actual current source.
2. Agree the expected behaviour and edge cases.
3. Keep changes focused and preserve unrelated behaviour.
4. Build affected projects and run available tests.
5. Review the diff and verify hardware-dependent behaviour on real devices.

There is currently no checked-in GitHub Actions workflow. The commands and workflow notes here are guidance, not a claim that automated builds or hardware tests are already running.

## Project documentation

- [GPS Simulator guide](CombatLR_GpsSimulator/README.md)
- [MapBuilder guide](MapBuilder/README.md)
- [CombatlrMapBuilder guide](CombatlrMapBuilder/README.md)
- [Firmware navigation display production guide](Firmware/NavigationDisplayTest_ProductionGuide.md)

## Bug reports

Include the affected component, steps to reproduce, expected and actual behaviour, relevant logs, and whether the issue was reproduced on hardware. Do not include credentials, private keys, Wi-Fi passwords, or other secrets.
