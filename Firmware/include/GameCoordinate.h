#pragma  once

/*
 * Game coordinates are deliberately independent of GPS.
 *
 * X = metres east of Home
 * Y = metres north of Home
 */
struct GameCoordinate
{
    float east = 0.0f;
    float north = 0.0f;
};