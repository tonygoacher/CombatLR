#include "ST7789VDisplay.h"

#include <math.h>
#include <string.h>


// =============================================================
// Logical rendering canvas
// =============================================================
//
// The physical TFT is 320x240, but arbitrary-angle rotation can
// require a larger composition canvas. 480x480 is large enough
// to contain the diagonal of a 320x240 display (400 pixels).
//
// The physical display is a 320x240 viewport centred on this
// canvas. The final sprite is therefore pushed at (-80,-120).
// =============================================================

static constexpr int16_t SCREEN_WIDTH  = 320;
static constexpr int16_t SCREEN_HEIGHT = 240;

static constexpr int16_t CANVAS_WIDTH  = 480;
static constexpr int16_t CANVAS_HEIGHT = 480;

static constexpr int16_t VIEWPORT_X =
    (CANVAS_WIDTH - SCREEN_WIDTH) / 2;

static constexpr int16_t VIEWPORT_Y =
    (CANVAS_HEIGHT - SCREEN_HEIGHT) / 2;

static constexpr int16_t CANVAS_CX =
    CANVAS_WIDTH / 2;

static constexpr int16_t CANVAS_CY =
    CANVAS_HEIGHT / 2;

    


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
     * The map sprite is a 320x240 viewport containing a scaled
     * view of the high-resolution source map.
     *
     * It is then rotated into the larger 480x480 screen canvas.
     */
    _mapSprite.setPivot(
        CANVAS_CX,
        CANVAS_CY);


    _screenSprite.setPivot(
        CANVAS_CX,
        CANVAS_CY);


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
    /*
     * _mapSprite is the complete 480x480 logical canvas.
     *
     * The original high-resolution map remains in _mapImage.
     * RenderMap() samples the source into this larger canvas before
     * rotating it, preventing the physical 320x240 viewport from
     * exposing the corners of a clipped 320x240 source rectangle.
     */
    if (_mapSprite.createSprite(
            CANVAS_WIDTH,
            CANVAS_HEIGHT) == nullptr)
    {
        return false;
    }


    if (_screenSprite.createSprite(
            CANVAS_WIDTH,
            CANVAS_HEIGHT) == nullptr)
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
        // Screen-space widget coordinates are specified in
        // physical-display coordinates. Translate them into the
        // larger logical canvas.
        result.x = VIEWPORT_X + position.x;
        result.y = VIEWPORT_Y + position.y;
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
     * Convert source-map pixels into display pixels.
     *
     * At zoom 1.0 the complete source map is fitted into the
     * 320x240 viewport.  Therefore map-widget coordinates must
     * use the same fit scale as the map image itself.
     */
    const float scaleX =
        static_cast<float>(SCREEN_WIDTH) /
        static_cast<float>(_mapWidth);

    const float scaleY =
        static_cast<float>(SCREEN_HEIGHT) /
        static_cast<float>(_mapHeight);

    const float fitScale =
        (scaleX < scaleY) ? scaleX : scaleY;

    const float effectiveScale =
        fitScale * _mapZoom;

    x *= effectiveScale;
    y *= effectiveScale;


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
        CANVAS_CX +
        rotatedX;


    result.y =
        CANVAS_CY +
        rotatedY;


    result.scale = effectiveScale;


    return result;
}


/*
 * =============================================================
 * RenderMap
 * =============================================================
 */



