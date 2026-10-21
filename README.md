# CombatLR

CombatLR is an ESP32-based laser-tag game system with a companion .NET MAUI mobile app for configuring and controlling games. The repository also contains map-building tools and a GPS simulator to support site setup and development.

This README is a high-level entry point. See the project-specific documentation and configuration files for implementation details.

## Repository layout

| Path | Purpose |
| --- | --- |
| `Firmware/` | ESP32 firmware built with PlatformIO and the Arduino framework. Includes game/state logic, networking, GPS/navigation, zones, weapons, pickups, and display/graphics code. |
| `CombatLR.mobile/` | .NET MAUI application for game-plan editing, map/coordinate handling, device discovery, and game control. |
| `CombatLR_GpsSimulator/` | Windows/WPF tool for sending simulated NMEA GPS data to an ESP32 over a serial connection. |
| `CombatlrMapBuilder/` | Map-building tool with project documentation in its directory. |
| `MapBuilder/` | MapBuilder project and documentation. Check the project files before assuming this directory and `CombatlrMapBuilder/` are interchangeable. |
| `Resources/` | Example map and game-plan resources. |
| `Electronics/`, `DocsAndIdeas/` | Supporting electronics material, notes, and design ideas. |

## Main components

### ESP32 firmware

The firmware project is in `Firmware/`. Its PlatformIO configuration is `Firmware/platformio.ini` and currently defines the `esp32s3_n16r8` environment for an ESP32-S3 board using the Arduino framework and LittleFS.

The source is separated into areas including:

- State machine and game control
- Game plans, zones, and timing
- GPS/navigation and map coordinates
- Network commands and device discovery
- Weapons and pickups
- Display management, graphics, and widgets

Build from the `Firmware/` directory with PlatformIO:

```sh
pio run
```

This command assumes PlatformIO CLI is installed and available on your PATH. A successful build has not yet been confirmed by this README.

### Mobile application

The MAUI application is in `CombatLR.mobile/`. Open `CombatLR.mobile.slnx` in a compatible version of Visual Studio, or build the project with the .NET SDK installed:

```sh
dotnet build CombatLR.mobile/CombatLR.mobile.csproj
```

The project currently targets .NET 10 for Android and conditionally adds other platform targets depending on the build operating system and installed workloads. Platform-specific builds may therefore require the corresponding SDKs and workloads.

### GPS simulator and map tools

The GPS simulator and map-building projects are separate Windows desktop applications. Their directories contain README files with usage notes and requirements. Read those instructions before building or using the tools.

## Network protocol notes

The current mobile and firmware source defines UDP discovery on port **4210** and TCP command communication on port **4211**. FTP uses port **21**. Keep the mobile and firmware protocol definitions in sync when changing commands, fields, or identifier limits.

## Development workflow

For changes that cross the mobile/firmware boundary:

1. Inspect both sides of the protocol before editing.
2. Agree expected behaviour and edge cases.
3. Keep changes focused and preserve unrelated behaviour.
4. Build the affected project(s) and run available tests.
5. Review the diff and test hardware-dependent behaviour on the actual device.

The repository currently has no checked-in GitHub Actions workflow. Build commands above are suggested entry points based on the project configuration; they are not a claim that automated builds or hardware tests are already running.

## Project documentation

- [GPS Simulator guide](CombatLR_GpsSimulator/README.md)
- [CombatlrMapBuilder guide](CombatlrMapBuilder/README.md)
- [MapBuilder guide](MapBuilder/README.md)
- [Firmware navigation display production guide](Firmware/NavigationDisplayTest_ProductionGuide.md)

## Contributions and issue reports

When reporting a problem, include the affected component, steps to reproduce, expected and actual behaviour, relevant logs, and whether the issue was reproduced on hardware. Do not include credentials, private keys, Wi-Fi passwords, or other secrets in issues or commits.
