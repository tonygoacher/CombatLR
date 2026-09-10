#pragma once

#include "ISounds.h"

#include <Arduino.h>
#include <FS.h>
#include <ESP_I2S.h>

#include <string>

class MAX98357ASounds : public ISounds
{
public:
    static constexpr uint32_t SAMPLE_RATE = 16000;
    static constexpr uint16_t BITS_PER_SAMPLE = 16;
    static constexpr size_t TRANSFER_BUFFER_SIZE = 256;

    MAX98357ASounds(
        fs::FS& filesystem,
        int bclkPin,
        int lrcPin,
        int dataPin);

    ~MAX98357ASounds() override;

    bool Begin();

    bool Play(const char* filename) override;
    bool PlayPriority(const char* filename) override;

    bool CacheSample(const char* filename) override;
    bool PlayCached() override;
    bool UnCacheSample() override;

    void Stop() override;

    bool HasFinished() const override;

    void SetVolume(float volume) override;

private:
    struct CachedSample
    {
        std::string filename;
        uint8_t* data;
        size_t size;
    };

    bool LoadWav(
        const char* filename,
        uint8_t*& data,
        size_t& size);

    bool ParseWav(
        File& file,
        size_t& dataOffset,
        size_t& dataSize);

    void AudioTask();

    static void AudioTaskEntry(void* parameter);

    CachedSample* FindCachedSample(
        const char* filename);

    const CachedSample* FindCachedSample(
        const char* filename) const;

    void ReleaseFileSample();

    void ReleaseCachedSample();

    void ApplyVolume(
        int16_t* samples,
        size_t sampleCount);

private:
    fs::FS& _filesystem;

    const int _bclkPin;
    const int _lrcPin;
    const int _dataPin;

    I2SClass _i2s;

    TaskHandle_t _audioTask;
    SemaphoreHandle_t _mutex;

    /*
     * There is only one cached sample.
     */
    CachedSample* _cachedSample;

    /*
     * Cached playback state.
     *
     * This is independent of file playback.
     */
    const uint8_t* _cachedData;
    size_t _cachedSize;
    size_t _cachedPosition;
    volatile bool _cachedPlaying;

    /*
     * File playback state.
     *
     * _fileOwnedData owns the PSRAM allocation.
     */
    const uint8_t* _fileData;
    size_t _fileSize;
    size_t _filePosition;
    uint8_t* _fileOwnedData;

    /*
     * True when the current file playback is priority
     * playback.
     */
    volatile bool _priorityPlaying;

    /*
     * True while either normal or priority file playback
     * is active.
     */
    volatile bool _filePlaying;

    /*
     * HasFinished() refers only to the file channel.
     */
    volatile bool _fileFinished;

    float _volume;

    bool _initialised;
};