#pragma once

#include <Arduino.h>
#include <deque>
#include <memory>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "ST7789VDisplay.h"
#include "IDisplayCommand.h"
#include "TextureManager.h"


class ZoomCommand;
class PlayerPositionCommand;
class TurnMapToHeadingCommand;
class DisplayBackgroundCommand;
class ConfigureMapCommand;
class AddWidgetCommand;
class AddZoneCommand;


class DisplayManager
{
public:
    DisplayManager(TextureManager& textureManager);
    void ConfigureNavigationMap(
    const char* mapFilename,
    double homeLatitude,
    double homeLongitude);
    ~DisplayManager();

    bool Begin();

    bool Send(
    std::unique_ptr<IDisplayCommand> command);

private:
    static void DisplayTaskEntry(
        void* parameter);

    void DisplayTask();

    void ProcessCommand(const IDisplayCommand* command);

    void SetZoom(const ZoomCommand*);
    void SetPlayerPosition(const PlayerPositionCommand*);
    void TurnMapToHeading(const TurnMapToHeadingCommand*);
    void DisplayBackground(const DisplayBackgroundCommand *);
    void ConfigureMap(const ConfigureMapCommand *);
    void AddWidget(const AddWidgetCommand* command);
    void AddZone(const AddZoneCommand* command);

std::deque<
    std::unique_ptr<IDisplayCommand>>
    _commandQueue;

std::deque<
    std::unique_ptr<IDisplayCommand>>
    _processingQueue;

    SemaphoreHandle_t _queueMutex;

    ST7789VDisplay* _display;

    GameCoordinate _playerPosition;

    float _heading;
    float _zoom;

    bool _playerVisible;
    TextureManager& _textureManager;
};