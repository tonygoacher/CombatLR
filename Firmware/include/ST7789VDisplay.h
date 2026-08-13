#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "IDisplay.h"
#include "IWidget.h"
#include "IDisplayContext.h"


class ST7789VDisplay : public IDisplay
{
public:

    static constexpr uint16_t MAX_WIDGETS = 32;


    /*
     * mapImage is RGB565 pixel data.
     *
     * The image is copied into PSRAM.
     */
    ST7789VDisplay(
        const uint16_t* mapImage,
        uint16_t mapWidth,
        uint16_t mapHeight);


    ~ST7789VDisplay() override;


    bool Init() override;


    void SetRotation(
        float angle) override;


    float GetRotation() const override;


    void SetMapZoom(
        float zoom) override;


    float GetMapZoom() const override;


    uint16_t AddWidget(
        IWidget* widget) override;


    bool RemoveWidget(
        uint16_t widgetId) override;


    void Update() override;


    void Clear() override;

    void SetPlayerHeading(float headingDegrees);




private:

    /*
     * ---------------------------------------------------------
     * Widget storage
     * ---------------------------------------------------------
     */

    struct WidgetEntry
    {
        uint16_t id;
        IWidget* widget;
        bool active;
    };


    WidgetEntry _widgets[MAX_WIDGETS];

    uint16_t _nextWidgetId;


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


    /*
     * ---------------------------------------------------------
     * State
     * ---------------------------------------------------------
     */

    bool _initialised;

    float _playerHeading;
    bool _playerHeadingValid;


    /*
     * ---------------------------------------------------------
     * Internal functions
     * ---------------------------------------------------------
     */

    bool AllocateSprites();


    void FreeSprites();


    void RenderMap();


    void RenderWidgets();


    WidgetRenderPosition
    TransformWidgetPosition(
        const IWidget& widget);


    uint16_t AllocateWidgetId();


    WidgetEntry* FindWidget(
        uint16_t widgetId);

    void TestSprites();
};