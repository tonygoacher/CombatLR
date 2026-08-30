#include "ST7789VDisplay.h"
#include "MapCoordinate.h"
#include "MapReference.h"


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




extern void PrintPSRAM(const char* label);




ST7789VDisplay::ST7789VDisplay()
    :
    _widgets{},
    _tft(),
    _mapSprite(&_tft),
    _screenSprite(&_tft),
    _mapImage(nullptr),
    _mapWidth(0),
    _mapHeight(0),
    _rotation(0.0f),
    _mapZoom(1.0f),
    _initialised(false),
    _navMapConfigured(false)
{}

 void ST7789VDisplay::RenderNavigationPlayer(
        TFT_eSprite& canvas,
        int16_t screenX,
        int16_t screenY)
    {
        if (!_playerVisible)
            return;

        constexpr int16_t radius = 8;
        constexpr int16_t arrowLength = 15;

        canvas.fillCircle(
            screenX,
            screenY,
            radius,
            TFT_BLUE);

        canvas.drawCircle(
            screenX,
            screenY,
            radius + 1,
            TFT_WHITE);

        /*
         * Heading-up map: the player's forward direction is always the
         * top of the display, so the marker itself remains upright.
         */
        const int16_t arrowX = screenX;
        const int16_t arrowY =
            screenY - arrowLength;

        canvas.drawLine(
            screenX,
            screenY,
            arrowX,
            arrowY,
            TFT_WHITE);

        canvas.fillCircle(
            arrowX,
            arrowY,
            2,
            TFT_WHITE);
    }



