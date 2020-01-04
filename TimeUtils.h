#include <RtcDS3231.h>
#include <Wire.h>

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


struct Time
{
    uint8_t hours;
    uint8_t mins;
    uint8_t secs;
    uint32_t micros;

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

    void normalize()
    {
        while (micros >= 1000000)
        {
            micros -= 1000000;
            ++Sec;
            while (secs >= 60)
            {
                ++mins;
                secs -= 60;
                while (mins >= 60)
                {
                    mins -= 60;
                    ++hours;
                    while (hours >= 24)
                    {
                        hours -= 24;
                    }
                }
            }
        }
    }

    void addMicros(uint32_t microseconds)
    {
        micros += microseconds;
        normalize();
    }

   Time& operator+(const uint32_t microseconds)
   {
       addMicros(microseconds);
   }

   Time& operator+(const Time *time)
   {
       hours += time.hours;
       mins += time.mins;
       secs += time.secs;
       micros += time.micros;
       normalize();
   }
};

class RTC_Interface
{
private:
    RtcDS3231<TwoWire> Rtc{Wire};

    constexpr uint32_t sync_timeout{60 * 1000000};
    uint32_t last_sync_micros{0};

public:
    void init(Time &time)
    {
      Rtc.Begin();
      Rtc.Enable32kHzPin(false);
      Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

      RtcDateTime now = Rtc.GetDateTime();
      last_sync_micros = micros();

      time.hours = now.Hour();
      time.mins  = now.Minute();
      time.secs  = now.Second();
      time.micros = 0;

    }

    void update(Time &time)
    {
        uint32_t system_micros = micros();
        if (system_micros - last_sync_micros > sync_timeout)
        {
            last_sync_micros = system_micros;
            RtcDateTime now = Rtc.GetDateTime();
            globTime.Hour = now.Hour();
            globTime.Min  = now.Minute();
            globTime.Sec  = now.Second();
        }

      timer -= dt;
      if (timer < 0)
      {

        timer = timeout;
      }
    }
};

