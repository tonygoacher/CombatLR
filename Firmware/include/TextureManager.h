#pragma once

#include <Arduino.h>
#include <FS.h>
#include <map>
#include <string>

#include "Texture.h"

class TextureManager
{
public:

    explicit TextureManager(
        fs::FS& filesystem);

    Texture* GetTexture(
        const char* filename);

private:

    TextureManager(
        const TextureManager&) = delete;

    TextureManager& operator=(
        const TextureManager&) = delete;

    fs::FS& _filesystem;

    std::map<std::string, Texture*> _textures;
};