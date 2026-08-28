#include "MAX98357ASounds.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

MAX98357ASounds*
    MAX98357ASounds::_instance = nullptr;

namespace
{
    constexpr uint32_t RiffId = 0x46464952; // "RIFF"
    constexpr uint32_t WaveId = 0x45564157; // "WAVE"
    constexpr uint32_t FmtId  = 0x20746D66; // "fmt "
    constexpr uint32_t DataId = 0x61746164; // "data"

    uint32_t ReadUInt32(const uint8_t* data)
    {
        return
            static_cast<uint32_t>(data[0]) |
            (static_cast<uint32_t>(data[1]) << 8) |
            (static_cast<uint32_t>(data[2]) << 16) |
            (static_cast<uint32_t>(data[3]) << 24);
    }

    uint16_t ReadUInt16(const uint8_t* data)
    {
        return
            static_cast<uint16_t>(data[0]) |
            (static_cast<uint16_t>(data[1]) << 8);
    }
}

MAX98357ASounds::MAX98357ASounds(
    fs::FS& filesystem,
    int bclkPin,
    int lrcPin,
    int dataPin)
    :
    _filesystem(filesystem),
    _bclkPin(bclkPin),
    _lrcPin(lrcPin),
    _dataPin(dataPin),
    _audioTask(nullptr),
    _playbackData(nullptr),
    _playbackSize(0),
    _playbackPosition(0),
    _uncachedData(nullptr),
    _uncachedSize(0),
    _playing(false),
    _finished(true),
    _stopRequested(false),
    _waitingForFinalTransmit(false),
    _transmitComplete(false),
    _volume(1.0f),
    _cache(),
    _initialised(false),
    _i2s(
    0,
    0,
    dataPin,
    bclkPin,
    lrcPin)

{


}

MAX98357ASounds::~MAX98357ASounds()
{
    Stop();

    if (_audioTask != nullptr)
    {
        vTaskDelete(_audioTask);
        _audioTask = nullptr;
    }

    for (CachedSample& sample : _cache)
    {
        free(sample.data);
        sample.data = nullptr;
    }

    _cache.clear();

    ReleaseUncachedSample();

    if (_instance == this)
    {
        _instance = nullptr;
    }

    if (_initialised)
    {
        _i2s.end();
    }
}

bool MAX98357ASounds::Begin()
{
    if (_initialised)
    {
        return true;
    }

    /*
     * The MAX98357A is an _i2s digital amplifier.
     *
     * ALL game audio is:
     *   22050 Hz
     *   16-bit
     *   mono
     *   uncompressed PCM WAV
     *
     * The installed Arduino _i2s library uses the ESP-IDF _i2s
     * driver and DMA internally.
     */

     Serial.println("BBBBBBBBBBBBBBBBB");
    _i2s.setAllPins(_bclkPin, _lrcPin, -1, _dataPin);
    if (_i2s.begin(
            I2S_PHILIPS_MODE,
            SampleRate,
            BitsPerSample) == 0)
    {
        Serial.println(
            "MAX98357ASounds: _i2s begin failed");
        return false;
    }

    Serial.println("CCCCCCCCCCCCCCCCCCCCCC");

    _instance = this;
    _i2s.onTransmit(OnTransmit);
    Serial.println("DDDDDDDDDDDDDDDDD");

    if (xTaskCreatePinnedToCore(
            AudioTaskEntry,
            "AudioTask",
            4096,
            this,
            5,
            &_audioTask,
            0) != pdPASS)
    {
        Serial.println(
            "MAX98357ASounds: audio task creation failed");

        _instance = nullptr;
        _i2s.end();
        return false;
    }

    Serial.println("EEEEEEEEEEEEEEEEE");
    _initialised = true;

    Serial.println(
        "MAX98357ASounds: initialised");

    return true;
}

bool MAX98357ASounds::Play(
    const char* filename)
{
    if (!_initialised || filename == nullptr)
    {
        return false;
    }

    /*
     * Load before stopping the current sound.  A failed file
     * load therefore does not interrupt currently playing audio.
     */
    uint8_t* data = nullptr;
    size_t size = 0;

    if (!LoadWavFile(filename, data, size))
    {
        return false;
    }

    Stop();

    ReleaseUncachedSample();

    _uncachedData = data;
    _uncachedSize = size;

    _playbackData = data;
    _playbackSize = size;
    _playbackPosition = 0;

    _stopRequested = false;
    _waitingForFinalTransmit = false;
    _transmitComplete = false;
    _playing = true;
    _finished = false;

    return true;
}

bool MAX98357ASounds::CacheSample(
    const char* filename)
{
    if (!_initialised || filename == nullptr)
    {
        return false;
    }

    if (FindCachedSample(filename) != nullptr)
    {
        return true;
    }

    uint8_t* data = nullptr;
    size_t size = 0;

    if (!LoadWavFile(filename, data, size))
    {
        return false;
    }

    CachedSample sample;
    sample.filename = filename;
    sample.data = data;
    sample.size = size;

    try
    {
        _cache.push_back(std::move(sample));
    }
    catch (...)
    {
        free(data);
        return false;
    }

    Serial.printf(
        "MAX98357ASounds: cached %s (%u bytes)\n",
        filename,
        static_cast<unsigned>(size));

    return true;
}

