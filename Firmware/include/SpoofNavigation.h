#pragma once

#include "Navigation.h"

/*
 * Software navigation source used while developing the game.
 *
 * It allows the game to be tested without the real GNSS hardware.
 */
class SpoofNavigation : public INavigation
{
public:
    SpoofNavigation();

    bool Begin() override;
    bool Update() override;

    NavigationData GetData() const override;

    void SetPosition(
        double latitude,
        double longitude);

    void SetPosition(
        double latitude,
        double longitude,
        float altitude);

    /*
     * Set position relative to an externally defined Home position.
     *
     * eastMeters  = metres east of Home
     * northMeters = metres north of Home
     */
    void SetPositionRelative(
        double homeLatitude,
        double homeLongitude,
        float eastMeters,
        float northMeters);

    void SetHeading(float headingDegrees);
    void SetCourse(float courseDegrees);
    void SetSpeed(float speedMps);

    void SetPositionValid(bool valid);
    void SetHeadingValid(bool valid);
    void SetSatellites(uint8_t satellites);

private:
    NavigationData _data;

    static double NormaliseLongitude(double longitude);
    static float NormaliseHeading(float heading);
};
