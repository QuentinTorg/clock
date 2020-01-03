#include "ClockAPI.h"
//#include "TimeUtils.h"
//#include "PathUtils.h"



// -- MAIN LOGIC BEGIN


//RTC_Interface rtcClock_;
//BoardPathMapper pathMapper_;
//HourPosMapper hourPosMapper_;

Gantry gantry_;
HourHand hourHand_;

void setup()
{
    initClockPins();
//    rtcClock_.init(globTime_);
    //Serial.begin(9600);
    gantry_.init();
    hourHand_.init();

    pinMode(13, OUTPUT);
}

uint32_t prev_sync_micros{0}, cur_micros{0};
constexpr uint32_t sync_duration_micros =  60 * 1000000;

void loop()
{
    // time from boot measured in micros
    // update glob time once per minute, where the minute is tracked with

    Point<pos_t>  start_pt{10,10};
    Point<pos_t>  dest_pt{100,100};
    bool at_dest = false;

    while (!gantry_.chase_point(start_pt, micros()));
    { }
    while (!gantry_.chase_point(dest_pt, micros()));
    { }
    //while (!gantry_.chase_point(dest_pt, micros()));
    //{ }


//    if (cur_micros - prev_sync_micros > sync_duration_micros)
//    {
//        globTime_.sync(globTime_,dt);
//    }




//    if (lastTime == 0) lastTime = time;
//    unsigned dt = time - lastTime;
//    lastTime = time;
//
//    globTime_.addMillis(dt);
//    rtcClock_.sync(globTime_, dt);

    //auto gpos = pathMapper_.getPos(globTime_);
    //auto hpos = hourPosMapper_.getPos(globTime_);

    //gantry_.moveTo(gpos);
    //hourHand_.moveTo(hpos);
}
