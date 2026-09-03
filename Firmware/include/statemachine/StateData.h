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

struct StateData
{

    DisplayManager& displayManager;
    INavigation& navigation;
    ZoneEngine& zoneEngine;
    ISounds& sounds;
    IZoom& zoom;
    GeoReference& geoReference;
    ISystemTime& systemTime;
    PBSwitch&   triggerSwitch;
    PBSwitch&   reloadSwitch;
    PBSwitch&   leftSwitch;
    PBSwitch&   zoomSwitch;     
    PBSwitch&   rightSwitch;
};