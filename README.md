# CombatLR

CombatLR is a GPS-based combat royale / laser-tag system built around ESP32 game units and a companion .NET MAUI mobile application. The game units handle player GPS positioning within current safe zone, weapon/health/bonus pickups, infrared firing and hit detection, audio, game state and display output. The mobile app is used to prepare game plans, work with site maps, discover and configure units, and control games.

This README describes the repository as it currently exists. Refer to the individual project files for implementation and build details.

## Repository layout

| Path | Purpose |
| --- | --- |
| `Firmware/` | ESP32-S3 firmware, built with PlatformIO and Arduino. Includes game state, timing and zones, GPS/map conversion, networking, weapons, pickups, audio, and display/graphics. |
| `CombatLR.mobile/` | .NET MAUI mobile app for site maps, game-plan editing, player/team setup, device discovery and game control. |
| `CombatlrMapBuilder/` | Windows/WPF tool for calibrating a site map and exporting the runtime map package. |
| `CombatLR_GpsSimulator/` | Windows/WPF tool for supplying simulated NMEA GPS data during firmware development and testing. |
| `Electronics/`, `DocsAndIdeas/` | Supporting hardware material, notes and design ideas. |

## Firmware

The firmware project is in `Firmware/`. Its PlatformIO configuration is `Firmware/platformio.ini`, with the `esp32s3_n16r8` environment for the ESP32-S3 and LittleFS.

Build from the firmware directory with PlatformIO CLI installed:

```sh
cd Firmware
pio run
```

### Device configuration and credentials

The firmware configuration file is `Firmware/data/system.json`. It contains non-secret device settings such as team/player IDs, friendly-fire behaviour and GPS settings. Wi-Fi network credentials and FTP login details are stored separately in ESP32 NVS and are not written to `system.json`.

On upgrade, firmware imports credentials from the legacy `system.json` into NVS before removing the credential fields from the file. If the NVS write fails, the legacy configuration is retained so the credentials are not lost. Keep the device powered while this first-boot migration runs.

A device with no Wi-Fi credentials enters network provisioning mode automatically. Provisioning can also be selected using the existing trigger/right-switch startup combination. The device creates a temporary access point named `CombatLR-XXXX`; the unique WPA2 setup key is shown on the device display. Connect to that access point and open `http://192.168.4.1` to configure up to eight Wi-Fi networks and the FTP login. Existing passwords are not sent back to the browser; leave a password field blank to retain the existing password for that network/account.

NVS separates secrets from the FTP-accessible LittleFS filesystem, but **NVS is not encrypted by this change**. Someone with physical access and suitable flash-reading equipment may still be able to recover stored credentials. ESP32 flash encryption and Secure Boot are separate hardening options, not enabled by this change. FTP itself should only be used on trusted networks.

This documents the configured build entry point; it does not imply that a build or hardware test was run as part of this README update.

## Mobile application

The MAUI project is in `CombatLR.mobile/`. Open `CombatLR.mobile.slnx` in a compatible Visual Studio installation. The project targets .NET 10 for Android and may include additional targets according to the operating system and installed workloads.

A command-line build can be attempted with the .NET SDK and required workloads installed:

```sh
dotnet build CombatLR.mobile/CombatLR.mobile.csproj
```

## Site-map calibration

The current map-building tool is `CombatlrMapBuilder/`. Its calibration uses four image control points, A, B, C and D, each associated with a GPS coordinate. The tool fits an affine transform between source-image pixels and local geographic east/north coordinates.

The selected playfield is exported as a **1280 × 960 RGB565 BMP** together with `map.json`. The JSON contains the source-image crop, playfield geographic coordinates, metres-per-pixel information and the four calibration reference points. Firmware consumes this metadata to relate map pixels to geographic positions. Orientation is derived from the calibration transform, the source map image should be pre-rotated to fit within a 4:3 image.

See [the CombatlrMapBuilder project guide](CombatlrMapBuilder/README.md) for tool-specific notes.

## Network ports

The current mobile and firmware source uses:

- **UDP 4210** for device discovery
- **TCP 4211** for commands
- **FTP 21** for file transfer. TLS only. TLSF is not currently supported. Note ESP32 wifi + FTP is only active during provisioning stage.

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
