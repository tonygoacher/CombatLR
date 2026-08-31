#pragma once



enum SystemStates
{
    PowerUp,
    AwaitingStart,
    GameInit,       // Time t to get in position around the map
    GameRunning,
    OutOfGame,
};