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
        return Hour <= o.Hour && Min <= o.Min &&
            Sec <= o.Sec && Min < o.Mil;
    }

    operator<=(const Time &o)
    {
        return Hour<=o.Hour && Min <= o.Min &&
            Sec <= o.Sec && Min <= o.Mil;
    }

    operator>(const Time &o)
    {
        return Hour >= o.Hour && Min >= o.Min &&
            Sec >= o.Sec && Min > o.Mil;
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
    unsigned long R_;
    unsigned long D_;
public:
    CirclePath(unsigned long Radius, unsigned long Duration) : R_(Radius), D_(Duration) {};
    Point getPos(const Time &t)
    {
        // do magic
    };
};

class SquarePath : Path
{
    unsigned long S_;
    unsigned long D_;
public:
    SquarePath(unsigned long Size, unsigned long Duration) : S_(Size), D_(Duration) {};

    /* square path is a continuous interval as such:
     *
     *    2------3
     *    |      |
     *    1      4
     *    |      |
     *    6------5
     *
     * Where the entire path takes Duration to complete, then repeats
     */

    Point getPos(const Time &t)
    {
        unsigned long sideDur = D_/4;
        unsigned long normTime = (t.Mil > D_ ? t.Mil % D_ : t.Mil) + sideDur/2;
        switch(normTime/sideDur)
        {
            case 1:  // top side
                return {S_*(normTime-sideDur/2)/sideDur,0};
            case 2:  // right side
                return {S_,S_*(normTime-sideDur)/sideDur};
            case 3:  // bottom side
                return {S_-S_*(normTime-3*sideDur/2)/sideDur,S_};
            default:
                return {0,((normTime-sideDur/2)%sideDur)*S_/sideDur};
        }
    };
};

class BoardPathMapper : Path
{
private:
    inline static constexpr Point boardOffset_{1600,2400};
    inline static constexpr Point minuteSquareOffset_{946,1480};
    inline static constexpr Point minutePathX{0,1000};
    inline static constexpr Point minutePathY{400,0};
    SquarePath minutePath{200,15000};
public:
    Point getPos(const Time &t)
    {
       return minuteSquareOffset_ + minutePath.getPos(t)
           + minutePathX*(t.Min%10) + minutePathY*(t.Min/10);
    };
};

class HourPosMapper
{
private:
    inline static constexpr unsigned long stepsPerHour = 167;
    inline static constexpr unsigned long totalHSteps = 2000;
public:
    pos_t getPos(const Time &t)
    {
        unsigned hour = t.Hour > 12 ? t.Hour - 12 : t.Hour;
        if ( hour > 1 && hour < 8)
        {
            return (hour*stepsPerHour + t.Min*stepsPerHour/60 + stepsPerHour);
        }
        else
        {
            return (hour*stepsPerHour + t.Min*stepsPerHour/60);
        }
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
    //Serial.begin(9600);
    gantry_.init();
    hourHand_.init();
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

    auto gpos = pathMapper_.getPos(globTime_);
    auto hpos = hourPosMapper_.getPos(globTime_);

    gantry_.moveTo(gpos);
    hourHand_.moveTo(hpos);
}
