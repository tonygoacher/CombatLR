#pragma once



enum SystemStates
{
    PowerUp,
    GPSLock,
    AwaitingStart,
    GameInit,       // Time t to get in position around the map
    GameRunning,
    Dead,
    Ftp,
    OTA,
    Provisioning,
    IRTestState,
};