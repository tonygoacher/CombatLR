# Combatlr Map Builder

`CombatlrMapBuilder/` contains the current Windows/WPF site-map calibration tool for CombatLR. It prepares a site map and its geographic metadata for use by the firmware and mobile app. Game objects and pickups are configured separately in the mobile app/game plan.

## Requirements

- Windows
- .NET 8 SDK
- Visual Studio with the .NET desktop development workload, or another compatible .NET build environment

The project targets `net8.0-windows` and uses WPF plus `System.Drawing.Common`.

## Workflow

1. Open or create a project using the **File** menu.
2. Open the source map image.
3. Mark four calibration points, A, B, C and D, on the source image and enter the latitude and longitude for each point.
4. Position and resize the playfield rectangle over the area to export. The playfield keeps the CombatLR 4:3 aspect ratio.
5. Save the editable project if you want to continue working on it later.
6. Choose **Export Site Package** when the image and all four GPS calibration points are complete.

The map can be zoomed for accurate point placement. The playfield can be moved and resized with the mouse.

## Calibration model

The four image/GPS correspondences are used to fit an affine transform from source-image pixel coordinates to local geographic east/north coordinates. The fit uses all four points; the workflow does not ask for a manually entered rotation angle.

Calibration points are recorded in the coordinate system of the original source image. The selected playfield crop is then resampled to the fixed runtime image dimensions.

## Exported site package

Export produces two files in the selected output directory:

- `map.bmp` — a **1280 × 960 RGB565 BMP** for the CombatLR runtime.
- `map.json` — versioned metadata describing the image dimensions, metres-per-pixel, source-image crop, geographic playfield centre and corners, and the A/B/C/D GPS calibration points.

The firmware uses the calibration metadata and source crop to map between the runtime map and geographic coordinates. The mobile app uses the exported map package when preparing and controlling a game.

Export requires a source image and complete, valid A/B/C/D calibration. Editable `.combatlr` project files are separate from the exported runtime package and can be saved before calibration is complete.

## Build

From this directory, with the .NET 8 SDK installed:

```sh
dotnet build CombatlrMapBuilder.csproj
```

A successful build or hardware/runtime test is not implied by these instructions.
