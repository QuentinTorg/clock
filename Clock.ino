#include "ClockAPI.h"
#include "TimeUtils.h"
#include "PathUtils.h"

// -- MAIN LOGIC BEGIN

RTC_Interface rtcClock_;
Time globTime_;
uint32_t micros_;

ClockUI clockUI_;

Gantry gantry_;
HourHand hourHand_;

void clockInterrupt()
{
    globTime_.Sec += 1;
    globTime_.normalize();
    if (globTime_.Sec == 0)
        micros_ = 0;
}

uint32_t lastMicros = 0;
uint32_t curMicros = 0;
void setup()
{
    // set up RTC
    rtcClock_.init(globTime_);
    attachInterrupt(digitalPinToInterrupt(SYNCPIN), clockInterrupt, FALLING);

    // set up hours and minutes
    gantry_.init();
    hourHand_.init();
    clockUI_.init();

    // assign micros_. not perfect but very close
    micros_ = globTime_.Sec * 1000000;
    curMicros = micros();
    lastMicros = curMicros;
}

MinutesPathMapper minutesPath_;
HourPosMapper hourPath_;
void loop()
{
    curMicros = micros();
    micros_ += (curMicros - lastMicros);
    lastMicros = curMicros;

    clockUI_.update(globTime_, rtcClock_, curMicros);

    auto point = minutesPath_.getPos(globTime_.Min, micros_);
    gantry_.chase_point(point, curMicros);

    auto angle = hourPath_.getPos(globTime_, micros_);
    hourHand_.chase_angle(angle, curMicros);


}
