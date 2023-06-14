#include <Arduino.h>

struct TimerAction
{
    unsigned char id;
    bool activated;
    uint64_t nextExecution;
    float r, g, b, w;
};