void ST7789VDisplay::RenderMap()
{
    if (_mapImage == nullptr)
    {
        _screenSprite.fillSprite(TFT_BLACK);
        return;
    }

    /*
     * Render the map into the complete 480x480 logical canvas.
     *
     * This is important because the physical display is only 320x240,
     * but the map is subsequently rotated. A 320x240 source rectangle
     * would expose its corners as black when rotated.
     *
     * At zoom 1.0 the complete map is fitted to the 320x240 viewport.
     * At higher zoom levels the map becomes progressively larger than
     * the physical viewport, providing the extra coverage needed for
     * arbitrary-angle rotation.
     */

    _mapSprite.fillSprite(TFT_BLACK);

    const float scaleX =
        static_cast<float>(SCREEN_WIDTH) /
        static_cast<float>(_mapWidth);

    const float scaleY =
        static_cast<float>(SCREEN_HEIGHT) /
        static_cast<float>(_mapHeight);

    // Preserve aspect ratio when fitting the complete map.
    const float fitScale =
        (scaleX < scaleY) ? scaleX : scaleY;

    const float effectiveScale =
        fitScale * _mapZoom;

    const float sourceCentreX =
        (static_cast<float>(_mapWidth) - 1.0f) * 0.5f;

    const float sourceCentreY =
        (static_cast<float>(_mapHeight) - 1.0f) * 0.5f;

    const float canvasCentreX =
        (static_cast<float>(CANVAS_WIDTH) - 1.0f) * 0.5f;

    const float canvasCentreY =
        (static_cast<float>(CANVAS_HEIGHT) - 1.0f) * 0.5f;

    /*
     * Render the zoomed source map into the complete 480x480 logical
     * canvas using nearest-neighbour sampling.
     *
     * Pixels outside the source map remain black. At sufficient zoom
     * the source map covers the entire area which can rotate into the
     * physical 320x240 viewport.
     */
    for (int16_t y = 0; y < CANVAS_HEIGHT; ++y)
    {
        const float sourceY =
            sourceCentreY +
            (static_cast<float>(y) - canvasCentreY) /
                effectiveScale;

        const int32_t sy =
            static_cast<int32_t>(floorf(sourceY + 0.5f));

        if (sy < 0 ||
            sy >= static_cast<int32_t>(_mapHeight))
        {
            continue;
        }

        for (int16_t x = 0; x < CANVAS_WIDTH; ++x)
        {
            const float sourceX =
                sourceCentreX +
                (static_cast<float>(x) - canvasCentreX) /
                    effectiveScale;

            const int32_t sx =
                static_cast<int32_t>(floorf(sourceX + 0.5f));

            if (sx < 0 ||
                sx >= static_cast<int32_t>(_mapWidth))
            {
                continue;
            }

            _mapSprite.drawPixel(
                x,
                y,
                _mapImage[
                    static_cast<size_t>(sy) *
                        _mapWidth +
                    static_cast<size_t>(sx)]);
        }
    }

    /*
     * Rotate the complete 480x480 map canvas into the 480x480 screen
     * canvas. The physical 320x240 display is then taken as a centred
     * viewport of the result.
     */
    _screenSprite.fillSprite(TFT_BLACK);

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
        VIEWPORT_X + 5,
        VIEWPORT_Y + 5,
        110, 35,
        5,
        TFT_RED);

    _screenSprite.setTextColor(TFT_WHITE, TFT_RED);
    _screenSprite.setTextSize(2);
    _screenSprite.setCursor(
        VIEWPORT_X + 15,
        VIEWPORT_Y + 14);
    _screenSprite.print("TEST");


    // --------------------------------------------------------
    // Fixed blue box
    // --------------------------------------------------------

    _screenSprite.fillRect(
        VIEWPORT_X + SCREEN_WIDTH - 55,
        VIEWPORT_Y + 5,
        50,
        35,
        TFT_BLUE);


    // --------------------------------------------------------
    // Centre marker
    // --------------------------------------------------------

    int cx = CANVAS_CX;
    int cy = CANVAS_CY;

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
        VIEWPORT_X,
        VIEWPORT_Y,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        TFT_WHITE);


    // --------------------------------------------------------
    // ONE physical TFT update
    // --------------------------------------------------------

    _screenSprite.pushSprite(-VIEWPORT_X, -VIEWPORT_Y);
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

    // 1. Render rotated map into _screenSprite
    RenderMap();

    // 2. Render widgets on top
    RenderWidgets();

    // 3. Present the completed frame
    _screenSprite.pushSprite(-VIEWPORT_X, -VIEWPORT_Y);

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
        -VIEWPORT_X,
        -VIEWPORT_Y);
}