#pragma  once

/*
 * Game coordinates are deliberately independent of GPS.
 *
 * X = metres east of Home
 * Y = metres north of Home
 */
class GameCoordinate
{
public:
    GameCoordinate(float eastPos, float northPos)
    :east(eastPos),
    north(northPos)
    {};
    GameCoordinate()
    : east(0.0f),
    north(0.0f)
    {}
    float east;
    float north;
};