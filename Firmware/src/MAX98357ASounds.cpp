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

    _cachedSample(nullptr),
    _cachedData(nullptr),
    _cachedSize(0),
    _cachedPosition(0),
    _cachedPlaying(false),

    _fileData(nullptr),
    _fileSize(0),
    _filePosition(0),
    _fileOwnedData(nullptr),

    _priorityPlaying(false),
    _filePlaying(false),
    _fileFinished(true),

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

    if (_cachedSample != nullptr)
    {
        free(_cachedSample->data);
        _cachedSample->data = nullptr;

        delete _cachedSample;
        _cachedSample = nullptr;
    }

    ReleaseFileSample();

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

    Serial.printf(
        "MAX98357A: BCLK=%d LRC=%d DATA=%d\n",
        _bclkPin,
        _lrcPin,
        _dataPin);

    _i2s.setPins(
        _bclkPin,
        _lrcPin,
        _dataPin);

    if (!_i2s.begin(
            I2S_MODE_STD,
            SAMPLE_RATE,
            I2S_DATA_BIT_WIDTH_16BIT,
            I2S_SLOT_MODE_MONO))
    {
        Serial.println(
            "MAX98357ASounds: I2S begin failed");

        return false;
    }

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
            8192,
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
    if (!_initialised ||
        filename == nullptr)
    {
        return false;
    }

    /*
     * A normal Play() is ignored while priority
     * playback is active.
     */
    if (_priorityPlaying)
    {
        return false;
    }

    uint8_t* data = nullptr;
    size_t size = 0;

    /*
     * Load before replacing the current file playback.
     * A filesystem error therefore leaves the current
     * sound playing.
     */
    if (!LoadWav(
            filename,
            data,
            size))
    {
        return false;
    }

    if (xSemaphoreTake(
            _mutex,
            portMAX_DELAY) != pdTRUE)
    {
        free(data);
        return false;
    }

    /*
     * Priority could have started while the WAV was
     * loading.
     */
    if (_priorityPlaying)
    {
        xSemaphoreGive(_mutex);

        free(data);

        return false;
    }

    /*
     * Replace the existing normal file playback.
     */
    ReleaseFileSample();

    _fileOwnedData = data;
    _fileData = data;
    _fileSize = size;
    _filePosition = 0;

    _filePlaying = true;
    _priorityPlaying = false;
    _fileFinished = false;

    xSemaphoreGive(_mutex);

    if (_audioTask != nullptr)
    {
        xTaskNotifyGive(_audioTask);
    }

    return true;
}


bool MAX98357ASounds::PlayPriority(
    const char* filename)
{
    if (!_initialised ||
        filename == nullptr)
    {
        return false;
    }

    uint8_t* data = nullptr;
    size_t size = 0;

    /*
     * Load the new file before cancelling the existing
     * file playback.
     */
    if (!LoadWav(
            filename,
            data,
            size))
    {
        return false;
    }

    if (xSemaphoreTake(
            _mutex,
            portMAX_DELAY) != pdTRUE)
    {
        free(data);
        return false;
    }

    /*
     * Cancel whatever file playback currently exists.
     * Cached playback is deliberately untouched.
     */
    ReleaseFileSample();

    _fileOwnedData = data;
    _fileData = data;
    _fileSize = size;
    _filePosition = 0;

    _filePlaying = true;
    _priorityPlaying = true;
    _fileFinished = false;

    xSemaphoreGive(_mutex);

    return true;
}


