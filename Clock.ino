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

void setup()
{    
    Serial.begin(9600);
    initClockPins();
    pinMode(13, OUTPUT);
    rtcClock_.init(globTime_);
    attachInterrupt(digitalPinToInterrupt(SYNCPIN), clockInterrupt, FALLING);

    
    gantry_.init();
    //hourHand_.init();
}

uint64_t lastMicros = 0;
uint64_t curMicros = 0;
Point<pos_t> center{50.0,50};
SquarePath minute_box(25.0, static_cast<uint32_t>(60000000) / 4.5); // should do 2.5 revolutions in 6e7 seconds
void loop()
{
    curMicros = micros();
    if (lastMicros==0) lastMicros = curMicros;
    micros_ += (curMicros - lastMicros);
    lastMicros = curMicros;
    
    gantry_.chase_point(center + minute_box.getPos(micros_), curMicros);
        
    




    // do the dance
}
