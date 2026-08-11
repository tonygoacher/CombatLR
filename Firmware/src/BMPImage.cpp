#include "BMPImage.h"

#include <stdlib.h>
#include <string.h>


BMPImage::BMPImage()
    :
    _data(nullptr),
    _width(0),
    _height(0)
{
}


BMPImage::~BMPImage()
{
    Free();
}


/*
 * =============================================================
 * Load
 * =============================================================
 */

bool BMPImage::Load(
    fs::FS& filesystem,
    const char* filename)
{
    /*
     * Make sure any previous image is released.
     */
    Free();


    File file =
        filesystem.open(
            filename,
            FILE_READ);


    if (!file)
    {
        Serial.printf(
            "BMPImage: unable to open %s\n",
            filename);

        return false;
    }


    /*
     * ---------------------------------------------------------
     * BITMAPFILEHEADER
     * ---------------------------------------------------------
     *
     * Offset  Size
     *
     * 0       2       Signature
     * 2       4       File size
     * 6       2       Reserved
     * 8       2       Reserved
     * 10      4       Pixel data offset
     */

    uint16_t signature;

    if (!Read16(file, signature))
    {
        file.close();
        return false;
    }


    if (signature != 0x4D42)
    {
        Serial.println(
            "BMPImage: not a BMP file");

        file.close();
        return false;
    }


    uint32_t fileSize;

    if (!Read32(file, fileSize))
    {
        file.close();
        return false;
    }


    uint16_t reserved1;
    uint16_t reserved2;

    if (!Read16(file, reserved1) ||
        !Read16(file, reserved2))
    {
        file.close();
        return false;
    }


    uint32_t pixelOffset;

    if (!Read32(file, pixelOffset))
    {
        file.close();
        return false;
    }


    /*
     * ---------------------------------------------------------
     * BITMAPINFOHEADER
     * ---------------------------------------------------------
     */

    uint32_t headerSize;

    if (!Read32(file, headerSize))
    {
        file.close();
        return false;
    }


    /*
     * This implementation uses the classic 40-byte
     * BITMAPINFOHEADER.
     */
    if (headerSize < 40)
    {
        Serial.println(
            "BMPImage: unsupported BMP header");

        file.close();
        return false;
    }


    int32_t width;
    int32_t height;


    if (!ReadSigned32(file, width) ||
        !ReadSigned32(file, height))
    {
        file.close();
        return false;
    }


    uint16_t planes;
    uint16_t bitsPerPixel;


    if (!Read16(file, planes) ||
        !Read16(file, bitsPerPixel))
    {
        file.close();
        return false;
    }


    if (planes != 1)
    {
        Serial.println(
            "BMPImage: invalid plane count");

        file.close();
        return false;
    }


    /*
     * We support 24-bit and 32-bit uncompressed BMPs.
     */
    if (bitsPerPixel != 24 &&
        bitsPerPixel != 32)
    {
        Serial.printf(
            "BMPImage: unsupported colour depth: %u\n",
            bitsPerPixel);

        file.close();
        return false;
    }


    uint32_t compression;

    if (!Read32(file, compression))
    {
        file.close();
        return false;
    }


    /*
     * BI_RGB = 0
     *
     * We deliberately don't handle compressed BMPs here.
     */
    if (compression != 0)
    {
        Serial.printf(
            "BMPImage: unsupported compression: %lu\n",
            static_cast<unsigned long>(compression));

        file.close();
        return false;
    }


    uint32_t imageSize;
    uint32_t xPixelsPerMeter;
    uint32_t yPixelsPerMeter;
    uint32_t coloursUsed;
    uint32_t importantColours;


    if (!Read32(file, imageSize) ||
        !Read32(file, xPixelsPerMeter) ||
        !Read32(file, yPixelsPerMeter) ||
        !Read32(file, coloursUsed) ||
        !Read32(file, importantColours))
    {
        file.close();
        return false;
    }


    /*
     * A negative BMP height means the bitmap is stored
     * top-to-bottom rather than the normal bottom-to-top.
     */
    const bool topDown =
        height < 0;


    const int32_t absoluteHeight =
        topDown
            ? -height
            : height;


    if (width <= 0 ||
        absoluteHeight <= 0 ||
        width > 65535 ||
        absoluteHeight > 65535)
    {
        Serial.println(
            "BMPImage: invalid image dimensions");

        file.close();
        return false;
    }


    _width =
        static_cast<uint16_t>(width);

    _height =
        static_cast<uint16_t>(absoluteHeight);


    /*
     * ---------------------------------------------------------
     * Skip any additional header bytes.
     * ---------------------------------------------------------
     *
     * We have consumed 40 bytes of the DIB header.
     */
    if (headerSize > 40)
    {
        if (!file.seek(
                14 + headerSize))
        {
            file.close();
            return false;
        }
    }


    /*
     * Allocate RGB565 output in PSRAM.
     */
    if (!AllocateBuffer())
    {
        Serial.println(
            "BMPImage: unable to allocate image buffer");

        file.close();

        return false;
    }


    /*
     * ---------------------------------------------------------
     * Calculate source row size.
     * ---------------------------------------------------------
     *
     * BMP rows are padded to a 4-byte boundary.
     */
    const uint32_t bytesPerPixel =
        bitsPerPixel / 8;


    const uint32_t unpaddedRowSize =
        static_cast<uint32_t>(_width) *
        bytesPerPixel;


    const uint32_t rowSize =
        (unpaddedRowSize + 3) & ~3U;


    /*
     * Allocate one source row in normal RAM.
     *
     * This is tiny compared with the complete image and avoids
     * requiring another large PSRAM buffer.
     */
    uint8_t* row =
        static_cast<uint8_t*>(
            malloc(rowSize));


    if (row == nullptr)
    {
        Serial.println(
            "BMPImage: unable to allocate row buffer");

        Free();

        file.close();

        return false;
    }


    /*
     * ---------------------------------------------------------
     * Read image rows.
     * ---------------------------------------------------------
     */

    for (uint16_t y = 0;
         y < _height;
         ++y)
    {
        /*
         * BMP files are normally bottom-up.
         *
         * Convert the source row into our top-down output.
         */
        uint16_t destinationY;


        if (topDown)
        {
            destinationY = y;
        }
        else
        {
            destinationY =
                _height - 1 - y;
        }


        /*
         * Seek rather than relying on the current position.
         *
         * This also makes the row ordering explicit.
         */
        const uint32_t sourceOffset =
            pixelOffset +
            (static_cast<uint32_t>(y) *
             rowSize);


        if (!file.seek(sourceOffset))
        {
            free(row);

            Free();

            file.close();

            return false;
        }


        const size_t bytesRead =
            file.read(
                row,
                rowSize);


        if (bytesRead != rowSize)
        {
            Serial.println(
                "BMPImage: truncated pixel data");

            free(row);

            Free();

            file.close();

            return false;
        }


        /*
         * Convert each BGR/BGRA pixel to RGB565.
         */
        for (uint16_t x = 0;
             x < _width;
             ++x)
        {
            const uint32_t sourceIndex =
                static_cast<uint32_t>(x) *
                bytesPerPixel;


            const uint8_t blue =
                row[sourceIndex + 0];


            const uint8_t green =
                row[sourceIndex + 1];


            const uint8_t red =
                row[sourceIndex + 2];


            _data[
                static_cast<size_t>(destinationY) *
                _width +
                x] =
                ConvertRGB565(
                    red,
                    green,
                    blue);
        }
    }


    free(row);

    file.close();


    Serial.printf(
        "BMPImage: loaded %ux%u RGB565 image\n",
        _width,
        _height);


    return true;
}


