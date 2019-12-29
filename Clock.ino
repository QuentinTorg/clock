#include "ClockAPI.h"
#include <Wire.h>
#include <RtcDS3231.h>

struct Time
{
    unsigned Hour;
    unsigned Min;
    unsigned Sec;
    unsigned Mil;

    operator<(const Time &o)
    {
        return Hour<o.Hour && Min < o.Min &&
            Sec < o.Sec && Min < o.Mil;
    }

    operator>(const Time &o)
    {
        return Hour>o.Hour && Min > o.Min &&
            Sec > o.Sec && Min > o.Mil;
    }

    operator>=(const Time &o)
    {
        return Hour>=o.Hour && Min >= o.Min &&
            Sec >= o.Sec && Min >= o.Mil;
    }

    void addMillis(unsigned milliseconds)
    {
        Mil += milliseconds;
        while (Mil >= 1000)
        {
            Mil -= 1000;
            ++Sec;
            while (Sec >= 60)
            {
                ++Min;
                Sec -= 60;
                while (Min >= 60)
                {
                    Min -= 60;
                    ++Hour;
                    while (Hour >= 24)
                    {
                        Hour -= 24;
                    }
                }
            }
        }
    }

   Time& operator+(const unsigned milliseconds)
   {
       addMillis(milliseconds);
   }
};

struct TimeRange
{
    Time begin_;
    Time end_;
    TimeRange(Time start, Time stop) : begin_(start),end_(stop)
    {
    }
    bool operator<(const Time &o)
    {
        o < begin_;
    }
    bool operator>(const Time &o)
    {
        o >= end_;
    }
};

class RTC_Updater
{
private:
    RtcDS3231<TwoWire> Rtc{Wire};
    unsigned timer{0};
    inline static unsigned timeout=6000;
public:
    void init(Time &globTime)
    {
      Rtc.Begin();
      Rtc.Enable32kHzPin(false);
      Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
      timer = timeout;

      RtcDateTime now = Rtc.GetDateTime();
      globTime.Hour = now.Hour();
      globTime.Min  = now.Minute();
      globTime.Sec  = now.Second();
    }
    void sync(Time &globTime, unsigned dt)
    {
      timer -= dt;
      if (timer < 0)
      {
        RtcDateTime now = Rtc.GetDateTime();
        globTime.Hour = now.Hour();
        globTime.Min  = now.Minute();
        globTime.Sec  = now.Second();

        timer = timeout;
      }
    }
};

// a path is a mapping of time to location
class Path
{
    virtual Point getPos(const Time &t) = 0;
};

class CirclePath : Path
{
private:
    unsigned R_;
    unsigned D_;
public:
    CirclePath(unsigned Radius, unsigned Duration) : R_(Radius), D_(Duration) {};
    Point getPos(const Time &t)
    {
        // do magic
    };
};

class SquarePath : Path
{
    unsigned S_;
    unsigned D_;
public:
    SquarePath(unsigned Size, unsigned Duration) : S_(Size), D_(Duration) {};
    // do magic
    Point getPos(const Time &t)
    {
    };
};

class BoardPathMapper : Path
{
private:
    inline static Point boardOffset_{1600,2400};
    inline static Point minuteSquareOffset_{946,1480};
    SquarePath minutePath{400,1000};
public:
    Point getPos(const Time &t)
    {
       return minuteSquareOffset_ + minutePath.getPos(t);
    };
};

class HourPosMapper
{
public:
    pos_t getPos(const Time &t)
    {
        // some spiffy function for the arm
    };
};


// -- MAIN LOGIC BEGIN


RTC_Updater rtcClock_;
Time globTime_{0,0,0,0};
BoardPathMapper pathMapper_;
HourPosMapper hourPosMapper_;

Gantry gantry_;
HourHand hourHand_;

void setup()
{
    initClockPins();
    rtcClock_.init(globTime_);    
    gantry_.zero();
    hourHand_.zero();
}

unsigned long time{0}, lastTime{0};
void loop()
{
    time = millis();
    if (lastTime == 0) lastTime = time;
    unsigned dt = time - lastTime;
    lastTime = time;

    globTime_.addMillis(dt);
    rtcClock_.sync(globTime_, dt);

    auto gpos = pathMaper_.getPos(globTime_);
    auto hpos = ourPosMapper(globTime_);

    gantry_.moveTo(gpos);
    hourHand_.moveTo(hpos);
}
