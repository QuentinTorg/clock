#include "ClockAPI.h"
#include "TimeUtils.h"
#include "PathUtils.h"



// -- MAIN LOGIC BEGIN


RTC_Interface rtcClock_;
//BoardPathMapper pathMapper_;
//HourPosMapper hourPosMapper_;

Gantry gantry_;
HourHand hourHand_;
Time globTime_;
uint32_t micros_;

void clockInterrupt()
{
    globTime_.Sec++;
    globTime_.normalize();
    if (globTime_.Sec == 0)
        micros_ = 0;
}

uint32_t lastMicros = 0;
uint32_t curMicros = 0;
void setup()
{
    Serial.begin(9600);
    initClockPins();

    pinMode(13, OUTPUT);

    // set up RTC
    rtcClock_.init(globTime_);
    attachInterrupt(digitalPinToInterrupt(SYNCPIN), clockInterrupt, FALLING);

    // set up hours and minutes
    gantry_.init();
    //hourHand_.init();


    // set up micros tracking
    int sec = globTime_.Sec;
    while (sec == globTime_.Sec) { } // wait until the next second changeover
    micros_ = globTime_.Sec * 1000000;  // assign micros_. not perfect but very close
    curMicros = micros();
    lastMicros = curMicros;
}

MinutesPathMapper minutesPath_;
void loop()
{
    curMicros = micros();
    micros_ += (curMicros - lastMicros);
    lastMicros = curMicros;

    auto point = minutesPath_(globTime_.Min, micros_);
    gantry_.chase_point(point, curMicros);







    // do the dance
}
