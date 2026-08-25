#pragma once

#include "BMPImage.h"

class Texture
{
public:
    Texture() = default;
    ~Texture() = default;

    bool Load(
        fs::FS& filesystem,
        const char* filename);

    const uint16_t* GetData() const;

    uint16_t GetWidth() const;

    uint16_t GetHeight() const;

    bool IsLoaded() const;

private:
    BMPImage _image;
};