#include "BMPImage.h"

#include <stdlib.h>
#include <string.h>

namespace
{
    constexpr uint32_t BI_RGB = 0;
    constexpr uint32_t BI_BITFIELDS = 3;

    constexpr uint32_t RGB565_RED_MASK   = 0x0000F800;
    constexpr uint32_t RGB565_GREEN_MASK = 0x000007E0;
    constexpr uint32_t RGB565_BLUE_MASK  = 0x0000001F;

    constexpr uint32_t MAX_DIB_HEADER_SIZE = 124;
}

BMPImage::BMPImage()
    : _data(nullptr),
      _width(0),
      _height(0)
{
}

BMPImage::~BMPImage()
{
    Free();
}

bool BMPImage::Load(
    fs::FS& filesystem,
    const char* filename)
{
    Free();

    File file = LittleFS.open(filename, FILE_READ);

    if (!file)
    {
        Serial.printf("BMPImage: unable to open %s\n", filename);
        return false;
    }

    // ---------------------------------------------------------------------
    // BITMAPFILEHEADER (14 bytes)
    // ---------------------------------------------------------------------

    uint16_t signature;
    if (!Read16(file, signature) || signature != 0x4D42)
    {
        Serial.println("BMPImage: not a BMP file");
        file.close();
        return false;
    }

    uint32_t fileSize;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t pixelOffset;

    if (!Read32(file, fileSize) ||
        !Read16(file, reserved1) ||
        !Read16(file, reserved2) ||
        !Read32(file, pixelOffset))
    {
        Serial.println("BMPImage: truncated file header");
        file.close();
        return false;
    }

    // ---------------------------------------------------------------------
    // DIB header
    //
    // We read the entire DIB header into memory. This is important because
    // BITMAPV4HEADER and BITMAPV5HEADER contain the RGB masks inside the
    // header, whereas a 40-byte BITMAPINFOHEADER with BI_BITFIELDS has the
    // masks immediately after the header.
    // ---------------------------------------------------------------------

    uint32_t headerSize;

    if (!Read32(file, headerSize))
    {
        Serial.println("BMPImage: unable to read DIB header size");
        file.close();
        return false;
    }

    if (headerSize < 40 || headerSize > MAX_DIB_HEADER_SIZE)
    {
        Serial.printf(
            "BMPImage: unsupported BMP header size: %lu\n",
            static_cast<unsigned long>(headerSize));

        file.close();
        return false;
    }

    uint8_t* dibHeader =
        static_cast<uint8_t*>(malloc(headerSize));

    if (dibHeader == nullptr)
    {
        Serial.println("BMPImage: unable to allocate DIB header");
        file.close();
        return false;
    }

    // The first DWORD of the DIB header (headerSize) was already consumed.
    dibHeader[0] = static_cast<uint8_t>(headerSize);
    dibHeader[1] = static_cast<uint8_t>(headerSize >> 8);
    dibHeader[2] = static_cast<uint8_t>(headerSize >> 16);
    dibHeader[3] = static_cast<uint8_t>(headerSize >> 24);

    if (headerSize > 4)
    {
        const size_t bytesRead =
            file.read(dibHeader + 4, headerSize - 4);

        if (bytesRead != headerSize - 4)
        {
            Serial.println("BMPImage: truncated DIB header");
            free(dibHeader);
            file.close();
            return false;
        }
    }

    // BITMAPINFOHEADER layout:
    //   +04 width
    //   +08 height
    //   +0C planes
    //   +0E bits per pixel
    //   +10 compression
    //   +14 image size
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    uint32_t compression;

    memcpy(&width,         dibHeader + 4,  sizeof(width));
    memcpy(&height,        dibHeader + 8,  sizeof(height));
    memcpy(&planes,        dibHeader + 12, sizeof(planes));
    memcpy(&bitsPerPixel,  dibHeader + 14, sizeof(bitsPerPixel));
    memcpy(&compression,   dibHeader + 16, sizeof(compression));

    free(dibHeader);

    // BMP is little-endian. ESP32 is little-endian, so the memcpy values
    // above are directly usable.

    if (planes != 1)
    {
        Serial.println("BMPImage: invalid plane count");
        file.close();
        return false;
    }

    if (bitsPerPixel != 16 &&
        bitsPerPixel != 24 &&
        bitsPerPixel != 32)
    {
        Serial.printf(
            "BMPImage: unsupported colour depth: %u\n",
            bitsPerPixel);

        file.close();
        return false;
    }

    // ---------------------------------------------------------------------
    // Determine the RGB masks for 16-bit images.
    // ---------------------------------------------------------------------

    uint32_t redMask   = RGB565_RED_MASK;
    uint32_t greenMask = RGB565_GREEN_MASK;
    uint32_t blueMask  = RGB565_BLUE_MASK;

    if (bitsPerPixel == 16)
    {
        if (compression == BI_RGB)
        {
            /*
             * For this loader, BI_RGB 16-bit BMPs are treated as RGB565.
             * This is the normal interpretation for the RGB565 files we
             * need to display.
             */
        }
        else if (compression == BI_BITFIELDS)
        {
            if (headerSize == 40)
            {
                /*
                 * BITMAPINFOHEADER + BI_BITFIELDS:
                 * the three masks immediately follow the 40-byte DIB
                 * header.
                 */
                if (!Read32(file, redMask) ||
                    !Read32(file, greenMask) ||
                    !Read32(file, blueMask))
                {
                    Serial.println(
                        "BMPImage: truncated RGB565 masks");

                    file.close();
                    return false;
                }
            }
            else
            {
                /*
                 * BITMAPV4HEADER / BITMAPV5HEADER:
                 *
                 * The masks are part of the DIB header itself:
                 *   offset 40: red mask
                 *   offset 44: green mask
                 *   offset 48: blue mask
                 *
                 * The file position is already immediately after the
                 * complete DIB header, so seek back to those offsets.
                 */
                if (!file.seek(
                        14 + 40)) // BMP file header + red-mask offset
                {
                    Serial.println(
                        "BMPImage: unable to seek to RGB masks");

                    file.close();
                    return false;
                }

                if (!Read32(file, redMask) ||
                    !Read32(file, greenMask) ||
                    !Read32(file, blueMask))
                {
                    Serial.println(
                        "BMPImage: unable to read RGB565 masks");

                    file.close();
                    return false;
                }
            }

            if (redMask != RGB565_RED_MASK ||
                greenMask != RGB565_GREEN_MASK ||
                blueMask != RGB565_BLUE_MASK)
            {
                Serial.printf(
                    "BMPImage: unsupported 16-bit pixel format: "
                    "%08lX/%08lX/%08lX\n",
                    static_cast<unsigned long>(redMask),
                    static_cast<unsigned long>(greenMask),
                    static_cast<unsigned long>(blueMask));

                file.close();
                return false;
            }
        }
        else
        {
            Serial.printf(
                "BMPImage: unsupported 16-bit compression: %lu\n",
                static_cast<unsigned long>(compression));

            file.close();
            return false;
        }
    }
    else
    {
        // 24-bit and 32-bit support only uncompressed BI_RGB.
        if (compression != BI_RGB)
        {
            Serial.printf(
                "BMPImage: unsupported %u-bit compression: %lu\n",
                bitsPerPixel,
                static_cast<unsigned long>(compression));

            file.close();
            return false;
        }
    }

    // ---------------------------------------------------------------------
    // Image dimensions
    // ---------------------------------------------------------------------

    const bool topDown = height < 0;
    const int32_t absoluteHeight = topDown ? -height : height;

    if (width <= 0 ||
        absoluteHeight <= 0 ||
        width > 65535 ||
        absoluteHeight > 65535)
    {
        Serial.println("BMPImage: invalid image dimensions");
        file.close();
        return false;
    }

    _width = static_cast<uint16_t>(width);
    _height = static_cast<uint16_t>(absoluteHeight);

    // ---------------------------------------------------------------------
    // Pixel data
    // ---------------------------------------------------------------------

    if (!file.seek(pixelOffset))
    {
        Serial.println("BMPImage: unable to seek to pixel data");
        file.close();
        return false;
    }

    if (!AllocateBuffer())
    {
        Serial.println("BMPImage: unable to allocate image buffer");
        file.close();
        return false;
    }

    const uint32_t bytesPerPixel =
        bitsPerPixel / 8;

    const uint32_t unpaddedRowSize =
        static_cast<uint32_t>(_width) * bytesPerPixel;

    // BMP rows are padded to a 4-byte boundary.
    const uint32_t rowSize =
        (unpaddedRowSize + 3U) & ~3U;

    uint8_t* row =
        static_cast<uint8_t*>(malloc(rowSize));

    if (row == nullptr)
    {
        Serial.println("BMPImage: unable to allocate row buffer");
        Free();
        file.close();
        return false;
    }

    for (uint16_t y = 0; y < _height; ++y)
    {
        /*
         * BMP normally stores rows bottom-to-top. A negative height means
         * the BMP is top-to-bottom.
         */
        const uint16_t destinationY =
            topDown ? y : (_height - 1 - y);

        const size_t bytesRead =
            file.read(row, rowSize);

        if (bytesRead != rowSize)
        {
            Serial.println("BMPImage: truncated pixel data");
            free(row);
            Free();
            file.close();
            return false;
        }

        for (uint16_t x = 0; x < _width; ++x)
        {
            const uint32_t sourceIndex =
                static_cast<uint32_t>(x) * bytesPerPixel;

            uint16_t pixel;

            if (bitsPerPixel == 16)
            {
                /*
                 * RGB565 is already in the required format.
                 * BMP stores the 16-bit value little-endian.
                 */
                pixel =
                    static_cast<uint16_t>(row[sourceIndex]) |
                    (static_cast<uint16_t>(
                        row[sourceIndex + 1]) << 8);
            }
            else
            {
                /*
                 * 24-bit BMP: BGR
                 * 32-bit BMP: BGRA
                 *
                 * Alpha is intentionally ignored.
                 */
                const uint8_t blue =
                    row[sourceIndex + 0];

                const uint8_t green =
                    row[sourceIndex + 1];

                const uint8_t red =
                    row[sourceIndex + 2];

                pixel =
                    ConvertRGB565(red, green, blue);
            }

            _data[
                static_cast<size_t>(destinationY) *
                _width + x] = pixel;
        }
    }

    free(row);
    file.close();

    Serial.printf(
        "BMPImage: loaded %ux%u RGB565 image %s\n",
        _width,
        _height,
        filename);

    return true;
}

