#pragma once



class IZoom
{
public:
    virtual bool GetZoom(float& zoom) = 0;
protected:    
    static constexpr float _maxZoom = 4.0f ;
    static constexpr float _minZoom = 0.7f;

};