bool MAX98357ASounds::UnCacheSample(
    const char* filename)
{
    if (filename == nullptr)
    {
        return false;
    }

    for (auto iterator = _cache.begin();
         iterator != _cache.end();
         ++iterator)
    {
        if (iterator->filename == filename)
        {
            if (_playbackData == iterator->data)
            {
                Stop();
            }

            free(iterator->data);
            iterator->data = nullptr;

            _cache.erase(iterator);

            return true;
        }
    }

    return false;
}

bool MAX98357ASounds::PlayCached(
    const char* filename)
{
    if (!_initialised || filename == nullptr)
    {
        return false;
    }

    const CachedSample* sample =
        FindCachedSample(filename);

    if (sample == nullptr)
    {
        /*
         * Deliberately fail rather than falling back to Play().
         *
         * PlayCached() must never perform filesystem I/O.
         */
        Serial.printf(
            "MAX98357ASounds: sample not cached: %s\n",
            filename);

        return false;
    }

    Stop();

    ReleaseUncachedSample();

    _playbackData = sample->data;
    _playbackSize = sample->size;
    _playbackPosition = 0;

    _stopRequested = false;
    _waitingForFinalTransmit = false;
    _transmitComplete = false;
    _playing = true;
    _finished = false;

    return true;
}

void MAX98357ASounds::Stop()
{
    if (!_initialised)
    {
        return;
    }

    _stopRequested = true;
    _playing = false;
    _finished = true;

    _waitingForFinalTransmit = false;
    _transmitComplete = false;

    _playbackData = nullptr;
    _playbackSize = 0;
    _playbackPosition = 0;

    /*
     * We intentionally do not call _i2s.end() here.
     *
     * The _i2s library owns the DMA/ring-buffer machinery.
     * Stopping playback only prevents this class from submitting
     * further PCM data.
     */
}

bool MAX98357ASounds::HasFinished() const
{
    return _finished;
}

void MAX98357ASounds::SetVolume(
    float volume)
{
    _volume =
        std::max(
            0.0f,
            std::min(
                1.0f,
                volume));
}

void MAX98357ASounds::AudioTaskEntry(
    void* parameter)
{
    auto* sounds =
        static_cast<MAX98357ASounds*>(parameter);

    sounds->AudioTask();

    vTaskDelete(nullptr);
}

void IRAM_ATTR MAX98357ASounds::OnTransmit()
{
    /*
     * Keep the transmit callback deliberately tiny.
     *
     * No filesystem access, allocation, or significant processing
     * is performed from the callback.
     */
    if (_instance != nullptr)
    {
        _instance->_transmitComplete = true;
    }
}

