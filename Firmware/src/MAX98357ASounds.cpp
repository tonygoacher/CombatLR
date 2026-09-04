#include "MAX98357ASounds.h"

#include <cstdlib>
#include <cstring>

namespace
{
    constexpr uint32_t RIFF_ID = 0x46464952; // "RIFF"
    constexpr uint32_t WAVE_ID = 0x45564157; // "WAVE"
    constexpr uint32_t FMT_ID  = 0x20746D66; // "fmt "
    constexpr uint32_t DATA_ID = 0x61746164; // "data"

    uint16_t ReadUInt16(const uint8_t* p)
    {
        return static_cast<uint16_t>(p[0]) |
               static_cast<uint16_t>(p[1]) << 8;
    }

    uint32_t ReadUInt32(const uint8_t* p)
    {
        return static_cast<uint32_t>(p[0]) |
               static_cast<uint32_t>(p[1]) << 8 |
               static_cast<uint32_t>(p[2]) << 16 |
               static_cast<uint32_t>(p[3]) << 24;
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
    _i2s(),
    _audioTask(nullptr),
    _mutex(nullptr),
    _cachedSamples(),
    _currentData(nullptr),
    _currentSize(0),
    _currentPosition(0),
    _currentOwnedData(nullptr),
    _playing(false),
    _stopRequested(false),
    _finished(true),
    _volume(1.0f),
    _initialised(false)
{
}

MAX98357ASounds::~MAX98357ASounds()
{
    if (_initialised)
    {
        Stop();
    }

    if (_audioTask != nullptr)
    {
        vTaskDelete(_audioTask);
        _audioTask = nullptr;
    }

    for (CachedSample& sample : _cachedSamples)
    {
        free(sample.data);
        sample.data = nullptr;
    }

    _cachedSamples.clear();

    ReleaseCurrentSample();

    if (_mutex != nullptr)
    {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }

    if (_initialised)
    {
        _i2s.end();
        _initialised = false;
    }
}

bool MAX98357ASounds::Begin()
{
    if (_initialised)
    {
        return true;
    }

    /*
     * MAX98357A wiring:
     *
     *   GPIO18 -> BCLK
     *   GPIO8  -> LRC / WS
     *   GPIO17 -> DIN
     *
     * The MAX98357A is an I2S receiver/amplifier. It has no
     * I2C address.
     */
    Serial.printf(
        "MAX98357A: BCLK=%d LRC=%d DATA=%d\n",
        _bclkPin,
        _lrcPin,
        _dataPin);

    /*
     * Arduino-ESP32 3.x requires the pins to be configured
     * before the I2S channel is started.
     */
    _i2s.setPins(
        _bclkPin,
        _lrcPin,
        _dataPin);

     _i2s.setPins(
        _bclkPin,
        _lrcPin,
        _dataPin);


    /*
     * Standard Philips I2S, 16 kHz, 16-bit, mono.
     *
     * I2SClass uses the ESP32 I2S peripheral and its DMA
     * transfer mechanism; this class does not implement an
     * audio ISR or an onTransmit callback.
     */
if (!_i2s.begin(
        I2S_MODE_STD,
        SAMPLE_RATE,
        I2S_DATA_BIT_WIDTH_16BIT,
        I2S_SLOT_MODE_MONO,
        I2S_STD_SLOT_BOTH))
{
    Serial.println(
        "MAX98357ASounds: I2S begin failed");

    return false;
}


// DEBUG

int16_t samples[256];

for (int i = 0; i < 256; ++i)
{
    samples[i] =
        static_cast<int16_t>(
            sinf(
                2.0f * PI * 440.0f *
                static_cast<float>(i) /
                SAMPLE_RATE) *
            12000.0f);
}

_i2s.write(
    reinterpret_cast<uint8_t*>(samples),
    sizeof(samples));
// __DEBUG
    _mutex = xSemaphoreCreateMutex();

    if (_mutex == nullptr)
    {
        Serial.println(
            "MAX98357ASounds: mutex creation failed");

        _i2s.end();
        return false;
    }

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

        vSemaphoreDelete(_mutex);
        _mutex = nullptr;

        _i2s.end();
        return false;
    }

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
     * Load before stopping the current sound. A filesystem error
     * therefore does not interrupt an already playing sound.
     */
    uint8_t* data = nullptr;
    size_t size = 0;

    if (!LoadWav(
            filename,
            data,
            size))
    {
        return false;
    }

    Stop();

    if (xSemaphoreTake(
            _mutex,
            portMAX_DELAY) != pdTRUE)
    {
        free(data);
        return false;
    }

    ReleaseCurrentSample();

    _currentOwnedData = data;
    _currentData = data;
    _currentSize = size;
    _currentPosition = 0;

    _stopRequested = false;
    _playing = true;
    _finished = false;
Serial.printf(
    "MAX98357ASounds: playing %s, %u bytes\n",
    filename,
    static_cast<unsigned>(size));
    xSemaphoreGive(_mutex);

    return true;
}

bool MAX98357ASounds::PlayCached(
    const char* filename)
{
    if (!_initialised || filename == nullptr)
    {
        return false;
    }

    Stop();

    if (xSemaphoreTake(
            _mutex,
            portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    /*
     * The lookup is protected because UnCacheSample() can remove
     * an entry from the vector.
     */
    CachedSample* sample =
        FindCachedSample(filename);

    if (sample == nullptr)
    {
        xSemaphoreGive(_mutex);

        /*
         * PlayCached deliberately never performs filesystem I/O.
         */
        return false;
    }

    ReleaseCurrentSample();

    _currentData = sample->data;
    _currentSize = sample->size;
    _currentPosition = 0;

    _stopRequested = false;
    _playing = true;
    _finished = false;

    xSemaphoreGive(_mutex);

    // Instantly wake the audio task so it begins writing to I2S
    if (_audioTask != nullptr) 
    {
        xTaskNotifyGive(_audioTask);
    }

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

    if (!LoadWav(
            filename,
            data,
            size))
    {
        return false;
    }

    CachedSample sample;
    sample.filename = filename;
    sample.data = data;
    sample.size = size;

    if (xSemaphoreTake(
            _mutex,
            portMAX_DELAY) != pdTRUE)
    {
        free(data);
        return false;
    }

    /*
     * Check again after taking the mutex in case another caller
     * cached the same filename while the WAV was being loaded.
     */
    if (FindCachedSample(filename) != nullptr)
    {
        xSemaphoreGive(_mutex);
        free(data);
        return true;
    }

    try
    {
        _cachedSamples.push_back(
            std::move(sample));
    }
    catch (...)
    {
        xSemaphoreGive(_mutex);
        free(data);
        return false;
    }

    xSemaphoreGive(_mutex);

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

    if (_mutex == nullptr)
    {
        return false;
    }

    if (xSemaphoreTake(
            _mutex,
            portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    for (auto iterator = _cachedSamples.begin();
         iterator != _cachedSamples.end();
         ++iterator)
    {
        if (iterator->filename == filename)
        {
            /*
             * Do not free a cached sample while it is the current
             * playback source.
             */
            if (_currentData == iterator->data)
            {
                _playing = false;
                _stopRequested = true;
                _finished = true;

                _currentData = nullptr;
                _currentSize = 0;
                _currentPosition = 0;
            }

            uint8_t* data = iterator->data;

            _cachedSamples.erase(iterator);

            xSemaphoreGive(_mutex);

            free(data);

            return true;
        }
    }

    xSemaphoreGive(_mutex);

    return false;
}

void MAX98357ASounds::Stop()
{
    if (!_initialised || _mutex == nullptr)
    {
        return;
    }

    if (xSemaphoreTake(
            _mutex,
            portMAX_DELAY) != pdTRUE)
    {
        return;
    }

    _stopRequested = true;
    _playing = false;
    _finished = true;

    _currentData = nullptr;
    _currentSize = 0;
    _currentPosition = 0;

    /*
     * Only Play() owns _currentOwnedData. Cached samples remain
     * in _cachedSamples.
     */
    ReleaseCurrentSample();

    xSemaphoreGive(_mutex);
}

bool MAX98357ASounds::HasFinished() const
{
    return _finished;
}

void MAX98357ASounds::SetVolume(
    float volume)
{
    if (volume < 0.0f)
    {
        volume = 0.0f;
    }
    else if (volume > 1.0f)
    {
        volume = 1.0f;
    }

    _volume = volume;
}

void MAX98357ASounds::AudioTaskEntry(
    void* parameter)
{

    Serial.println("MAX98357ASounds: AudioTask started");
    auto* sounds =
        static_cast<MAX98357ASounds*>(parameter);

    sounds->AudioTask();

    vTaskDelete(nullptr);
}

void MAX98357ASounds::AudioTask()
{
    /*
     * This buffer is in internal RAM. The complete sample is in
     * PSRAM. 2048 bytes represents 64 ms of mono 16-bit audio at
     * 16 kHz.
     */

    for (;;)
    {
        if (!_playing)
        {
            // Block and wait for a notification from PlayCached, 
            // checking at least every so often just in case.
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5));
            if(!_playing)
            {
                // If no sample requested, pump the I2S bus with zeros to prevent the 98357 shutting down.
                // This is required to precent slight delay if no samples have been played for a while
                _i2s.write(_silence, 256);  
            }
  
            continue;
        }

        size_t bytesToWrite = 0;

        if (xSemaphoreTake(
                _mutex,
                portMAX_DELAY) != pdTRUE)
        {
            continue;
        }

        if (_stopRequested || !_playing)
        {
            xSemaphoreGive(_mutex);
            continue;
        }

        const size_t remaining =
            _currentSize - _currentPosition;

        if (remaining == 0)
        {
            /*
             * The final block has been accepted by I2SClass::write().
             * The ESP32 I2S peripheral/DMA is responsible for the
             * subsequent hardware transfer.
             */
            _playing = false;
            _finished = true;

            _currentData = nullptr;
            _currentSize = 0;
            _currentPosition = 0;

            ReleaseCurrentSample();

            xSemaphoreGive(_mutex);
            continue;
        }

    // Force a much smaller software chunk to reduce blocking time
        const size_t maxChunk = 256; 
        bytesToWrite =
            (remaining < maxChunk)
                ? remaining
                : maxChunk;

        /*
         * Keep PCM samples whole.
         */
        bytesToWrite &= ~static_cast<size_t>(1);

        if (bytesToWrite == 0)
        {
            _playing = false;
            _finished = true;

            _currentData = nullptr;
            _currentSize = 0;
            _currentPosition = 0;

            ReleaseCurrentSample();

            xSemaphoreGive(_mutex);
            continue;
        }

        memcpy(
            _transferBuffer,
            _currentData + _currentPosition,
            bytesToWrite);

        _currentPosition += bytesToWrite;

        xSemaphoreGive(_mutex);

        /*
         * Volume is applied to the temporary buffer so cached
         * samples are never modified.
         */
        ApplyVolume(
            reinterpret_cast<int16_t*>(_transferBuffer),
            bytesToWrite / sizeof(int16_t));

        /*
         * I2SClass::write() feeds the ESP32 I2S TX channel.
         * The actual serial transfer is performed by the I2S
         * peripheral using its DMA/interrupt machinery.
         */
   const size_t written =
    _i2s.write(
        _transferBuffer,
        bytesToWrite);

        if (written < bytesToWrite)
        {
            if (xSemaphoreTake(
                    _mutex,
                    portMAX_DELAY) == pdTRUE)
            {
                _currentPosition -=
                    bytesToWrite - written;

                xSemaphoreGive(_mutex);
            }

            vTaskDelay(1);
        }
    }
}

bool MAX98357ASounds::LoadWav(
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
            "MAX98357ASounds: unable to open %s\n",
            filename);

        return false;
    }

    size_t dataOffset = 0;
    size_t dataSize = 0;

    if (!ParseWav(
            file,
            dataOffset,
            dataSize))
    {
        file.close();
        return false;
    }

    /*
     * Complete PCM sample is stored in PSRAM.
     */
    data =
        static_cast<uint8_t*>(
            ps_malloc(dataSize));

    if (data == nullptr)
    {
        Serial.printf(
            "MAX98357ASounds: PSRAM allocation failed "
            "(%u bytes)\n",
            static_cast<unsigned>(dataSize));

        file.close();
        return false;
    }

    if (!file.seek(dataOffset))
    {
        free(data);
        data = nullptr;
        file.close();
        return false;
    }

    const size_t bytesRead =
        file.read(
            data,
            dataSize);

    file.close();

    if (bytesRead != dataSize)
    {
        Serial.printf(
            "MAX98357ASounds: short WAV read "
            "(%u/%u bytes)\n",
            static_cast<unsigned>(bytesRead),
            static_cast<unsigned>(dataSize));

        free(data);
        data = nullptr;
        return false;
    }

    size = dataSize;

    return true;
}

bool MAX98357ASounds::ParseWav(
    File& file,
    size_t& dataOffset,
    size_t& dataSize)
{
    uint8_t header[12];

    if (file.read(
            header,
            sizeof(header)) != sizeof(header))
    {
        return false;
    }

    if (ReadUInt32(header) != RIFF_ID ||
        ReadUInt32(header + 8) != WAVE_ID)
    {
        Serial.println(
            "MAX98357ASounds: not a RIFF/WAVE file");

        return false;
    }

    bool formatFound = false;
    bool dataFound = false;

    while (file.available())
    {
        uint8_t chunkHeader[8];

        if (file.read(
                chunkHeader,
                sizeof(chunkHeader)) != sizeof(chunkHeader))
        {
            break;
        }

        const uint32_t chunkId =
            ReadUInt32(chunkHeader);

        const uint32_t chunkSize =
            ReadUInt32(chunkHeader + 4);

        const size_t chunkOffset =
            file.position();

        if (chunkId == FMT_ID)
        {
            if (chunkSize < 16)
            {
                return false;
            }

            uint8_t format[16];

            if (file.read(
                    format,
                    sizeof(format)) != sizeof(format))
            {
                Serial.println("Chunk header error");
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

            if (audioFormat != 1 ||
                channels != 1 ||
                sampleRate != SAMPLE_RATE ||
                bitsPerSample != BITS_PER_SAMPLE)
            {
                Serial.printf(
                    "MAX98357ASounds: unsupported WAV "
                    "(format=%u channels=%u rate=%u bits=%u)\n",
                    audioFormat,
                    channels,
                    static_cast<unsigned>(sampleRate),
                    bitsPerSample);

                return false;
            }

            formatFound = true;
        }
        else if (chunkId == DATA_ID)
        {
            dataOffset = chunkOffset;
            dataSize = chunkSize;
            dataFound = true;
            break;
        }

        /*
         * RIFF chunks are word aligned.
         */
        size_t nextOffset =
            chunkOffset + chunkSize;

        if ((nextOffset & 1U) != 0)
        {
            ++nextOffset;
        }

        if (!file.seek(nextOffset))
        {
            Serial.println("WAV seek error");
            return false;
        }
    }

    if (!formatFound || !dataFound || dataSize == 0)
    {
        Serial.println(
            "MAX98357ASounds: invalid WAV");

        return false;
    }

    /*
     * A PCM sample must contain complete 16-bit samples.
     */
    if ((dataSize & 1U) != 0)
    {
        Serial.println(
            "MAX98357ASounds: WAV data is not 16-bit aligned");

        return false;
    }
    Serial.println("WAV load OK");
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

        if (value > 32767)
        {
            samples[index] = 32767;
        }
        else if (value < -32768)
        {
            samples[index] = -32768;
        }
        else
        {
            samples[index] =
                static_cast<int16_t>(value);
        }
    }
}

MAX98357ASounds::CachedSample*
MAX98357ASounds::FindCachedSample(
    const char* filename)
{
    for (CachedSample& sample : _cachedSamples)
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
    for (const CachedSample& sample :
         _cachedSamples)
    {
        if (sample.filename == filename)
        {
            return &sample;
        }
    }

    return nullptr;
}

void MAX98357ASounds::ReleaseCurrentSample()
{
    if (_currentOwnedData != nullptr)
    {
        free(_currentOwnedData);
        _currentOwnedData = nullptr;
    }
}
