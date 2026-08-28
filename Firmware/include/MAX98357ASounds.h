#pragma once

#include "ISounds.h"

#include <Arduino.h>
#include <FS.h>
#include <I2S.h>

#include <string>
#include <vector>

class MAX98357ASounds : public ISounds
{
public:
    /*
     * ALL game sound effects must be:
     *
     *   - uncompressed PCM WAV
     *   - 22050 Hz
     *   - 16-bit
     *   - mono
     *
     * 22050 Hz / 16-bit / mono requires only 44100 bytes
     * of PCM data per second.
     */
    static constexpr uint32_t SampleRate = 16000;
    static constexpr uint16_t BitsPerSample = 16;
    static constexpr uint16_t Channels = 1;

    /*
     * Temporary internal-RAM transfer buffer.
     *
     * The complete cached sample is stored in PSRAM.
     * I2S/DMA transfers data from this staging buffer.
     */
    static constexpr size_t TransferBufferSize = 4096;

    MAX98357ASounds(
        fs::FS& filesystem,
        int bclkPin,
        int lrcPin,
        int dataPin);

    ~MAX98357ASounds() override;

    bool Begin();

    bool Play(const char* filename) override;
    bool CacheSample(const char* filename) override;
    bool UnCacheSample(const char* filename) override;
    bool PlayCached(const char* filename) override;

    void Stop() override;
    bool HasFinished() const override;
    void SetVolume(float volume) override;

private:
    struct WavInfo
    {
        uint32_t dataOffset = 0;
        uint32_t dataSize = 0;
    };

    struct CachedSample
    {
        std::string filename;
        uint8_t* data = nullptr;
        size_t size = 0;
    };

    static void AudioTaskEntry(void* parameter);
    static void IRAM_ATTR OnTransmit();

    void AudioTask();

    bool LoadWavFile(
        const char* filename,
        uint8_t*& data,
        size_t& size);

    bool ReadWavInfo(
        File& file,
        WavInfo& info);

    void ApplyVolume(
        int16_t* samples,
        size_t sampleCount);

    CachedSample* FindCachedSample(const char* filename);
    const CachedSample* FindCachedSample(const char* filename) const;

    void ReleaseUncachedSample();

private:
    fs::FS& _filesystem;
    I2SClass _i2s;

    int _bclkPin;
    int _lrcPin;
    int _dataPin;

    TaskHandle_t _audioTask;

    const uint8_t* _playbackData;
    size_t _playbackSize;
    size_t _playbackPosition;

    uint8_t* _uncachedData;
    size_t _uncachedSize;

    volatile bool _playing;
    volatile bool _finished;
    volatile bool _stopRequested;

    /*
     * Set once the final block has been submitted.
     * The I2S transmit callback is used to detect completion.
     */
    volatile bool _waitingForFinalTransmit;
    volatile bool _transmitComplete;

    float _volume;

    std::vector<CachedSample> _cache;

    bool _initialised;

    /*
     * The installed Arduino I2S library exposes a global I2SClass
     * instance and internally uses the ESP-IDF I2S driver/DMA.
     */
    static MAX98357ASounds* _instance;
};
