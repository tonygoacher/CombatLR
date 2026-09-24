#pragma once

#include "Widget.h"
#include "graphics/Texture.h"
#include "graphics/TextureManager.h"
#include "PickupTypes.h"

class MapWidget : public Widget
{
public:

    MapWidget(
        uint32_t id,
        TextureManager& textureManager,
        const GameCoordinate& position,
        const char* filename,
        PickupTypes pickupType);

    ~MapWidget();        

    WidgetCoordinateSpace
        GetCoordinateSpace() const override;

    WidgetPosition
        GetPosition() const override;

    GameCoordinate
        GetGamePosition() const override;

    void Render(
        IDisplayContext& context) override;
protected:
    Texture* _texture;  
    uint16_t _colour;
private:

    GameCoordinate _position;

  
    TextureManager& _textureManager;
    char    _filename[128];
    PickupTypes _pickupType;
 
};