/*
 * =============================================================
 * Free
 * =============================================================
 */

void BMPImage::Free()
{
    ReleaseBuffer();

    _width = 0;
    _height = 0;
}


/*
 * =============================================================
 * GetData
 * =============================================================
 */

const uint16_t* BMPImage::GetData() const
{
    return _data;
}


/*
 * =============================================================
 * GetWidth
 * =============================================================
 */

uint16_t BMPImage::GetWidth() const
{
    return _width;
}


/*
 * =============================================================
 * GetHeight
 * =============================================================
 */

uint16_t BMPImage::GetHeight() const
{
    return _height;
}


/*
 * =============================================================
 * IsLoaded
 * =============================================================
 */

bool BMPImage::IsLoaded() const
{
    return _data != nullptr;
}


/*
 * =============================================================
 * Read16
 * =============================================================
 *
 * BMP is little-endian.
 */

bool BMPImage::Read16(
    File& file,
    uint16_t& value)
{
    uint8_t buffer[2];


    if (file.read(
            buffer,
            sizeof(buffer)) != sizeof(buffer))
    {
        return false;
    }


    value =
        static_cast<uint16_t>(buffer[0]) |
        (static_cast<uint16_t>(buffer[1]) << 8);


    return true;
}


/*
 * =============================================================
 * Read32
 * =============================================================
 */

bool BMPImage::Read32(
    File& file,
    uint32_t& value)
{
    uint8_t buffer[4];


    if (file.read(
            buffer,
            sizeof(buffer)) != sizeof(buffer))
    {
        return false;
    }


    value =
        static_cast<uint32_t>(buffer[0]) |
        (static_cast<uint32_t>(buffer[1]) << 8) |
        (static_cast<uint32_t>(buffer[2]) << 16) |
        (static_cast<uint32_t>(buffer[3]) << 24);


    return true;
}


/*
 * =============================================================
 * ReadSigned32
 * =============================================================
 */

bool BMPImage::ReadSigned32(
    File& file,
    int32_t& value)
{
    uint32_t unsignedValue;


    if (!Read32(
            file,
            unsignedValue))
    {
        return false;
    }


    memcpy(
        &value,
        &unsignedValue,
        sizeof(value));


    return true;
}


/*
 * =============================================================
 * ConvertRGB565
 * =============================================================
 */

uint16_t BMPImage::ConvertRGB565(
    uint8_t red,
    uint8_t green,
    uint8_t blue)
{
    return
        static_cast<uint16_t>(
            ((red & 0xF8) << 8) |
            ((green & 0xFC) << 3) |
            (blue >> 3));
}


/*
 * =============================================================
 * AllocateBuffer
 * =============================================================
 */

bool BMPImage::AllocateBuffer()
{
    const size_t bytes =
        static_cast<size_t>(_width) *
        static_cast<size_t>(_height) *
        sizeof(uint16_t);


    /*
     * ps_malloc() allocates from PSRAM when available.
     */
    _data =
        static_cast<uint16_t*>(
            ps_malloc(bytes));


    return _data != nullptr;
}


/*
 * =============================================================
 * ReleaseBuffer
 * =============================================================
 */

void BMPImage::ReleaseBuffer()
{
    if (_data != nullptr)
    {
        free(_data);

        _data = nullptr;
    }
}