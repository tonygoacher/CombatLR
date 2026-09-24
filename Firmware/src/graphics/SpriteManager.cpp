#include "graphics/SpriteManager.h"

SpriteManager::~SpriteManager()
{
    for (uint16_t i = 0;
         i < MAX_SPRITES;
         ++i)
    {
        if (_entries[i].used)
        {
            DestroyEntry(_entries[i]);
        }
    }
}


SpriteSet* SpriteManager::Get(
    TFT_eSprite& canvas,
    const char* filename,
    const Texture& texture)
{
    /*
     * First look for an existing sprite set.
     */
    for (uint16_t i = 0;
         i < MAX_SPRITES;
         ++i)
    {
        if (!_entries[i].used)
        {
            continue;
        }

        if (strcmp(
                _entries[i].filename,
                filename) == 0)
        {
            return &_entries[i].sprites;
        }
    }


    /*
     * Find a free entry.
     */
    for (uint16_t i = 0;
         i < MAX_SPRITES;
         ++i)
    {
        if (_entries[i].used)
        {
            continue;
        }

        Entry& entry = _entries[i];

        entry.used = true;

        strncpy(
            entry.filename,
            filename,
            MAX_FILENAME);

        entry.filename[MAX_FILENAME] = '\0';

        return CreateSprites(
            canvas,
            entry,
            texture);
    }

    return nullptr;
}


SpriteSet* SpriteManager::CreateSprites(
    TFT_eSprite& canvas,
    Entry& entry,
    const Texture& texture)
{
    const uint16_t width =
        texture.GetWidth();

    const uint16_t height =
        texture.GetHeight();


    /*
     * Full-size sprite.
     */
    entry.sprites.full =
        new TFT_eSprite(&canvas);

    if (entry.sprites.full == nullptr)
    {
        DestroyEntry(entry);
        return nullptr;
    }

    if (entry.sprites.full->createSprite(
            width,
            height) == nullptr)
    {
        DestroyEntry(entry);
        return nullptr;
    }

    entry.sprites.full->setSwapBytes(true);

    entry.sprites.full->pushImage(
        0,
        0,
        width,
        height,
        texture.GetData());


    /*
     * Half-size sprite.
     */
    const uint16_t smallWidth =
        (width + 1) / 2;

    const uint16_t smallHeight =
        (height + 1) / 2;

    entry.sprites.small =
        new TFT_eSprite(&canvas);

    if (entry.sprites.small == nullptr)
    {
        DestroyEntry(entry);
        return nullptr;
    }

    if (entry.sprites.small->createSprite(
            smallWidth,
            smallHeight) == nullptr)
    {
        DestroyEntry(entry);
        return nullptr;
    }

    entry.sprites.small->setSwapBytes(true);

    if (!CreateSmallSprite(
            *entry.sprites.small,
            texture))
    {
        DestroyEntry(entry);
        return nullptr;
    }

    return &entry.sprites;
}


bool SpriteManager::CreateSmallSprite(
    TFT_eSprite& sprite,
    const Texture& texture)
{
    const uint16_t width =
        texture.GetWidth();

    const uint16_t height =
        texture.GetHeight();

    const uint16_t* source =
        texture.GetData();

    const uint16_t smallWidth =
        (width + 1) / 2;

    const uint16_t smallHeight =
        (height + 1) / 2;


    /*
     * Generate the half-size image once.
     *
     * Average each 2x2 block in RGB565 space.
     */
    for (uint16_t y = 0;
         y < smallHeight;
         ++y)
    {
        for (uint16_t x = 0;
             x < smallWidth;
             ++x)
        {
            uint32_t red = 0;
            uint32_t green = 0;
            uint32_t blue = 0;

            uint32_t count = 0;


            for (uint16_t dy = 0;
                 dy < 2;
                 ++dy)
            {
                for (uint16_t dx = 0;
                     dx < 2;
                     ++dx)
                {
                    const uint16_t sourceX =
                        x * 2 + dx;

                    const uint16_t sourceY =
                        y * 2 + dy;

                    if (sourceX >= width ||
                        sourceY >= height)
                    {
                        continue;
                    }

                    const uint16_t pixel =
                        source[
                            sourceY * width +
                            sourceX];


                    red +=
                        (pixel >> 11) & 0x1F;

                    green +=
                        (pixel >> 5) & 0x3F;

                    blue +=
                        pixel & 0x1F;

                    ++count;
                }
            }


            if (count != 0)
            {
                red /= count;
                green /= count;
                blue /= count;
            }


            const uint16_t pixel =
                static_cast<uint16_t>(
                    (red << 11) |
                    (green << 5) |
                    blue);


            sprite.drawPixel(
                x,
                y,
                pixel);
        }
    }

    return true;
}


void SpriteManager::DestroyEntry(
    Entry& entry)
{
    if (entry.sprites.full != nullptr)
    {
        entry.sprites.full->deleteSprite();

        delete entry.sprites.full;

        entry.sprites.full = nullptr;
    }

    if (entry.sprites.small != nullptr)
    {
        entry.sprites.small->deleteSprite();

        delete entry.sprites.small;

        entry.sprites.small = nullptr;
    }

    entry.used = false;

    entry.filename[0] = '\0';
}