bool MAX98357ASounds::CacheSample(
    const char* filename)
{
    if (!_initialised ||
        filename == nullptr)
    {
        return false;
    }

    /*
     * If the requested sample is already cached,
     * there is nothing more to do.
     */
    if (xSemaphoreTake(
            _mutex,
            portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    if (_cachedSample != nullptr)
    {
        if (_cachedSample->filename == filename)
        {
            xSemaphoreGive(_mutex);
            return true;
        }
    }

    xSemaphoreGive(_mutex);

    /*
     * Load outside the mutex because filesystem access
     * can take some time.
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

    if (xSemaphoreTake(
            _mutex,
            portMAX_DELAY) != pdTRUE)
    {
        free(data);
        return false;
    }

    /*
     * Another caller may have cached something while
     * the WAV was loading.
     *
     * Replace the old cached sample with this one.
     * The old sample must not remain playable.
     */
    if (_cachedSample != nullptr)
    {
        _cachedPlaying = false;
        _cachedData = nullptr;
        _cachedSize = 0;
        _cachedPosition = 0;

        free(_cachedSample->data);
        _cachedSample->data = nullptr;

        delete _cachedSample;
        _cachedSample = nullptr;
    }

    _cachedSample = new CachedSample;

    if (_cachedSample == nullptr)
    {
        xSemaphoreGive(_mutex);

        free(data);

        return false;
    }

    _cachedSample->filename = filename;
    _cachedSample->data = data;
    _cachedSample->size = size;

    xSemaphoreGive(_mutex);

    Serial.printf(
        "MAX98357ASounds: cached %s (%u bytes)\n",
        filename,
        static_cast<unsigned>(size));

    return true;
}


bool MAX98357ASounds::PlayCached()
{
    if (!_initialised)
    {
        return false;
    }

    if (xSemaphoreTake(
            _mutex,
            portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    if (_cachedSample == nullptr)
    {
        xSemaphoreGive(_mutex);

        return false;
    }

    /*
     * Restart the cached sample from the beginning.
     */
    _cachedData = _cachedSample->data;
    _cachedSize = _cachedSample->size;
    _cachedPosition = 0;
    _cachedPlaying = true;

    xSemaphoreGive(_mutex);

    if (_audioTask != nullptr)
    {
        xTaskNotifyGive(_audioTask);
    }

    return true;
}


bool MAX98357ASounds::UnCacheSample()
{
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

    if (_cachedSample == nullptr)
    {
        xSemaphoreGive(_mutex);

        return false;
    }

    /*
     * Stop cached playback first.
     *
     * The actual memory is not freed until we hold the
     * mutex, so AudioTask cannot be using the cached
     * buffer at the same time.
     */
    _cachedPlaying = false;
    _cachedData = nullptr;
    _cachedSize = 0;
    _cachedPosition = 0;

    free(_cachedSample->data);
    _cachedSample->data = nullptr;

    delete _cachedSample;
    _cachedSample = nullptr;

    xSemaphoreGive(_mutex);

    return true;
}


void MAX98357ASounds::Stop()
{
    if (!_initialised ||
        _mutex == nullptr)
    {
        return;
    }

    if (xSemaphoreTake(
            _mutex,
            portMAX_DELAY) != pdTRUE)
    {
        return;
    }

    /*
     * Stop cached playback.
     */
    _cachedPlaying = false;
    _cachedData = nullptr;
    _cachedSize = 0;
    _cachedPosition = 0;

    /*
     * Stop normal/priority file playback.
     */
    _filePlaying = false;
    _priorityPlaying = false;
    _fileFinished = true;

    _fileData = nullptr;
    _fileSize = 0;
    _filePosition = 0;

    ReleaseFileSample();

    xSemaphoreGive(_mutex);
}


bool MAX98357ASounds::HasFinished() const
{
    return _fileFinished;
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
    auto* sounds =
        static_cast<MAX98357ASounds*>(parameter);

    sounds->AudioTask();

    vTaskDelete(nullptr);
}

void MAX98357ASounds::AudioTask()
{
    /*
     * This buffer remains in internal RAM.
     *
     * Samples are mixed here before being passed to I2S.
     */
    uint8_t transferBuffer[
        TRANSFER_BUFFER_SIZE];

    const size_t sampleCapacity =
        TRANSFER_BUFFER_SIZE /
        sizeof(int16_t);

    constexpr size_t IDLE_SILENCE_BYTES = 256;

    uint8_t silenceBuffer[TRANSFER_BUFFER_SIZE];
    memset(
    silenceBuffer,
    0,
    TRANSFER_BUFFER_SIZE);

    for (;;)
    {
        /*
         * When there is no active playback, wait for up to
         * 5 ms for a playback operation to signal the mutex.
         *
         * If nothing starts playback during that period,
         * send a short block of silence. This keeps the
         * MAX98357A awake without continuously polling.
         */
   
        if (!_cachedPlaying &&
            !_filePlaying)
        {
            /*
            * Nothing is playing.
            *
            * Sleep until a playback operation explicitly wakes us,
            * or periodically wake to keep the I2S device alive.
            */
            // Block and wait for a notification from PlayCached, 
            // checking at least every so often just in case.
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5));
             if (!_cachedPlaying &&
            !_filePlaying)
            {
                // If no sample requested, pump the I2S bus with zeros to prevent the 98357 shutting down.
                // This is required to precent slight delay if no samples have been played for a while
                _i2s.write(silenceBuffer, TRANSFER_BUFFER_SIZE);
                continue;
            }
  


        }
        else
        {
            /*
             * Playback is active. Wait until we can safely
             * access the playback buffers.
             */
            if (xSemaphoreTake(
                    _mutex,
                    portMAX_DELAY) != pdTRUE)
            {
                continue;
            }
        }

        /*
         * Work out whether anything is actually playable.
         */
        const bool cachedActive =
            _cachedPlaying &&
            _cachedData != nullptr &&
            _cachedPosition < _cachedSize;

        const bool fileActive =
            _filePlaying &&
            _fileData != nullptr &&
            _filePosition < _fileSize;

        if (!cachedActive &&
            !fileActive)
        {
            /*
             * Cached playback has completed.
             */
            if (_cachedPlaying)
            {
                _cachedPlaying = false;
                _cachedData = nullptr;
                _cachedSize = 0;
                _cachedPosition = 0;
            }

            /*
             * File playback has completed.
             */
            if (_filePlaying)
            {
                _filePlaying = false;
                _priorityPlaying = false;
                _fileFinished = true;

                _fileData = nullptr;
                _fileSize = 0;
                _filePosition = 0;

                ReleaseFileSample();
            }

            xSemaphoreGive(_mutex);
            vTaskDelay(1);
            /*
             * We got the mutex, but there was no audio to
             * play. The next iteration will perform the
             * 5 ms idle wait.
             */
            continue;
        }

        /*
         * Determine how many complete 16-bit samples we
         * can mix this iteration.
         */
        size_t samplesToMix = sampleCapacity;

        if (cachedActive)
        {
            const size_t cachedRemaining =
                (_cachedSize -
                 _cachedPosition) /
                sizeof(int16_t);

            if (cachedRemaining < samplesToMix)
            {
                samplesToMix = cachedRemaining;
            }
        }

        if (fileActive)
        {
            const size_t fileRemaining =
                (_fileSize -
                 _filePosition) /
                sizeof(int16_t);

            if (fileRemaining < samplesToMix)
            {
                samplesToMix = fileRemaining;
            }
        }

        if (samplesToMix == 0)
        {
            /*
             * This should only occur for an invalid or
             * exhausted source.
             */
            xSemaphoreGive(_mutex);
            continue;
        }

        const int16_t* cachedSamples =
            cachedActive
                ? reinterpret_cast<const int16_t*>(
                    _cachedData + _cachedPosition)
                : nullptr;

        const int16_t* fileSamples =
            fileActive
                ? reinterpret_cast<const int16_t*>(
                    _fileData + _filePosition)
                : nullptr;

        /*
         * Mix the active sources.
         *
         * Use int32_t while adding so two full-scale
         * samples cannot overflow int16_t.
         */
        for (size_t index = 0;
             index < samplesToMix;
             ++index)
        {
            int32_t mixed = 0;

            if (cachedSamples != nullptr)
            {
                mixed += cachedSamples[index];
            }

            if (fileSamples != nullptr)
            {
                mixed += fileSamples[index];
            }

            if (mixed > 32767)
            {
                mixed = 32767;
            }
            else if (mixed < -32768)
            {
                mixed = -32768;
            }

            reinterpret_cast<int16_t*>(transferBuffer)[index] =
                static_cast<int16_t>(mixed);
        }

        const size_t bytesToWrite =
            samplesToMix *
            sizeof(int16_t);

        /*
         * Advance source positions before releasing the
         * mutex. The I2S write is performed while holding
         * the mutex so the source buffers cannot be
         * replaced or freed halfway through this block.
         */
        if (cachedActive)
        {
            _cachedPosition += bytesToWrite;

            if (_cachedPosition >= _cachedSize)
            {
                _cachedPlaying = false;
                _cachedData = nullptr;
                _cachedSize = 0;
                _cachedPosition = 0;
            }
        }

        if (fileActive)
        {
            _filePosition += bytesToWrite;

            if (_filePosition >= _fileSize)
            {
                _filePlaying = false;
                _priorityPlaying = false;
                _fileFinished = true;

                _fileData = nullptr;
                _fileSize = 0;
                _filePosition = 0;

                /*
                 * The file has now been completely copied
                 * into the I2S buffer. Its PSRAM allocation
                 * can be released.
                 */
                ReleaseFileSample();
            }
        }

        /*
         * Apply volume after mixing.
         */
        ApplyVolume(
            reinterpret_cast<int16_t*>(transferBuffer),
            samplesToMix);


            /*
        * The transfer buffer contains a complete copy of the
        * source samples, so the mutex is no longer required.
        */
        xSemaphoreGive(_mutex);

        _i2s.write(
            transferBuffer,
            bytesToWrite);

        vTaskDelay(1);
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
            return false;
        }
    }

    if (!formatFound ||
        !dataFound ||
        dataSize == 0)
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
    if (_cachedSample == nullptr)
    {
        return nullptr;
    }

    if (_cachedSample->filename == filename)
    {
        return _cachedSample;
    }

    return nullptr;
}


const MAX98357ASounds::CachedSample*
MAX98357ASounds::FindCachedSample(
    const char* filename) const
{
    if (_cachedSample == nullptr)
    {
        return nullptr;
    }

    if (_cachedSample->filename == filename)
    {
        return _cachedSample;
    }

    return nullptr;
}


void MAX98357ASounds::ReleaseFileSample()
{
    if (_fileOwnedData != nullptr)
    {
        free(_fileOwnedData);

        _fileOwnedData = nullptr;
    }
}


void MAX98357ASounds::ReleaseCachedSample()
{
    if (_cachedSample != nullptr)
    {
        free(_cachedSample->data);

        _cachedSample->data = nullptr;

        delete _cachedSample;

        _cachedSample = nullptr;
    }
}