# Combatlr Map Builder — Stage 1

First prototype of the user-supplied map workflow.

## Current workflow

1. Open an arbitrary-size map image.
2. Enter the rotation angle supplied with the imagery.
3. Click two reference points on the image.
4. Enter the real-world distance between those points.
5. The two points establish **scale only**.
6. A fixed 1280 × 960 playfield rectangle is shown.
7. The rectangle is intended to be moved around the source image to select the desired playfield.

The reference points do **not** define the output rectangle or its aspect ratio.

## Important limitation in this first prototype

The 1280 × 960 rectangle is currently shown at native source-image pixel size. This is intentional for the first UI prototype, but it means the next stage needs to implement the relationship between the calculated metres/pixel scale and the source-image crop resolution.

The rotation value is currently captured in the UI but is not yet applied to the image or coordinate transformation.

## Requirements

- Windows
- Visual Studio 2022 Community
- .NET 8 SDK
- Visual Studio workload: `.NET desktop development`

No third-party packages are required.

## Running

Open `CombatlrMapBuilder.sln` in Visual Studio 2022 and press F5.
