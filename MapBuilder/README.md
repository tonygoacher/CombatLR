# Combatlr Map Builder — Stage 2

This stage adds the first usable map-selection workflow.

## Features

- Open an arbitrary-size source map image.
- Enter the supplied map rotation angle.
- Select two reference points.
- Enter the real-world distance between the reference points.
- Calculate metres/pixel.
- Display the physical size represented by a 1280×960 Combatlr image.
- Move the 1280×960 selection rectangle with the mouse.
- Constrain the selection rectangle to the source image.
- Zoom the map with the mouse wheel.
- Export a PNG map and accompanying JSON metadata.

## Important design rule

The two reference points establish **scale only**.

They do not define the output rectangle or its aspect ratio.

The Combatlr output is always 1280×960.

## Stage 3 interaction

- Mouse-wheel zoom keeps the map point under the mouse cursor stationary.
- The playfield can be dragged.
- The playfield has top-left and bottom-right resize handles.
- Resizing changes the physical area represented by the eventual 1280×960 output.
- The exported image is always resampled to 1280×960.
- The reference points remain independent of the playfield.

GPS centre coordinates and GPS-to-pixel transformations are reserved for a later stage.

## Requirements

- Windows
- Visual Studio 2022 Community
- .NET 8 SDK
- Visual Studio workload: .NET desktop development

No third-party packages are required.


## Stage 4

The playfield selection is locked to the Combatlr 1280×960 aspect ratio (4:3).

Resizing from either corner changes the physical coverage while preserving 4:3. The selection can still be moved freely inside the source image.

All `Path` operations use fully-qualified `System.IO.Path` to avoid ambiguity with WPF's `System.Windows.Shapes.Path`.


## Stage 5

- Map zoom now uses a WPF `LayoutTransform`, so the ScrollViewer scroll bars reflect the actual zoomed content size.
- Zoom is centred on the current mouse position.
- The playfield can be moved with right-click drag (left-click drag remains supported).
- The playfield is hosted in a `Canvas`, so its position is explicitly controlled by its source-image coordinates.
- The playfield is forced visible after loading/clearing calibration points.
- `System.IO.Path` remains fully qualified.


## Stage 6

Reference points now use their actual GPS coordinates.

The builder calculates:
- A→B real-world distance using the haversine formula.
- A→B initial geographic bearing.
- An image-derived rotation estimate from the image A→B vector and GPS bearing.

The calculated GPS distance is used for metres/pixel, so the user no longer needs to enter a separate distance.

The exported JSON includes GPS A/B coordinates and the calculated bearing.

Map zoom uses a top-left-aligned workspace and keeps the exact image point under the mouse cursor stationary.


## Stage 7

The manual reference distance field has been restored.

- GPS A/B coordinates are retained and provide a calculated GPS distance.
- If the distance field is blank, the calculated GPS distance is automatically inserted.
- The distance field remains editable, so a supplied/measured distance can be used as the authoritative calibration value.
- The JSON records both the calibration distance and the GPS-derived distance when available.


## Stage 8

Playfield resizing now selects the dominant resize axis once when the drag starts. It no longer changes between horizontal and vertical sizing during the drag, eliminating the jumpy size behaviour.

The opposite corner remains fixed and the 4:3 aspect ratio is maintained throughout the resize.

Resize operations are also ignored unless a source image is loaded.


## Stage 9 — Site Data

MapBuilder is now explicitly a **site-data tool**, not a game-object editor.

GPS coordinates for image A and B are the authoritative calibration. The builder calculates:

- A→B distance
- A→B bearing
- metres/pixel
- a geographic transform between image pixels and GPS coordinates

The selected site area is exported as a 1280×960 PNG plus `map.json`.

`map.json` includes the GPS coordinates of the selected area's centre and four corners, so the mobile app can consume the site map without needing to understand how it was created.

Game objects are intentionally not part of MapBuilder and will belong in the mobile app/game JSON.


## Stage 10 — MapBuilder project files

MapBuilder now has an editable project format using the `.combatlr` extension.

A project stores:
- relative source PNG path
- rotation
- image A/B pixel positions
- GPS A/B coordinates
- calculated reference distance/bearing
- selected site-area rectangle

Project files can be opened and saved without exporting a deployable site package.

The source image path is relative to the `.combatlr` file, so a project and its source image can be moved together.

The deployable site package remains separate and contains the final map plus runtime site metadata.


## Stage 11 — Save incomplete projects

`.combatlr` project files can now be saved at any stage of the workflow.

A project may contain:
- no source image
- a source image with no reference points
- reference points without GPS
- partially completed calibration
- a partially positioned site area

Loading such a project restores whatever information is present.

**Export Site Package** remains strict and requires complete calibration and a valid map.

This deliberately separates the editable working document from the deployable site data.


## Stage 15 — compile fixes for RGB565 export

This stage is based directly on Stage 14 and fixes the compile errors introduced by
using the pixel dimensions as doubles.

- `OutputWidth` and `OutputHeight` are now integer constants because they are pixel counts.
- The resize handler uses `newHeight` for the intermediate calculation, avoiding duplicate
  local `height` declarations.
- `System.Drawing.Common` 8.0.0 is declared explicitly in the project.
- No Windows Forms dependency is introduced.
- WPF types remain untouched and the System.Drawing types remain fully qualified in the
  export-only code.
