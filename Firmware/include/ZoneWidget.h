#pragma once

#include "IWidget.h"
#include "Zone.h"

class ZoneWidget : public IWidget
{
public:
    ZoneWidget(uint32_t id,  const Zone& zone, uint32_t colour);

    WidgetPosition GetPosition() const override;

    WidgetCoordinateSpace GetCoordinateSpace() const override;

    GameCoordinate GetGamePosition() const override;

    void Render(
        IDisplayContext& context) override;

private:
    const Zone& _zone;
    const uint32_t _colour;
};