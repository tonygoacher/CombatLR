#include "ST7789VDisplay.h"

#include <math.h>
#include <string.h>


ST7789VDisplay::ST7789VDisplay(
    const uint16_t* mapImage,
    uint16_t mapWidth,
    uint16_t mapHeight)
    :
    _widgets{},
    _nextWidgetId(1),
    _tft(),
    _mapSprite(&_tft),
    _screenSprite(&_tft),
    _mapImage(nullptr),
    _mapWidth(mapWidth),
    _mapHeight(mapHeight),
    _rotation(0.0f),
    _mapZoom(1.0f),
    _initialised(false)
{
    /*
     * Copy the map image into PSRAM.
     *
     * The image is RGB565, therefore:
     *
     * width * height * 2 bytes
     */
    const size_t imageBytes =
        static_cast<size_t>(_mapWidth) *
        static_cast<size_t>(_mapHeight) *
        sizeof(uint16_t);


    if (mapImage != nullptr)
    {
        _mapImage =
            static_cast<uint16_t*>(
                ps_malloc(imageBytes));


        if (_mapImage != nullptr)
        {
            memcpy(
                _mapImage,
                mapImage,
                imageBytes);
        }
    }
}


ST7789VDisplay::~ST7789VDisplay()
{
    FreeSprites();


    if (_mapImage != nullptr)
    {
        free(_mapImage);
        _mapImage = nullptr;
    }
}


/*
 * =============================================================
 * Init
 * =============================================================
 */

bool ST7789VDisplay::Init()
{
    _tft.init();


    /*
     * Enable TFT_eSPI PSRAM support.
     *
     * This must be done before sprite allocation.
     */
    _tft.setAttribute(
        PSRAM_ENABLE,
        1);


    /*
     * Physical orientation of the ST7789V.
     *
     * This is NOT the dynamic compass rotation.
     */
    _tft.setRotation(1);

    _tft.setSwapBytes(true);

// DEBUG


//_DEBUG
    if (!AllocateSprites())
    {
        return false;
    }

     

    /*
     * Set rotation pivots so the map rotates around the
     * centre of the display.
     */
    _mapSprite.setPivot(
        _mapWidth / 2,
        _mapHeight / 2);


    _screenSprite.setPivot(
        _tft.width() / 2,
        _tft.height() / 2);


    _tft.fillScreen(
        TFT_BLACK);


    _initialised = true;


    return true;
}


/*
 * =============================================================
 * AllocateSprites
 * =============================================================
 */

bool ST7789VDisplay::AllocateSprites()
{
    /*
     * 16-bit RGB565.
     */
    _mapSprite.setColorDepth(16);

    _screenSprite.setColorDepth(16);

    _mapSprite.setSwapBytes(true);
    _screenSprite.setSwapBytes(true);
    /*
     * TFT_eSPI will allocate sprites from PSRAM because
     * PSRAM_ENABLE was enabled above.
     */
    if (_mapSprite.createSprite(
            _mapWidth,
            _mapHeight) == nullptr)
    {
        return false;
    }


    if (_screenSprite.createSprite(
            _tft.width(),
            _tft.height()) == nullptr)
    {
        _mapSprite.deleteSprite();

        return false;
    }


    return true;
}


/*
 * =============================================================
 * FreeSprites
 * =============================================================
 */

void ST7789VDisplay::FreeSprites()
{
    _mapSprite.deleteSprite();

    _screenSprite.deleteSprite();
}


/*
 * =============================================================
 * SetRotation
 * =============================================================
 */

void ST7789VDisplay::SetRotation(
    float angle)
{
    _rotation = fmodf(
        angle,
        360.0f);


    if (_rotation < 0.0f)
    {
        _rotation += 360.0f;
    }
}


/*
 * =============================================================
 * GetRotation
 * =============================================================
 */

float ST7789VDisplay::GetRotation() const
{
    return _rotation;
}


/*
 * =============================================================
 * SetMapZoom
 * =============================================================
 */

void ST7789VDisplay::SetMapZoom(
    float zoom)
{
    /*
     * Don't allow zero or negative zoom.
     */
    if (zoom <= 0.0f)
    {
        return;
    }


    _mapZoom = zoom;
}


/*
 * =============================================================
 * GetMapZoom
 * =============================================================
 */

