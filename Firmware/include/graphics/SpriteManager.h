#pragma once

#include <stdint.h>
#include <string.h>

#include "TFT_eSPI.h"
#include "graphics/Texture.h"

struct SpriteSet
{
    TFT_eSprite* full = nullptr;
    TFT_eSprite* small = nullptr;
};

class SpriteManager
{
public:

    static constexpr uint16_t MAX_SPRITES = 32;
    static constexpr uint16_t MAX_FILENAME = 127;

    SpriteManager() = default;

    ~SpriteManager();

    SpriteSet* Get(
        TFT_eSprite& canvas,
        const char* filename,
        const Texture& texture);

private:

    struct Entry
    {
        bool used = false;

        char filename[MAX_FILENAME + 1];

        SpriteSet sprites;
    };

    Entry _entries[MAX_SPRITES];

    SpriteSet* CreateSprites(
        TFT_eSprite& canvas,
        Entry& entry,
        const Texture& texture);

    void DestroyEntry(
        Entry& entry);

    bool CreateSmallSprite(
        TFT_eSprite& sprite,
        const Texture& texture);
};