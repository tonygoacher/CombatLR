#pragma once

class ISounds
{
public:
    virtual ~ISounds() = default;

    /*
     * Play a WAV file from the filesystem.
     *
     * ALL game sound effects must be:
     *   - uncompressed PCM WAV
     *   - 22050 Hz
     *   - 16-bit
     *   - mono
     */
    virtual bool Play(const char* filename) = 0;

    /*
     * Cache the complete PCM sample in PSRAM.
     *
     * Required format:
     *   - uncompressed PCM WAV
     *   - 22050 Hz
     *   - 16-bit
     *   - mono
     */
    virtual bool CacheSample(const char* filename) = 0;

    virtual bool UnCacheSample(const char* filename) = 0;

    /*
     * Play an already cached sample.
     * This must never perform filesystem I/O.
     */
    virtual bool PlayCached(const char* filename) = 0;

    virtual void Stop() = 0;

    /*
     * True when the current sound has completed transmission.
     */
    virtual bool HasFinished() const = 0;

    /*
     * Volume:
     *   0.0 = silent
     *   1.0 = maximum
     */
    virtual void SetVolume(float volume) = 0;
};
