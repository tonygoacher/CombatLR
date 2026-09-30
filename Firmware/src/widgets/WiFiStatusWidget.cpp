#include "WiFi.h"
#include "widgets/WiFiStatusWidget.h"
#include "TFT_eSPI.h"
#include "graphics/IDisplayContext.h"
#include "Navigation.h"


namespace WiFiStatusFile
{
    static const char * const goodFile = "/wifigood.bmp";
    static const char * const badFile = "/wifibad.bmp";
};

WiFiStatusWidget::WiFiStatusWidget(
    uint32_t id,
    TextureManager& textureManager,
    const WidgetPosition& position)
    :
    Widget(id, WidgetType::UI),
    _position(position),
    _textureManager(textureManager),
    _state(WifiState::UnknownWifi)
{
    _ok = _textureManager.GetTexture(WiFiStatusFile::goodFile);
    _bad = _textureManager.GetTexture(WiFiStatusFile::badFile);
}


WiFiStatusWidget::~WiFiStatusWidget()
{
   _textureManager.FreeTexture(WiFiStatusFile::goodFile);
   _textureManager.FreeTexture(WiFiStatusFile::badFile);
}


WidgetCoordinateSpace
WiFiStatusWidget::GetCoordinateSpace() const
{
    return WidgetCoordinateSpace::Screen;
}

WidgetPosition
WiFiStatusWidget::GetPosition() const
{
    return _position;
}

GameCoordinate
WiFiStatusWidget::GetGamePosition() const
{
    return {};
}

void WiFiStatusWidget::Render(
    IDisplayContext& context)
{
    if(_ok == nullptr || _bad == nullptr)
    {
        return;
    }


    Texture* textureToPush = _ok;

    if (WiFi.status() != WL_CONNECTED)
    {
        textureToPush = _bad;
    }
    else
    {
        textureToPush = _ok;
    }
  

    context.DrawTexture(*textureToPush, TFT_BLACK);
}