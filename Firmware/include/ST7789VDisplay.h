#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <map>

#include "IDisplay.h"
#include "IWidget.h"
#include "IDisplayContext.h"
#include "ConfigureMapCommand.h"
#include "MapReference.h"
#include "Texture.h"


class ST7789VDisplay : public IDisplay
{
public:
    ST7789VDisplay();

    ~ST7789VDisplay();

    bool Init();

    bool SetBackground(const char * filename);

    void SetRotation(
        float angle) override;


    float GetRotation() const override;


    void RemovedWidget(uint32_t widgetId) override;
    
    void SetMapZoom(
        float zoom) override;


    float GetMapZoom() const override;


    uint16_t AddWidget(
        IWidget* widget) override;


    bool RemoveWidget(
        uint16_t widgetId) override;


    
    void Update(
    const GameCoordinate& playerPosition,
    float heading,
    float zoom,
    bool playerVisible) override;


    void Clear() override;

    void SetPlayerHeading(float headingDegrees);

    void SetMapConfigData(const ConfigureMapCommand::MapConfigData& configData);

    bool ConfigureNavigationMap( const char* mapFilename,  double homeLatitude, double homeLongitude) override;

    void SetWidgetColour(uint32_t widgetId, uint32_t colour) override;

     void SetZoneGeometry(uint32_t zoneId, GameCoordinate newPos, float radius) override;


private:

    typedef std::pair<uint16_t, IWidget*>  WidgetEntry;
    typedef  std::map<uint16_t , IWidget*>WidgetCollection;
    WidgetCollection _widgets;


     /*
     * ---------------------------------------------------------
     * TFT
     * ---------------------------------------------------------
     */

    TFT_eSPI _tft;


    /*
     * Map sprite.
     *
     * Contains the unrotated map.
     *
     * Allocated by TFT_eSPI in PSRAM.
     */
    TFT_eSprite _mapSprite;


    /*
     * Final screen sprite.
     *
     * Contains the rotated map and the upright widgets.
     *
     * Allocated in PSRAM.
     */
    TFT_eSprite _screenSprite;


    /*
     * ---------------------------------------------------------
     * Map image
     * ---------------------------------------------------------
     */

    uint16_t* _mapImage;

    uint16_t _mapWidth;
    uint16_t _mapHeight;


    /*
     * ---------------------------------------------------------
     * Map transform
     * ---------------------------------------------------------
     */

    float _rotation;

    float _mapZoom;

    float _cameraMapX;
    float _cameraMapY;

    float _mapUpHeading;


    /*
     * ---------------------------------------------------------
     * State
     * ---------------------------------------------------------
     */

    bool _initialised;

    float _playerHeading;
    bool _playerHeadingValid;
    GameCoordinate _playerPosition;

    bool _playerVisible;


    /*
     * ---------------------------------------------------------
     * Internal functions
     * ---------------------------------------------------------
     */

    bool AllocateSprites();


    void FreeSprites();


    void RenderMap();


    void RenderWidgets();

    void RenderNavigationPlayer(
        TFT_eSprite& canvas,
        int16_t screenX,
        int16_t screenY);


    WidgetRenderPosition
    TransformWidgetPosition(
        const IWidget& widget);


    void TestSprites();

    MapReference _navigationMap;
    bool _navMapConfigured;
    Texture _backgroundTexture;
};