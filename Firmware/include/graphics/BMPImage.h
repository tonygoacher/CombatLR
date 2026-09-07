#pragma once

#include <Arduino.h>
#include <LittleFS.h>
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
     *   - 16-bit RGB565, BI_RGB
     *   - 16-bit RGB565, BI_BITFIELDS
     *     (40-byte BITMAPINFOHEADER with masks following the header)
     *   - 16-bit RGB565, BI_BITFIELDS
     *     (BITMAPV4HEADER / BITMAPV5HEADER with masks in the header)
     *   - 24-bit uncompressed BGR
     *   - 32-bit uncompressed BGRA
     *
     * Pixels are converted/stored as RGB565 in PSRAM.
     * The returned image is always top-to-bottom, left-to-right.
     */
    bool Load(fs::FS& filesystem, const char* filename);

    void Free();

    uint16_t* GetData() const;

    uint16_t GetWidth() const;
    uint16_t GetHeight() const;

    bool IsLoaded() const;

private:
    uint16_t* _data;
    uint16_t _width;
    uint16_t _height;

    bool Read16(File& file, uint16_t& value);
    bool Read32(File& file, uint32_t& value);
    bool ReadSigned32(File& file, int32_t& value);

    uint16_t ConvertRGB565(
        uint8_t red,
        uint8_t green,
        uint8_t blue);

    bool AllocateBuffer();
    void ReleaseBuffer();

    static uint32_t ExtractChannel(
        uint32_t pixel,
        uint32_t mask,
        uint8_t outputBits);
};
