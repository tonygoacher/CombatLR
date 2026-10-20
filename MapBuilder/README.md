# CombatLR MapBuilder

MapBuilder prepares source map imagery and GPS calibration metadata for CombatLR's GPS-based combat royale game. It is a site-data tool: game objects and pickups are configured separately in the mobile app.

## Current calibration format

The runtime `map.json` format used by the ESP32 firmware contains **four GPS/image control points (A, B, C and D)**, with pixel coordinates in the **original source image**. The firmware fits an affine transform from those four correspondences to local east/north coordinates using least squares. The source-image crop is applied when converting positions to the exported 1280×960 map.

Map orientation follows the fitted transform. A manually entered map heading or a legacy `RotationDegrees` value is **not** part of this four-point runtime calibration.

## Important implementation mismatch

**The WPF editor source currently checked into `MapBuilder/MainWindow.xaml.cs` still uses two reference points (A and B) and a rotation field.** The corresponding file in `CombatlrMapBuilder/` also needs checking before assuming it implements the newer workflow.

That editor implementation does not match the four-point affine runtime format in `Firmware/include/MapReference.h` and `Firmware/src/MapReference.cpp`. This README documents the intended/current runtime contract, **not a verified four-point feature in the checked-in WPF editor**. Do not rely on the editor to produce compatible four-point map metadata until the mismatch is resolved.

For an example of the runtime format, see `Resources/Alexandra/map.json`, which contains `SourceCrop` and `Reference.Points` A–D.

## Site output

The runtime map is **1280×960 pixels** (4:3). The selected source-image crop and GPS reference points are used to relate geographic positions to pixels in that exported image.

Editable `.combatlr` projects and exported site packages are distinct. The editor's current project file and export behavior should be reviewed alongside any upgrade to four-point calibration.

## Requirements

The checked-in WPF project targets **.NET 8 for Windows** and requires a Windows development environment with the .NET desktop workload. Refer to `CombatlrMapBuilder.csproj` for the current project settings.

## Historical notes

Earlier MapBuilder stages used two points, a measured distance, bearing, and/or manually entered rotation. Those approaches are historical and must not be confused with the four-point affine calibration expected by the firmware.
