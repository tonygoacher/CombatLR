#pragma once

#include "IWidget.h"
class ZoneWidget : public IWidget
{
public:
    ZoneWidget(
        uint32_t id,
        const GameCoordinate& position,
        const uint16_t radius,
        uint32_t colour);

    WidgetPosition GetPosition() const override;

    WidgetCoordinateSpace GetCoordinateSpace() const override;

    GameCoordinate GetGamePosition() const override;

    void  SetGamePosition(GameCoordinate newPos)override { _position = newPos;}  
    void  SetRadius(uint16_t newRadius) override { _radius = newRadius;}  

    void Render(
        IDisplayContext& context) override;

    void SetColour(uint32_t colour);

    uint32_t GetColour() const;

private:
    GameCoordinate _position;
    uint32_t _colour;
    uint16_t _radius;
};
