#include "IWidget.h"
#include "TFT_eSPI.h"
#include "IDisplayContext.h"

class TestWidget : public IWidget
{
public:
    TestWidget(
        WidgetCoordinateSpace space,
        float x,
        float y,
        uint16_t col
    )
        :
        _space(space),
        _x(x),
        _y(y),
        _col(col)
    {
    }

    WidgetCoordinateSpace GetCoordinateSpace() const override
    {
        return _space;
    }

    WidgetPosition GetPosition() const override
    {
        return {_x, _y};
    }

    void Render(IDisplayContext& context) override
    {
        TFT_eSprite& canvas = context.GetCanvas();

        const auto& p = context.GetPosition();

        canvas.fillCircle(
            static_cast<int16_t>(p.x),
            static_cast<int16_t>(p.y),
            12,
            _col);
    }

private:
    WidgetCoordinateSpace _space;
    float _x;
    float _y;
    uint16_t _col;
};