#define FASTLED_ESP8266_RAW_PIN_ORDER

#include <LedController.h>
#include <Protocol.h>
#include <FastLED.h>

class DigitalLedController : public LedController
{
public:
    DigitalLedController(char ledCount) : LedController()
    {
        this->ledCount = ledCount;
        colors = new CRGB[ledCount];
    }

    ~DigitalLedController()
    {
        delete colors;
    }

    virtual void Setup()
    {
        FastLED.addLeds<WS2812B, D2, GRB>(colors, ledCount);
        FastLED.setDither(BINARY_DITHER);
    }

    virtual char GetLedCount()
    {
        return ledCount;
    }

    virtual void SetColor(float r, float g, float b, float w)
    {
        CRGB color((char)round(r * 255), (char)round(g * 255), (char)round(b * 255));

        for (char i = 0; i < ledCount; i++)
        {
            colors[i] = color;
        }
    }

    virtual void ReadColors(WiFiClient client)
    {
        client.readBytes(((uint8_t *)colors), ledCount * 3);
        FastLED.setBrightness(client.read());
    }

    virtual void ShowColors()
    {
        FastLED.show();
    }

    virtual char GetFlags()
    {
        return (char)(HAS_DITHER);
    }

private:
    char ledCount;
    CRGB *colors;
};