ST7789VDisplay::~ST7789VDisplay()
{
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

void ST7789VDisplay::RemovedWidget(uint32_t widgetId)
{
    WidgetCollection::iterator widgetIterator = _widgets.find(widgetId);
    Serial.printf("Display removing widget %d\n", widgetId);
    if(widgetIterator != _widgets.end())
    {
        Serial.println("Found");
        free(widgetIterator->second);
        _widgets.erase(widgetIterator);
    }
    else
    {
        Serial.println("Not found");
    }
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
    uint16_t id = 0;
    if (widget != nullptr)
    {
        Serial.printf("Adding widget %lx Type %d to renderer", widget->GetId(), widget->GetType());
        _widgets.insert(std::pair<uint16_t , IWidget*>( widget->GetId(), widget));
    }    
    return id;
}


/*
 * =============================================================
 * RemoveWidget
 * =============================================================
 */

bool ST7789VDisplay::RemoveWidget(
    uint16_t widgetId)
{
    bool result = false;
    std::map<uint16_t, IWidget*>::iterator widgetIter = _widgets.find(widgetId);
    if(widgetIter != _widgets.end())
    {
        _widgets.erase(widgetIter);
        result = true;
    }

    return result;
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
WidgetRenderPosition ST7789VDisplay::TransformWidgetPosition(
    const IWidget& widget)
{
    WidgetRenderPosition result;

    /*
     * ---------------------------------------------------------
     * Screen widget (Physical screen space relative to viewport)
     * ---------------------------------------------------------
     */
    if (widget.GetCoordinateSpace() == WidgetCoordinateSpace::Screen)
    {
        const WidgetPosition position = widget.GetPosition();
        result.x = VIEWPORT_X + position.x;
        result.y = VIEWPORT_Y + position.y;
        result.scale = 1.0f;
        result.zoom = 1.0f;
        return result;
    }

    /*
     * ---------------------------------------------------------
     * Map widget (World/Game space relative to map camera)
     * ---------------------------------------------------------
     */
    float mapX = 0.0f;
    float mapY = 0.0f;

    GameCoordinate gamePosition = widget.GetGamePosition();

    if (!_navigationMap.GameToMapPixel(gamePosition, mapX, mapY))
    {
        return result;
    }

    // 1. Calculate effective scale matching RenderMap()
    const float scaleX = static_cast<float>(CANVAS_WIDTH) / static_cast<float>(_mapWidth);
    const float scaleY = static_cast<float>(CANVAS_HEIGHT) / static_cast<float>(_mapHeight);
    const float fitScale = (scaleX < scaleY) ? scaleX : scaleY;
    const float effectiveScale = fitScale * _mapZoom;

    // 2. Compute pixel offset relative to camera position
    const float dx = (mapX - _cameraMapX) * effectiveScale;
    const float dy = (mapY - _cameraMapY) * effectiveScale;

    // 3. Obtain exact rotation angle applied during RenderMap()
const float mapRotation = _rotation;


    // 4. Convert degrees to radians and rotate offset vector
  const float radians =
    mapRotation *
    (PI / 180.0f);

const float sinAngle =
    sinf(radians);

const float cosAngle =
    cosf(radians);

const float rotatedX =
    (dx * cosAngle) -
    (dy * sinAngle);

const float rotatedY =
    (dx * sinAngle) +
    (dy * cosAngle);

    // 5. Translate offset back to logical Canvas Centre
    const float canvasCentreX = (static_cast<float>(CANVAS_WIDTH) - 1.0f) * 0.5f;
    const float canvasCentreY = (static_cast<float>(CANVAS_HEIGHT) - 1.0f) * 0.5f;

    result.x = static_cast<int16_t>(lroundf(canvasCentreX + rotatedX));
    result.y = static_cast<int16_t>(lroundf(canvasCentreY + rotatedY));
    result.scale = effectiveScale;
    result.zoom = _mapZoom;



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

    static bool havePreviousPlayerPosition = false;

    static GameCoordinate previousPlayerPosition;

    /*
     * Camera behaviour:
     *
     *   - Normally the player is at the centre of the display.
     *   - When the player gets close enough to a map edge that the camera
     *     can no longer follow, the camera is clamped to the map edge.
     *   - The player then moves towards that edge of the display.
     *
     * The camera is rotated heading-up around its own centre.
     */

    _mapSprite.fillSprite(TFT_BLACK);

  const float scaleX =
    static_cast<float>(CANVAS_WIDTH) /
    static_cast<float>(_mapWidth);

const float scaleY =
    static_cast<float>(CANVAS_HEIGHT) /
    static_cast<float>(_mapHeight);

    const float fitScale =
        (scaleX < scaleY) ? scaleX : scaleY;

    const float effectiveScale =
        fitScale * _mapZoom;

    if (effectiveScale <= 0.0f)
        return;

     /*
     * The map is rendered into a 480x480 canvas so that there is
     * sufficient image around the physical viewport for rotation.
     *
     * Camera following, however, is based on the actual 320x240
     * display viewport.  This means the player remains centred until
     * the map edge reaches the display edge, at every zoom level.
     */

    const float viewportSourceWidth =
        static_cast<float>(SCREEN_WIDTH) /
        effectiveScale;

    const float viewportSourceHeight =
        static_cast<float>(SCREEN_HEIGHT) /
        effectiveScale;

    float playerMapX =
        static_cast<float>(_mapWidth) * 0.5f;

    float playerMapY =
        static_cast<float>(_mapHeight) * 0.5f;

    if (_navMapConfigured &&
        _playerVisible)
    {
        if (!_navigationMap.GameToMapPixel(
                _playerPosition,
                playerMapX,
                playerMapY))
        {
            playerMapX =
                static_cast<float>(_mapWidth) * 0.5f;

            playerMapY =
                static_cast<float>(_mapHeight) * 0.5f;
        }
    }

    /*
     * Start by following the player.
     */
    float cameraMapX = playerMapX;
    float cameraMapY = playerMapY;

 
        /*
     * Clamp the camera using the actual display viewport.
     *
     * The camera follows the player until the map edge reaches
     * the corresponding screen edge.  After that the camera stops
     * and the player moves towards the screen edge.
     *
     * If the viewport is larger than the map, there is no valid
     * camera position that keeps the whole viewport inside the map.
     * In that case the camera is allowed to follow the player;
     * pixels outside the map are already rendered as TFT_BLACK.
     */

    if (viewportSourceWidth < static_cast<float>(_mapWidth))
    {
        const float halfWidth =
            viewportSourceWidth * 0.5f;

        const float minCameraX =
            halfWidth;

        const float maxCameraX =
            static_cast<float>(_mapWidth) - halfWidth;

        cameraMapX =
            constrain(
                cameraMapX,
                minCameraX,
                maxCameraX);
    }

    if (viewportSourceHeight < static_cast<float>(_mapHeight))
    {
        const float halfHeight =
            viewportSourceHeight * 0.5f;

        const float minCameraY =
            halfHeight;

        const float maxCameraY =
            static_cast<float>(_mapHeight) - halfHeight;

        cameraMapY =
            constrain(
                cameraMapY,
                minCameraY,
                maxCameraY);
    }

    _cameraMapX = cameraMapX;
    _cameraMapY = cameraMapY;
    

    const float canvasCentreX =
        (static_cast<float>(CANVAS_WIDTH) - 1.0f) * 0.5f;

    const float canvasCentreY =
        (static_cast<float>(CANVAS_HEIGHT) - 1.0f) * 0.5f;

    /*
     * Render the camera canvas.
     */
    for (int16_t y = 0; y < CANVAS_HEIGHT; ++y)
    {
        const float sourceY =
            cameraMapY +
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
                cameraMapX +
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

    const float mapRotation = _rotation;

        // DEBUG
if (_playerVisible)
{
  
    if (havePreviousPlayerPosition)
    {
         
        const double deltaEast =
            _playerPosition.east -
            previousPlayerPosition.east;

        const double deltaNorth =
            _playerPosition.north -
            previousPlayerPosition.north;

        const double distance =
            sqrt(
                deltaEast * deltaEast +
                deltaNorth * deltaNorth);
        if (distance > 0.01)
        {
            double movementBearing =
                atan2(
                    deltaEast,
                    deltaNorth) *
                180.0 /
                M_PI;

            if (movementBearing < 0.0)
                movementBearing += 360.0;

        }
    }

    previousPlayerPosition =
        _playerPosition;

    havePreviousPlayerPosition = true;
}

      //      __DEBUG

  

    _screenSprite.fillSprite(TFT_BLACK);

    _mapSprite.pushRotated(
        &_screenSprite,
        static_cast<int16_t>(mapRotation),
        TFT_BLACK);

    /*
     * Calculate where the player actually appears.
     *
     * If the camera is following the player, this is the centre.
     * If the camera is clamped by a map edge, the player moves towards
     * that edge instead.
     *
     * Rotate the player offset by the same map rotation so it remains
     * correctly registered with the map.
     */
    if (_playerVisible)
    {

        float playerCanvasX =
            canvasCentreX +
            (playerMapX - cameraMapX) *
                effectiveScale;

        float playerCanvasY =
            canvasCentreY +
            (playerMapY - cameraMapY) *
                effectiveScale;

        const float angle =
            mapRotation *
            0.01745329251994329576923690768489f;

        const float cosAngle = cosf(angle);
        const float sinAngle = sinf(angle);

        const float dx =
            playerCanvasX - canvasCentreX;

        const float dy =
            playerCanvasY - canvasCentreY;

        const float rotatedX =
            dx * cosAngle -
            dy * sinAngle;

        const float rotatedY =
            dx * sinAngle +
            dy * cosAngle;

        const int16_t playerScreenX =
            static_cast<int16_t>(
                lroundf(
                    canvasCentreX + rotatedX));

        const int16_t playerScreenY =
            static_cast<int16_t>(
                lroundf(
                    canvasCentreY + rotatedY));


        RenderNavigationPlayer(
            _screenSprite,
            playerScreenX,
            playerScreenY);

        
        }

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
    WidgetCollection::iterator widgetIterator = _widgets.begin();
    
    while(widgetIterator != _widgets.end())
    {
        /*
         * Calculate the final screen position.
         *
         * This is where the display applies map rotation
         * and zoom.
         */
        
        const WidgetRenderPosition position =
            TransformWidgetPosition(
                *(widgetIterator->second));


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
                const WidgetRenderPosition& position,
                float mapPixelsPerMetre)
                :
                _canvas(canvas),
                _position(position),
                _mapPixelsPerMetre(mapPixelsPerMetre)
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


            float GetMapScale() const override
            {
                return _position.scale;
            }


            float GetMapPixelsPerMetre() const override
            {
                return _mapPixelsPerMetre;
            }

        private:

            TFT_eSprite& _canvas;

            const WidgetRenderPosition& _position;

            float _mapPixelsPerMetre;
        };


    DisplayContext context(
        _screenSprite,
        position,
        _navigationMap.GetPixelsPerMetre());


        /*
         * The widget draws itself normally.
         *
         * It knows nothing about rotation.
         */
        widgetIterator->second->Render(
            context);

        widgetIterator++;
    }
}



/*
 * =============================================================
 * Update
 * =============================================================
 */

void ST7789VDisplay::Update(
    const GameCoordinate& playerPosition,
    float heading,
    float zoom,
    bool playerVisible)
{
    if (!_initialised)
    {
        return;
    }

    _playerPosition = playerPosition;
    _playerVisible = playerVisible;

    _rotation =
        _mapUpHeading -
        heading;

    _mapZoom = zoom;

    RenderMap();
    RenderWidgets();

    _screenSprite.pushSprite(
        -VIEWPORT_X,
        -VIEWPORT_Y);
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

void ST7789VDisplay::SetMapConfigData(const ConfigureMapCommand::MapConfigData& configData)
{
    _mapUpHeading = configData.mapUpHeading;    
}



bool ST7789VDisplay::SetBackground(const char* filename)
{
    PrintPSRAM("SET BACKGROUND");
 
    if(_mapImage != nullptr)
    {
        free(_mapImage);
        _mapImage = nullptr;
    }
    
    _backgroundTexture.Free();
    if(!_backgroundTexture.Load(LittleFS, filename))
    {
        Serial.printf("Failed to load background image %s", filename);
    }

    if (_backgroundTexture.GetData() == nullptr)
    {
        Serial.println(
            "ST7789VDisplay: failed to allocate background");

        return false;
    }



    _mapImage = _backgroundTexture.GetData();

    _mapWidth = _backgroundTexture.GetWidth();
    _mapHeight = _backgroundTexture.GetHeight();

    /*
     * A new background starts in its neutral state.
     */
    _rotation = 0.0f;
    _mapZoom = 1.0f;

    return true;
}

bool ST7789VDisplay::ConfigureNavigationMap(
    const char* mapFilename,
    double homeLatitude,
    double homeLongitude)
{
    _navMapConfigured = false;
    Serial.println("ConfigureNavigationMap");
    if (!_navigationMap.Load(mapFilename))
    {
        Serial.printf("Failed to load %s", mapFilename);
        return false;
    }
    else
    {
         _navMapConfigured = true;
    }

    if(_navMapConfigured &&!_navigationMap.Configure(
            homeLatitude,
            homeLongitude))
            {
                Serial.println("Failed to configure _navigationMap");
                _navMapConfigured = false;
            }
    _mapUpHeading = _navigationMap.GetMapUpHeading();
            Serial.printf(
    "Navigation map up heading = %.3f\n",
    _mapUpHeading);

    return _navMapConfigured; 
}

void ST7789VDisplay::SetWidgetColour(uint32_t widgetId, uint32_t colour)
{
    WidgetCollection::iterator widgetIterator = _widgets.find(widgetId);
    if(widgetIterator != _widgets.end())
    {
        widgetIterator->second->SetColour(colour);
    }
    else
    {
        Serial.println("Not found");
    }
}

void ST7789VDisplay::SetZoneGeometry(uint32_t zoneId, GameCoordinate newPos, float radius)
{
      WidgetCollection::iterator widgetIterator = _widgets.find(zoneId);
      if(widgetIterator != _widgets.end())
      {
           widgetIterator->second->SetGamePosition(newPos);
           widgetIterator->second->SetRadius(radius);
      } 
      else
      {
        Serial.printf("SetZoneZeometry could not find zone %lx\n", zoneId);
      }  
}
