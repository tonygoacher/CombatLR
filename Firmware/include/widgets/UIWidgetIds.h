#pragma once


// GUI widget Ids have their own ID range to separate them from Zonewidgets and MapWidgets

enum UIWidgetIds
{
    GPStatus = 0xF000,
    CountdownTimer = 0xF001,
    SelectWeapon = 0xF002,   // The weapon on the weapon select
    AmmoDisplay = 0xF003,           // The ammo of the current weapon
    PlayerHealth = 0xF004,
    GPSHdop = 0xF005,
    WiFiStatus = 0xF006,
    TeamID = 0xF007,
    IPAdrString= 0xF008,
    SSIDString= 0xF009,
    GunIDString = 0xF00a
};