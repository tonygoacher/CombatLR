#pragma once

#include "ISounds.h"

#include <Arduino.h>
#include <FS.h>
#include <ESP_I2S.h>

#include <string>
#include <vector>

class MAX98357ASounds : public ISounds
{
public:
    /*
     * CombatLR audio format:
     *
     *   PCM WAV
     *   16000 Hz
     *   16-bit
     *   mono
     *
     * This is intentionally modest quality for a small mono
     * speaker and keeps PSRAM use and I2S bandwidth low.
     */
    static constexpr uint32_t SAMPLE_RATE = 16000;
    static constexpr uint16_t BITS_PER_SAMPLE = 16;

    /*
     * Temporary internal-RAM buffer used between PSRAM and I2S.
     * The complete sample itself remains in PSRAM.
     */
    static constexpr size_t TRANSFER_BUFFER_SIZE = 2048;

    MAX98357ASounds(
        fs::FS& filesystem,
        int bclkPin,
        int lrcPin,
        int dataPin);

    ~MAX98357ASounds() override;

    bool Begin();

    bool Play(const char* filename) override;
    bool PlayCached(const char* filename) override;

    bool CacheSample(const char* filename) override;
    bool UnCacheSample(const char* filename) override;

    void Stop() override;
    bool HasFinished() const override;
    void SetVolume(float volume) override;

private:

    uint8_t _transferBuffer[TRANSFER_BUFFER_SIZE];
    
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

    void ReleaseCurrentSample();

    void ApplyVolume(
        int16_t* samples,
        size_t sampleCount);

private:
    fs::FS& _filesystem;

    const int _bclkPin;
    const int _lrcPin;
    const int _dataPin;

    /*
     * Arduino-ESP32 3.x I2S implementation.
     *
     * There is deliberately no legacy I2S.h object and no
     * onTransmit callback. I2SClass owns the hardware/DMA path.
     */
    I2SClass _i2s;

    TaskHandle_t _audioTask;
    SemaphoreHandle_t _mutex;

    /*
     * Explicitly cached samples. Every sample is allocated from
     * PSRAM by CacheSample().
     */
    std::vector<CachedSample> _cachedSamples;

    /*
     * Current playback source.
     *
     * For Play(), _currentOwnedData owns the PSRAM allocation.
     * For PlayCached(), _currentData points into _cachedSamples.
     */
    const uint8_t* _currentData;
    size_t _currentSize;
    size_t _currentPosition;

    uint8_t* _currentOwnedData;

    volatile bool _playing;
    volatile bool _stopRequested;
    volatile bool _finished;

    float _volume;

    bool _initialised;
};
