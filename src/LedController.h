#pragma once

#include <WiFiClient.h>

class LedController
{
public:
    LedController()
    {
    }

    virtual void Setup();

    virtual char GetLedCount();
    virtual void ShowColor(float r, float g, float b, float w)
    {
        SetColor(r, g, b, w);
        ShowColors();
    }
    virtual void SetColor(float r, float g, float b, float w);
    virtual void ReadColors(WiFiClient client);
    virtual void ShowColors();
    virtual char GetFlags();
};