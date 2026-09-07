#include "widgets/TextWidget.h"
#include "graphics/IDisplayContext.h"



TextWidget::~TextWidget()
{
    Serial.printf("TextWidget %d destroyed\n", GetId());
}


void TextWidget::Render(IDisplayContext& context)
{
    if (!_visible)
        return;


    TFT_eSprite& canvas =
        context.GetCanvas();

    const WidgetRenderPosition& position =
        context.GetPosition();

  
    canvas.setTextColor(
        _textColour,
        _backgroundColour);

    canvas.setTextDatum(
        _datum);

    canvas.setTextSize(_size);

    canvas.drawString(
        _text,
        static_cast<int16_t>(position.x),
        static_cast<int16_t>(position.y),
        _font);

}