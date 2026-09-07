#include "graphics/TextureManager.h"

TextureManager::TextureManager(
    fs::FS& filesystem)
    :
    _filesystem(filesystem)
{
}

Texture*
TextureManager::GetTexture(
    const char* filename)
{
    if (filename == nullptr)
        return nullptr;

    std::map<std::string, Texture*>::iterator
        textureIterator =
            _textures.find(filename);

    if (textureIterator != _textures.end())
    {
        return textureIterator->second;
    }

    Texture* texture =
        new Texture();

    if (!texture->Load(
            _filesystem,
            filename))
    {
        Serial.print("Cannot find texture "); Serial.println(filename);
        delete texture;
        return nullptr;
    }

    _textures.insert(
        std::pair<std::string, Texture*>(
            filename,
            texture));

    return texture;
}

void TextureManager::FreeTexture(const char* filename)
{
    if (filename == nullptr)
        return;

    std::map<std::string, Texture*>::iterator
        textureIterator =
            _textures.find(filename);

    if (textureIterator != _textures.end())
    {
        delete(textureIterator->second);
        _textures.erase(textureIterator);
    }
}