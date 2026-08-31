#pragma once

class DisplayManager;
class INavigation;
class ZoneEngine;
class ZoneEngine;
class ISounds;
class IZoom;
class GeoReference;

struct StateData
{

    DisplayManager& displayManager;
    INavigation& navigation;
    ZoneEngine& zoneEngine;
    ISounds& sounds;
    IZoom& zoom;
    GeoReference& geoReference;
};