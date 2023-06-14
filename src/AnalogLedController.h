#include <LedController.h>
#include <Protocol.h>

#define PIN_R 12
#define PIN_G 5
#define PIN_B 13
#define PIN_W 15

class AnalogLedController : public LedController
{
public:
    AnalogLedController() : LedController()
    {
    }

    virtual void Setup()
    {
        analogWriteRange(16383);
        analogWriteFreq(1000);
    }

    virtual char GetLedCount()
    {
        return 1;
    }

    virtual void SetColor(float r, float g, float b, float w)
    {
        color[0] = (uint16_t)round(r * 65535);
        color[1] = (uint16_t)round(g * 65535);
        color[2] = (uint16_t)round(b * 65535);
        color[3] = (uint16_t)round(w * 65535);
    }

    virtual void ReadColors(WiFiClient client)
    {
        client.readBytes((char *)&color, 8);
    }

    virtual void ShowColors()
    {
        buffer[0] += (float)color[0] / 4.0f;
        buffer[1] += (float)color[1] / 4.0f;
        buffer[2] += (float)color[2] / 4.0f;
        buffer[3] += (float)color[3] / 4.0f;

        uint16_t r = min((uint16_t)round(buffer[0]), (uint16_t)16383);
        uint16_t g = min((uint16_t)round(buffer[1]), (uint16_t)16383);
        uint16_t b = min((uint16_t)round(buffer[2]), (uint16_t)16383);
        uint16_t w = min((uint16_t)round(buffer[3]), (uint16_t)16383);

        buffer[0] = max(0.0f, buffer[0] - r);
        buffer[1] = max(0.0f, buffer[1] - g);
        buffer[2] = max(0.0f, buffer[2] - b);
        buffer[3] = max(0.0f, buffer[3] - w);

        analogWrite(PIN_R, r);
        analogWrite(PIN_G, g);
        analogWrite(PIN_B, b);
        analogWrite(PIN_W, w);
    }

    virtual char GetFlags()
    {
        return (char)(HAS_16BIT_COLOR | HAS_W_CHANNEL);
    }

private:
    uint16_t color[4];
    float buffer[4];
};