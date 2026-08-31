#include "widgets/TextWidget.h"
#include "IDisplayContext.h"


void TextWidget::Render(IDisplayContext& context)
{
TFT_eSprite& canvas = context.GetCanvas();

canvas.fillRect(150, 190, 100, 30, TFT_RED);

canvas.setTextColor(TFT_WHITE);
canvas.setTextFont(2);
canvas.drawString("HELLO", 160, 200);
}