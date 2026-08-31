#include "widgets/GPSStatusWidget.h"
#include "TFT_eSPI.h"
#include "IDisplayContext.h"
#include "Navigation.h"


namespace GPSStatusFile
{
    static const char * const goodFile = "/satgood.bmp";
    static const char * const badFile = "/satbad.bmp";
};

GPSStatusWidget::GPSStatusWidget(
    uint32_t id,
    TextureManager& textureManager,
    const WidgetPosition& position,
    INavigation& navigation)
    :
    Widget(id, WidgetType::GPS),
    _position(position),
    _textureManager(textureManager),
    _navigation(navigation)
{
    _okSat = _textureManager.GetTexture(GPSStatusFile::goodFile);

    _badSat = _textureManager.GetTexture(GPSStatusFile::badFile);
}


GPSStatusWidget::~GPSStatusWidget()
{
   _textureManager.FreeTexture(GPSStatusFile::goodFile);
   _textureManager.FreeTexture(GPSStatusFile::badFile);
}


WidgetCoordinateSpace
GPSStatusWidget::GetCoordinateSpace() const
{
    return WidgetCoordinateSpace::Screen;
}

WidgetPosition
GPSStatusWidget::GetPosition() const
{
    return _position;
}

GameCoordinate
GPSStatusWidget::GetGamePosition() const
{
    return {};
}

void GPSStatusWidget::Render(
    IDisplayContext& context)
{
    if(_okSat == nullptr || _badSat == nullptr)
    {
        return;
    }


    Texture* textureToPush = _okSat;

    if(!_navigation.HasPosition())
    {
        textureToPush = _badSat;
    }
  

    context.DrawTexture(*textureToPush, TFT_BLUE);
}