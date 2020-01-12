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
    globTime_.Hour = 12;
    globTime_.Min = 0;
    globTime_.Sec = 45;


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
bool toggle = true;
const Point<pos_t> point1 = {20.0,20.0};
const Point<pos_t> point2 = {100.0,20.0};
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

    auto point = minutesPath_.getPos(globTime_.Min, micros_);
    gantry_.chase_point(point, curMicros);
//    if (toggle) 
//    {
//        if (gantry_.chase_point(point1,curMicros)) toggle = !toggle;
//    }
//    else
//    {
//        if (gantry_.chase_point(point2,curMicros)) toggle = !toggle;
//    }

    auto angle = hourPath_.getPos(globTime_, micros_);
    //Serial.println(angle);
    hourHand_.chase_angle(angle, curMicros);
}
