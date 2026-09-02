#pragma once

#include <stdint.h>
#include <string.h>

#include "Widget.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


class TextWidget : public Widget
{
public:

    TextWidget(
        int id,
        const char* text ,
        WidgetPosition position,
        int font,
        uint8_t datum,
        uint16_t textColour,
        uint16_t backgroundColour,
        uint8_t size)
        :
        Widget(id, WidgetType::Text),
        _position{ position },
        _font(font),
        _datum(datum),
        _textColour(textColour),
        _backgroundColour(backgroundColour),
        _size(size)

    {
        SetText(text);
    }


    ~TextWidget() override
    {

    }


    WidgetCoordinateSpace GetCoordinateSpace() const override
    {
        return WidgetCoordinateSpace::Screen;
    }


    WidgetPosition GetPosition() const override
    {
        return _position;
    }


    GameCoordinate GetGamePosition() const override
    {
        return GameCoordinate();
    }


    void SetPosition(WidgetPosition position)
    {
        _position = position;
    }


    void SetText(const char* text)
    {

        if (text == nullptr)
        {
            _text[0] = '\0';
        }
        else
        {
            strncpy(
                _text,
                text,
                sizeof(_text) - 1);

            _text[sizeof(_text) - 1] = '\0';
        }

    }




    void SetDatum(uint8_t datum)
    {
        _datum = datum;
    }


    void SetBackgroundColour(uint32_t colour)
    {
        _backgroundColour = colour;
    }


    void Render(IDisplayContext& context) override;


private:

    WidgetPosition _position;

    char _text[64];

    uint8_t _font;
    uint8_t _datum;

    uint16_t _backgroundColour;
    uint16_t _textColour;
    uint8_t _size;
};