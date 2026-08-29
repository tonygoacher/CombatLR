#pragma once

class ISounds
{
public:
    virtual ~ISounds() = default;

    virtual bool Play(const char* filename) = 0;
    virtual bool PlayCached(const char* filename) = 0;

    virtual bool CacheSample(const char* filename) = 0;
    virtual bool UnCacheSample(const char* filename) = 0;

    virtual void Stop() = 0;
    virtual bool HasFinished() const = 0;
    virtual void SetVolume(float volume) = 0;
};
