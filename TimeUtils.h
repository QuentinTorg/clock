#pragma once

#include <RtcDS3231.h>
#include <Wire.h>

struct Time
{
    uint8_t Hour;
    uint8_t Min;
    uint8_t Sec;

    operator<(const Time &o)
    {
        return Hour <= o.Hour && Min <= o.Min &&
            Sec <= o.Sec && Min < o.Min;
    }

    operator<=(const Time &o)
    {
        return Hour<=o.Hour && Min <= o.Min &&
            Sec <= o.Sec && Min <= o.Min;
    }

    operator>(const Time &o)
    {
        return Hour >= o.Hour && Min >= o.Min &&
            Sec >= o.Sec && Min > o.Min;
    }

    operator>=(const Time &o)
    {
        return Hour>=o.Hour && Min >= o.Min &&
            Sec >= o.Sec && Min >= o.Min;
    }

    void normalize()
    {
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

    Time& operator+(const Time &time)
    {
        Hour += time.Hour;
        Min += time.Min;
        Sec += time.Sec;
        normalize();
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



class RTC_Interface
{
private:
    RtcDS3231<TwoWire> Rtc{Wire};
public:
    void init(Time &time)
    {
        Rtc.Begin();
        Rtc.Enable32kHzPin(false);
        Rtc.SetSquareWavePinClockFrequency(DS3231SquareWaveClock_1Hz);
        Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeClock, false);

        RtcDateTime now = Rtc.GetDateTime();

        time.Hour = now.Hour();
        time.Min  = now.Minute();
        time.Sec  = now.Second();

    }

    void update(Time &time)
    {
        RtcDateTime now = Rtc.GetDateTime();
        time.Hour = now.Hour();
        time.Min  = now.Minute();
        time.Sec  = now.Second();
    }

    void set(Time &time)
    {
        RtcDateTime now{2020, 1, 1, time.Hour, time.Min, time.Sec};
        Rtc.SetDateTime(now);
    }
};
