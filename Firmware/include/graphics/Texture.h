#pragma once

#include "graphics/BMPImage.h"

class Texture
{
public:
    Texture() = default;
    ~Texture();

    bool Load(
        fs::FS& filesystem,
        const char* filename);

    uint16_t* GetData() const;

    uint16_t GetWidth() const;

    uint16_t GetHeight() const;

    bool IsLoaded() const;

    void Free() {_image.Free();};
private:
    BMPImage _image;
};