void BMPImage::Free()
{
    ReleaseBuffer();
    _width = 0;
    _height = 0;
}

uint16_t* BMPImage::GetData() 
{
    return _data;
}

uint16_t BMPImage::GetWidth() const
{
    return _width;
}

uint16_t BMPImage::GetHeight() const
{
    return _height;
}

bool BMPImage::IsLoaded() const
{
    return _data != nullptr;
}

bool BMPImage::Read16(File& file, uint16_t& value)
{
    uint8_t buffer[2];

    if (file.read(buffer, sizeof(buffer)) != sizeof(buffer))
        return false;

    value =
        static_cast<uint16_t>(buffer[0]) |
        (static_cast<uint16_t>(buffer[1]) << 8);

    return true;
}

bool BMPImage::Read32(File& file, uint32_t& value)
{
    uint8_t buffer[4];

    if (file.read(buffer, sizeof(buffer)) != sizeof(buffer))
        return false;

    value =
        static_cast<uint32_t>(buffer[0]) |
        (static_cast<uint32_t>(buffer[1]) << 8) |
        (static_cast<uint32_t>(buffer[2]) << 16) |
        (static_cast<uint32_t>(buffer[3]) << 24);

    return true;
}

bool BMPImage::ReadSigned32(File& file, int32_t& value)
{
    uint32_t unsignedValue;

    if (!Read32(file, unsignedValue))
        return false;

    memcpy(&value, &unsignedValue, sizeof(value));

    return true;
}

