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
        const char* text = "")
        :
        Widget(id, WidgetType::Text),
        _position{ 0.0f, 0.0f },
        _font(2),
        _datum(TL_DATUM),
        _backgroundColour(TFT_BLACK),
        _textMutex(nullptr)
    {
        _textMutex = xSemaphoreCreateMutex();

        SetText(text);
    }


    ~TextWidget() override
    {
        if (_textMutex != nullptr)
        {
            vSemaphoreDelete(_textMutex);
        }
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
        if (_textMutex == nullptr)
            return;

        if (xSemaphoreTake(_textMutex, portMAX_DELAY) != pdTRUE)
            return;

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

        xSemaphoreGive(_textMutex);
    }


    void SetFont(uint8_t font)
    {
        _font = font;
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

    uint32_t _backgroundColour;

    SemaphoreHandle_t _textMutex;
};