void MAX98357ASounds::AudioTask()
{
    /*
     * This buffer is normal internal RAM.
     *
     * The complete sample is stored in PSRAM and this buffer feeds
     * the _i2s library's DMA-backed transmit path.
     */
    uint8_t buffer[TransferBufferSize];

    while (true)
    {
        if (!_playing)
        {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        if (_stopRequested)
        {
            _playing = false;
            _finished = true;
            _waitingForFinalTransmit = false;
            continue;
        }

        const size_t remaining =
            _playbackSize -
            _playbackPosition;

        if (remaining == 0)
        {
            /*
             * All PCM has been submitted.  Wait for the _i2s
             * transmit callback before reporting completion.
             */
            _waitingForFinalTransmit = true;

            if (_transmitComplete)
            {
                _playing = false;
                _finished = true;
                _waitingForFinalTransmit = false;
                _transmitComplete = false;

                ReleaseUncachedSample();

                _playbackData = nullptr;
                _playbackSize = 0;
                _playbackPosition = 0;
            }
            else
            {
                vTaskDelay(1);
            }

            continue;
        }

        const size_t bytesToWrite =
            std::min(
                remaining,
                TransferBufferSize);

        memcpy(
            buffer,
            _playbackData + _playbackPosition,
            bytesToWrite);

        ApplyVolume(
            reinterpret_cast<int16_t*>(buffer),
            bytesToWrite / sizeof(int16_t));

        /*
         * _i2s.write() feeds the library's DMA/ring-buffer path.
         * The _i2s peripheral performs the actual timed transfer
         * to the MAX98357A.
         */
        const size_t written =
            _i2s.write(
                buffer,
                bytesToWrite);

        if (written == 0)
        {
            vTaskDelay(1);
            continue;
        }

        _playbackPosition += written;
        _transmitComplete = false;
    }
}

bool MAX98357ASounds::LoadWavFile(
    const char* filename,
    uint8_t*& data,
    size_t& size)
{
    data = nullptr;
    size = 0;

    File file =
        _filesystem.open(
            filename,
            FILE_READ);

    if (!file)
    {
        Serial.printf(
            "MAX98357ASounds: failed to open %s\n",
            filename);

        return false;
    }

    WavInfo info;

    if (!ReadWavInfo(file, info))
    {
        file.close();
        return false;
    }

    if (info.dataSize == 0)
    {
        file.close();
        return false;
    }

    /*
     * All PCM audio is stored in PSRAM.
     */
    data =
        static_cast<uint8_t*>(
            ps_malloc(info.dataSize));

    if (data == nullptr)
    {
        Serial.printf(
            "MAX98357ASounds: PSRAM allocation failed "
            "(%u bytes)\n",
            static_cast<unsigned>(info.dataSize));

        file.close();
        return false;
    }

    if (!file.seek(info.dataOffset))
    {
        free(data);
        data = nullptr;
        file.close();
        return false;
    }

    const size_t bytesRead =
        file.read(
            data,
            info.dataSize);

    file.close();

    if (bytesRead != info.dataSize)
    {
        Serial.printf(
            "MAX98357ASounds: short WAV read "
            "(%u/%u bytes)\n",
            static_cast<unsigned>(bytesRead),
            static_cast<unsigned>(info.dataSize));

        free(data);
        data = nullptr;
        return false;
    }

    size = info.dataSize;

    return true;
}

bool MAX98357ASounds::ReadWavInfo(
    File& file,
    WavInfo& info)
{
    uint8_t header[12];

    if (file.read(header, sizeof(header)) != sizeof(header))
    {
        return false;
    }

    if (ReadUInt32(header) != RiffId ||
        ReadUInt32(header + 8) != WaveId)
    {
        Serial.println(
            "MAX98357ASounds: not a RIFF/WAVE file");

        return false;
    }

    bool foundFormat = false;
    bool foundData = false;

    while (file.available())
    {
        uint8_t chunkHeader[8];

        if (file.read(
                chunkHeader,
                sizeof(chunkHeader)) !=
            sizeof(chunkHeader))
        {
            break;
        }

        const uint32_t chunkId =
            ReadUInt32(chunkHeader);

        const uint32_t chunkSize =
            ReadUInt32(chunkHeader + 4);

        const size_t chunkDataOffset =
            file.position();

        if (chunkId == FmtId)
        {
            if (chunkSize < 16)
            {
                return false;
            }

            uint8_t format[16];

            if (file.read(
                    format,
                    sizeof(format)) !=
                sizeof(format))
            {
                return false;
            }

            const uint16_t audioFormat =
                ReadUInt16(format);

            const uint16_t channels =
                ReadUInt16(format + 2);

            const uint32_t sampleRate =
                ReadUInt32(format + 4);

            const uint16_t bitsPerSample =
                ReadUInt16(format + 14);

            /*
             * Required game audio format:
             *
             *   PCM
             *   22050 Hz
             *   16-bit
             *   mono
             */
            if (audioFormat != 1 ||
                channels != Channels ||
                sampleRate != SampleRate ||
                bitsPerSample != BitsPerSample)
            {
                Serial.printf(
                    "MAX98357ASounds: unsupported WAV format "
                    "(format=%u channels=%u rate=%u bits=%u)\n",
                    audioFormat,
                    channels,
                    static_cast<unsigned>(sampleRate),
                    bitsPerSample);

                return false;
            }

            foundFormat = true;
        }

        if (chunkId == DataId)
        {
            info.dataOffset =
                static_cast<uint32_t>(
                    chunkDataOffset);

            info.dataSize = chunkSize;
            foundData = true;
            break;
        }

        /*
         * RIFF chunks are word aligned.
         */
        size_t nextOffset =
            chunkDataOffset + chunkSize;

        if ((nextOffset & 1U) != 0)
        {
            ++nextOffset;
        }

        if (!file.seek(nextOffset))
        {
            return false;
        }
    }

    if (!foundFormat || !foundData)
    {
        Serial.println(
            "MAX98357ASounds: WAV missing fmt or data chunk");

        return false;
    }

    return true;
}

void MAX98357ASounds::ApplyVolume(
    int16_t* samples,
    size_t sampleCount)
{
    const float volume = _volume;

    if (volume >= 0.999f)
    {
        return;
    }

    if (volume <= 0.0f)
    {
        memset(
            samples,
            0,
            sampleCount * sizeof(int16_t));

        return;
    }

    for (size_t index = 0;
         index < sampleCount;
         ++index)
    {
        const int32_t value =
            static_cast<int32_t>(
                static_cast<float>(
                    samples[index]) *
                volume);

        samples[index] =
            static_cast<int16_t>(
                std::max(
                    -32768,
                    std::min(
                        32767,
                        value)));
    }
}

MAX98357ASounds::CachedSample*
MAX98357ASounds::FindCachedSample(
    const char* filename)
{
    for (CachedSample& sample : _cache)
    {
        if (sample.filename == filename)
        {
            return &sample;
        }
    }

    return nullptr;
}

const MAX98357ASounds::CachedSample*
MAX98357ASounds::FindCachedSample(
    const char* filename) const
{
    for (const CachedSample& sample : _cache)
    {
        if (sample.filename == filename)
        {
            return &sample;
        }
    }

    return nullptr;
}

void MAX98357ASounds::ReleaseUncachedSample()
{
    if (_uncachedData != nullptr)
    {
        free(_uncachedData);

        _uncachedData = nullptr;
        _uncachedSize = 0;
    }
}