float ST7789VDisplay::GetMapZoom() const
{
    return _mapZoom;
}


/*
 * =============================================================
 * AddWidget
 * =============================================================
 */

uint16_t ST7789VDisplay::AddWidget(
    IWidget* widget)
{
    if (widget == nullptr)
    {
        return 0;
    }


    for (uint16_t i = 0;
         i < MAX_WIDGETS;
         ++i)
    {
        if (!_widgets[i].active)
        {
            const uint16_t id =
                AllocateWidgetId();


            _widgets[i].id = id;
            _widgets[i].widget = widget;
            _widgets[i].active = true;


            return id;
        }
    }


    return 0;
}


/*
 * =============================================================
 * RemoveWidget
 * =============================================================
 */

bool ST7789VDisplay::RemoveWidget(
    uint16_t widgetId)
{
    WidgetEntry* entry =
        FindWidget(widgetId);


    if (entry == nullptr)
    {
        return false;
    }


    entry->id = 0;
    entry->widget = nullptr;
    entry->active = false;


    return true;
}


/*
 * =============================================================
 * AllocateWidgetId
 * =============================================================
 */

uint16_t ST7789VDisplay::AllocateWidgetId()
{
    uint16_t id = _nextWidgetId++;


    /*
     * Zero is reserved as an invalid ID.
     */
    if (id == 0)
    {
        id = _nextWidgetId++;
    }


    return id;
}


/*
 * =============================================================
 * FindWidget
 * =============================================================
 */

ST7789VDisplay::WidgetEntry*
ST7789VDisplay::FindWidget(
    uint16_t widgetId)
{
    if (widgetId == 0)
    {
        return nullptr;
    }


    for (uint16_t i = 0;
         i < MAX_WIDGETS;
         ++i)
    {
        if (_widgets[i].active &&
            _widgets[i].id == widgetId)
        {
            return &_widgets[i];
        }
    }


    return nullptr;
}


/*
 * =============================================================
 * TransformWidgetPosition
 * =============================================================
 *
 * Map widget:
 *
 *     1. Translate relative to map centre
 *     2. Apply zoom
 *     3. Apply rotation
 *     4. Translate to screen centre
 *
 * Screen widget:
 *
 *     Position is unchanged.
 *
 * The widget itself is never rotated.
 */

WidgetRenderPosition
ST7789VDisplay::TransformWidgetPosition(
    const IWidget& widget)
{
    const WidgetPosition position =
        widget.GetPosition();


    WidgetRenderPosition result;


    /*
     * ---------------------------------------------------------
     * Screen widget
     * ---------------------------------------------------------
     */

    if (widget.GetCoordinateSpace() ==
        WidgetCoordinateSpace::Screen)
    {
        result.x = position.x;
        result.y = position.y;
        result.scale = 1.0f;

        return result;
    }


    /*
     * ---------------------------------------------------------
     * Map widget
     * ---------------------------------------------------------
     *
     * Map coordinates are currently interpreted as pixels
     * relative to the map image centre.
     *
     * A later map-coordinate layer can convert the CombatLR
     * metre coordinates into these coordinates.
     */

    float x = position.x;
    float y = position.y;


    /*
     * Apply zoom.
     */
    x *= _mapZoom;
    y *= _mapZoom;


    /*
     * Apply clockwise rotation.
     */
    const float radians =
        _rotation *
        (PI / 180.0f);


    const float sinAngle =
        sinf(radians);


    const float cosAngle =
        cosf(radians);


    const float rotatedX =
        (x * cosAngle) -
        (y * sinAngle);


    const float rotatedY =
        (x * sinAngle) +
        (y * cosAngle);


    /*
     * Move from map-centred coordinates into screen
     * coordinates.
     */
    result.x =
        (_tft.width() * 0.5f) +
        rotatedX;


    result.y =
        (_tft.height() * 0.5f) +
        rotatedY;


    result.scale = _mapZoom;


    return result;
}


/*
 * =============================================================
 * RenderMap
 * =============================================================
 */

/*

void ST7789VDisplay::RenderMap()
{
    if (_mapImage == nullptr)
    {
        _screenSprite.fillSprite(TFT_BLACK);
        return;
    }

    _mapSprite.pushImage(
        0,
        0,
        _mapWidth,
        _mapHeight,
        _mapImage);


    _mapSprite.pushSprite(0, 0);
}
    */