uint16_t BMPImage::ConvertRGB565(
    uint8_t red,
    uint8_t green,
    uint8_t blue)
{
    return static_cast<uint16_t>(
        ((red & 0xF8) << 8) |
        ((green & 0xFC) << 3) |
        (blue >> 3));
}

bool BMPImage::AllocateBuffer()
{
    const size_t bytes =
        static_cast<size_t>(_width) *
        static_cast<size_t>(_height) *
        sizeof(uint16_t);

    _data =
        static_cast<uint16_t*>(ps_malloc(bytes));

    return _data != nullptr;
}

void BMPImage::ReleaseBuffer()
{
    if (_data != nullptr)
    {
        free(_data);
        _data = nullptr;
    }
}

uint32_t BMPImage::ExtractChannel(
    uint32_t pixel,
    uint32_t mask,
    uint8_t outputBits)
{
    if (mask == 0)
        return 0;

    uint8_t shift = 0;

    while ((mask & (1UL << shift)) == 0 && shift < 32)
        ++shift;

    const uint32_t value =
        (pixel & mask) >> shift;

    uint8_t maskBits = 0;
    uint32_t temp = mask >> shift;

    while (temp != 0)
    {
        ++maskBits;
        temp >>= 1;
    }

    if (maskBits == outputBits)
        return value;

    if (maskBits > outputBits)
        return value >> (maskBits - outputBits);

    const uint32_t maxInput =
        (1UL << maskBits) - 1;

    const uint32_t maxOutput =
        (1UL << outputBits) - 1;

    return (value * maxOutput + maxInput / 2) / maxInput;
}
