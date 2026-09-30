#pragma once

class DisplayManager;
class INavigation;
class ZoneEngine;
class ZoneEngine;
class ISounds;
class IZoom;
class GeoReference;
class ISystemTime;
class PBSwitch;
class IRLaserTag;
class WeaponConfig;
class ESP32LaserTag;
class PacedLED;
class GamePlan;
class SystemConfig;

struct CombatLRServices
{
    SystemConfig& systemConfig;
    DisplayManager& displayManager;
    INavigation& navigation;
    ZoneEngine& zoneEngine;
    ISounds& sounds;
    IZoom& zoom;
    GamePlan& gamePlan;

    GeoReference& geoReference;
    IRLaserTag& laserTag;
    ISystemTime& systemTime;
    WeaponConfig& weaponConfig;

    PBSwitch&   triggerSwitch;
    PBSwitch&   reloadSwitch;
    PBSwitch&   leftSwitch;
    PBSwitch&   zoomSwitch;     
    PBSwitch&   rightSwitch;
    PacedLED&   muzzleLED;
    PacedLED&   hitLED;
};