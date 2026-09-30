#include "widgets/TextureWidget.h"
#include "graphics/TextureManager.h"
#include <LittleFS.h>

#include <Arduino.h>

TextureWidget::TextureWidget(
    uint32_t id,
    TextureManager& textureManager,
    const WidgetPosition& position,
    const char* filename)
    :
    Widget(
        id,
        WidgetType::Texture),
    _position(position),
    _texture(nullptr),
    _textureManager(textureManager)
{
    SetTexture(filename);
}


TextureWidget::~TextureWidget()
{
    if (_texture != nullptr)
    {
        delete _texture;
        _texture = nullptr;
    }
}


WidgetCoordinateSpace
TextureWidget::GetCoordinateSpace() const
{
    return WidgetCoordinateSpace::Screen;
}


WidgetPosition
TextureWidget::GetPosition() const
{
    return _position;
}


GameCoordinate
TextureWidget::GetGamePosition() const
{
    return GameCoordinate();
}


bool TextureWidget::SetTexture(
    const char* filename)
{
    if (filename == nullptr ||
        filename[0] == '\0')
    {
        return false;
    }

    /*
     * Load the new texture first.
     *
     * This means a failed load does not destroy
     * the texture which is currently being displayed.
     */
    Texture* newTexture =
        new Texture();

    if (newTexture == nullptr)
    {
        return false;
    }

    if (!newTexture->Load(LittleFS,filename))
    {
        delete newTexture;
        return false;
    }

    /*
     * New texture is valid. Now the old texture
     * can safely be released.
     */
    if (_texture != nullptr)
    {
        delete _texture;
    }

    _texture = newTexture;

    return true;
}


void TextureWidget::Render(
    IDisplayContext& context)
{
    if (_texture == nullptr ||
        !_texture->IsLoaded())
    {
        return;
    }

    /*
     * Screen widgets are already in screen coordinates,
     * so use the position supplied by the display context.
     */
    context.DrawTexture(
        *_texture,
        TFT_BLACK);
}