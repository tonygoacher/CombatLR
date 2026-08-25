#include "Texture.h"

bool Texture::Load(
    fs::FS& filesystem,
    const char* filename)
{
    return _image.Load(
        filesystem,
        filename);
}

const uint16_t*
Texture::GetData() const
{
    return _image.GetData();
}

uint16_t
Texture::GetWidth() const
{
    return _image.GetWidth();
}

uint16_t
Texture::GetHeight() const
{
    return _image.GetHeight();
}

bool
Texture::IsLoaded() const
{
    return _image.IsLoaded();
}