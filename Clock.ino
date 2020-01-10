#define TIME_MULTIPLIER 1

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
    globTime_.Sec += TIME_MULTIPLIER;
    globTime_.normalize();
    if (globTime_.Sec == 0)
        micros_ = 0;
}

uint32_t lastMicros = 0;
uint32_t curMicros = 0;
void setup()
{
    Serial.begin(115200);
    Serial.println("starting");
    initClockPins();

    pinMode(13, OUTPUT);

    // set up RTC
    rtcClock_.init(globTime_);
    attachInterrupt(digitalPinToInterrupt(SYNCPIN), clockInterrupt, FALLING);

    // set up hours and minutes
    gantry_.init();
    hourHand_.init();


    // set up micros tracking
    int sec = globTime_.Sec;
    while (sec == globTime_.Sec) 
    { 
        Serial.println("waiting for sec");
    } // wait until the next second changeover
    micros_ = globTime_.Sec * 1000000;  // assign micros_. not perfect but very close
    curMicros = micros();
    lastMicros = curMicros;
}

MinutesPathMapper minutesPath_;
HourPosMapper hourPath_;
void loop()
{
    curMicros = micros()*TIME_MULTIPLIER;
    micros_ += (curMicros - lastMicros);
    lastMicros = curMicros;
    if (false)
    {
        Serial.print(globTime_.Hour);
        Serial.print(":");
        Serial.print(globTime_.Min);
        Serial.print(":");
        Serial.println(globTime_.Sec);
    }
    // gantry_.chase_point(dest, micros());
    // auto point = minutesPath_.getPos(0, micros_);
    // gantry_.chase_point(point, curMicros);

    
    //     
    // Serial.println("looping");

    auto point = minutesPath_.getPos(globTime_.Min, micros_);
    gantry_.chase_point(point, curMicros);

    auto angle = hourPath_.getPos(globTime_, micros_);
    hourHand_.chase_angle(angle, curMicros);
}
