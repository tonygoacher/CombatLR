#include "widgets/UIWidget.h"
#include "TFT_eSPI.h"
#include "IDisplayContext.h"
#include "TextureManager.h"
#include "widgets/UIWidgetIds.h"
#include "widgets/UIWidgetIds.h"
#include <LittleFS.h>





UIWidget::UIWidget(
    uint32_t id,
     const char * const filename,
    const WidgetPosition& position)
    :
    Widget(id, WidgetType::UI),
    _position(position)
{
    strcpy(_filename, filename);
    _texture.Load(LittleFS, filename);
}



UIWidget::~UIWidget()
{
   _texture.Free();
}


WidgetCoordinateSpace
UIWidget::GetCoordinateSpace() const
{
    return WidgetCoordinateSpace::Screen;
}

WidgetPosition
UIWidget::GetPosition() const
{
    return _position;
}

GameCoordinate
UIWidget::GetGamePosition() const
{
    return {};
}

void UIWidget::Render(
    IDisplayContext& context)
{
    context.DrawTexture(_texture, TFT_BLUE);
}