void ST7789VDisplay::RenderMap()
{
    if (_mapImage == nullptr)
    {
        _screenSprite.fillSprite(TFT_BLACK);
        return;
    }

    _screenSprite.fillSprite(TFT_BLACK);

    _mapSprite.pushImage(
        0,
        0,
        _mapWidth,
        _mapHeight,
        _mapImage);

    _mapSprite.pushRotated(
        &_screenSprite,
        static_cast<int16_t>(_rotation),
        TFT_BLACK);
}


void ST7789VDisplay::TestSprites()
{
    RenderMap();

    // --------------------------------------------------------
    // Fixed red status box
    // --------------------------------------------------------

    _screenSprite.fillRoundRect(
        5, 5,
        110, 35,
        5,
        TFT_RED);

    _screenSprite.setTextColor(TFT_WHITE, TFT_RED);
    _screenSprite.setTextSize(2);
    _screenSprite.setCursor(15, 14);
    _screenSprite.print("TEST");


    // --------------------------------------------------------
    // Fixed blue box
    // --------------------------------------------------------

    _screenSprite.fillRect(
        _screenSprite.width() - 55,
        5,
        50,
        35,
        TFT_BLUE);


    // --------------------------------------------------------
    // Centre marker
    // --------------------------------------------------------

    int cx = _screenSprite.width() / 2;
    int cy = _screenSprite.height() / 2;

    _screenSprite.drawLine(
        cx - 10, cy,
        cx + 10, cy,
        TFT_WHITE);

    _screenSprite.drawLine(
        cx, cy - 10,
        cx, cy + 10,
        TFT_WHITE);


    // --------------------------------------------------------
    // Border
    // --------------------------------------------------------

    _screenSprite.drawRect(
        0,
        0,
        _screenSprite.width(),
        _screenSprite.height(),
        TFT_WHITE);


    // --------------------------------------------------------
    // ONE physical TFT update
    // --------------------------------------------------------

    _screenSprite.pushSprite(0, 0);
}
/*
 * =============================================================
 * RenderWidgets
 * =============================================================
 */

void ST7789VDisplay::RenderWidgets()
{
    for (uint16_t i = 0;
         i < MAX_WIDGETS;
         ++i)
    {
        if (!_widgets[i].active)
        {
            continue;
        }


        if (_widgets[i].widget == nullptr)
        {
            continue;
        }


        /*
         * Calculate the final screen position.
         *
         * This is where the display applies map rotation
         * and zoom.
         */
        const WidgetRenderPosition position =
            TransformWidgetPosition(
                *_widgets[i].widget);


        /*
         * Create a rendering context containing the final
         * position.
         */
        class DisplayContext final
            : public IDisplayContext
        {
        public:

            DisplayContext(
                TFT_eSprite& canvas,
                const WidgetRenderPosition& position)
                :
                _canvas(canvas),
                _position(position)
            {
            }


            TFT_eSprite& GetCanvas() override
            {
                return _canvas;
            }


            const WidgetRenderPosition&
                GetPosition() const override
            {
                return _position;
            }


        private:

            TFT_eSprite& _canvas;

            const WidgetRenderPosition& _position;
        };


        DisplayContext context(
            _screenSprite,
            position);


        /*
         * The widget draws itself normally.
         *
         * It knows nothing about rotation.
         */
        _widgets[i].widget->Render(
            context);
    }
}



/*
 * =============================================================
 * Update
 * =============================================================
 */

void ST7789VDisplay::Update()
{
    if (!_initialised)
    {
        return;
    }


    /*
     * 1. Render the map.
     *
     * 2. Rotate the map into the screen sprite.
     */

    TestSprites();

    /*
     * 3. Transform map widget positions.
     *
     * 4. Render all widgets upright.
     */
 //   RenderWidgets();


    /*
     * 5. Present the completed frame.
     */
 //   _screenSprite.pushSprite(
 ///       0,
  //      0);



}


/*
 * =============================================================
 * Clear
 * =============================================================
 */

void ST7789VDisplay::Clear()
{
    if (!_initialised)
    {
        return;
    }


    _screenSprite.fillSprite(
        TFT_BLACK);


    _screenSprite.pushSprite(
        0,
        0);
}