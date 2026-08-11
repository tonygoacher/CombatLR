#pragma once

#include <Arduino.h>
#include <FS.h>
#include <stdint.h>


class BMPImage
{
public:

    BMPImage();
    ~BMPImage();


    /*
     * Load a BMP file from the supplied filesystem.
     *
     * Supported:
     *   - 24-bit BMP
     *   - 32-bit BMP
     *   - uncompressed BI_RGB
     *
     * The converted RGB565 image is stored in PSRAM.
     */
    bool Load(
        fs::FS& filesystem,
        const char* filename);


    /*
     * Free the image from memory.
     */
    void Free();


    /*
     * Return RGB565 pixel data.
     *
     * Pixels are stored left-to-right, top-to-bottom.
     */
    const uint16_t* GetData() const;


    uint16_t GetWidth() const;

    uint16_t GetHeight() const;


    bool IsLoaded() const;


private:

    uint16_t* _data;

    uint16_t _width;
    uint16_t _height;


    bool Read16(
        File& file,
        uint16_t& value);


    bool Read32(
        File& file,
        uint32_t& value);


    bool ReadSigned32(
        File& file,
        int32_t& value);


    uint16_t ConvertRGB565(
        uint8_t red,
        uint8_t green,
        uint8_t blue);


    bool AllocateBuffer();


    void ReleaseBuffer();
};