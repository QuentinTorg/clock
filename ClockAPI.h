#pragma once

#include "TimeUtils.h"

// x axis pins
constexpr int MXSTEP = 3;
constexpr int MXDIR = A2;

// y axis pins
constexpr int MYSTEP = 5;
constexpr int MYDIR = 4;

// hour axis pins
constexpr int MHSTEP = 7; //A2
constexpr int MHDIR = 6; //A3

// limit switches
constexpr int LSWITCHY = 10;
constexpr int LSWITCHX = 9;
constexpr int LSWITCHH = 8;

// time setting buttons
constexpr int HR_MIN_BUTTON_PIN = A3;
constexpr int UP_BUTTON_PIN = A6;
constexpr int DOWN_BUTTON_PIN = A7;

constexpr int SYNCPIN = 2;

template <uint8_t pin>
class Button
{
public:
    void init()
    {
        pinMode(pin, INPUT_PULLUP);
        prevState_ = digitalRead(pin);
        lastChange_ = micros();
    }

    // return true on transition from button press, as well as true once per repeatTime after that
    bool new_press(const uint32_t micros)
    {
        bool curState = prevState_;

        // don't read the new pin if within debounce time
        if (micros - lastChange_ > dbTime_)
        {
            curState = digitalRead(pin);
            if (curState != prevState_) lastChange_ = micros;
        }

        bool wasPushed = false;
        if (curState == pushed_ &&
                (curState != prevState_ || micros - lastRepeat_ > repeatTime_))
        {
            wasPushed = true;
            lastRepeat_ = micros;
        }

        prevState_ = curState;
        return wasPushed;
    }

    bool is_pressed()
    {
        return digitalRead(pin) == pushed_;
    }

private:
    static constexpr uint32_t dbTime_ = 10000; // 10ms switch debounce time
    static constexpr uint32_t repeatTime_ = 500000; // repeat every 0.5 seconds

    static constexpr bool pushed_ = LOW;

    bool prevState_;
    uint32_t lastChange_;
    uint32_t lastRepeat_;
};


// initializes motor pins - must be called before any of the following functions
class ClockUI
{
public:
    void init()
    {
        upButton.init();
        downButton.init();
        hrMinButton.init();
    }

    void update(Time &time, RTC_Interface &rtc, const uint32_t micros)
    {
        if (upButton.new_press(micros))
        {
            indexTime(time, rtc, up_);
        }
        if (downButton.new_press(micros))
        {
            indexTime(time, rtc, down_);
        }
    }

private:
    Button<UP_BUTTON_PIN> upButton;
    Button<DOWN_BUTTON_PIN> downButton;
    Button<HR_MIN_BUTTON_PIN> hrMinButton;

    static constexpr bool hourSet_ = false;
    static constexpr bool minSet_ = !hourSet_;

    static constexpr bool up_ = true;
    static constexpr bool down_ = !up_;

    void indexTime(Time &t, RTC_Interface &rtc, bool dir)
    {
        int8_t increment = (dir == up_ ? 1 : -1);
        int8_t hour = t.Hour;
        int8_t min = t.Min;

        if (hrMinButton.is_pressed() == hourSet_) hour += increment;
        else min += increment;

        if (hour > 23) hour = 0;
        else if (hour < 0) hour = 23;

        if (min > 59) min = 0;
        if (min < 0) min = 59;

        t.Min = min;
        t.Hour = hour;

        rtc.set(t);
    }
};

// higher level abstractions
using step_t=int32_t;
using pos_t=float;
using ang_t=float;

template <uint8_t step_pin, uint8_t dir_pin, uint8_t lim_pin, bool flipped, uint32_t max_speed>
class Motor
{
private:
    static constexpr uint16_t min_step_time = {static_cast<uint16_t>(1000000.0/max_speed + 0.5)}; // micros per step at max speed

    uint32_t prev_micros = {0};
    step_t cur_step = {0};

    // motor directions
    static constexpr bool DIR_MINUS = HIGH;
    static constexpr bool DIR_PLUS = LOW;

    void step(bool dir, uint32_t micros)
    {
        cur_step = dir == DIR_PLUS ? cur_step + 1 : cur_step - 1;

        if (flipped)
        {
            dir = dir == HIGH ? LOW : HIGH;
        }
        digitalWrite(dir_pin, dir);
        digitalWrite(step_pin, HIGH);
        digitalWrite(step_pin, LOW);

        prev_micros = micros;
    }

    bool zero_step(const bool dir)
    {
        if (digitalRead(lim_pin) == dir)
        {
            step(dir, micros());
            return false;
        }
        return true;
    }

public:
    void init()
    {
        pinMode(step_pin, OUTPUT);
        pinMode(dir_pin, OUTPUT);
        pinMode(lim_pin, INPUT_PULLUP);
    }

    void zero(const uint16_t ff_delay, const uint16_t r_delay, const uint16_t sf_delay)
    {
        const bool directions[] = {DIR_MINUS, DIR_PLUS, DIR_MINUS};
        const unsigned delays[] = {ff_delay, r_delay, sf_delay};

        // will go towards limit, away from limit, then approach limit again slowly
        // makes sure that we did not start with limit switch alread depressed
        for (int i = 0; i < 3; i++)
        {
            while (!zero_step(directions[i]))
            {
                delayMicroseconds(delays[i]);
            }
        }

        cur_step = 0;
    }

    // decides direction and step timing to prevent violation of max speed and accel
    bool chase_step(const step_t target_step, const uint32_t cur_micros)
    {
        step_t delta_distance = target_step - cur_step;
        if (delta_distance == 0) return true;

        uint32_t delta_micros = cur_micros - prev_micros; // rollover is handled well with uints
        if (delta_micros < min_step_time) return false;
        bool direction = delta_distance > 0 ? DIR_PLUS : DIR_MINUS;
        step(direction, cur_micros);

        return false;
    }

};

template <typename T>
struct Point
{
    T x,y;

    constexpr Point() : x(0), y(0) {}
    constexpr Point(T x, T y) : x(x), y(y) {}

    constexpr Point operator+(const Point &o) const
    {
        return {x+o.x,y+o.y};
    }

    template <typename ScalarT>
    constexpr Point operator*(const ScalarT o) const
    {
        return {x*o,y*o};
    }
};

class Gantry
{
private:
    static constexpr uint8_t steps_per_mm_ = {200 * 16 / 20 / 2};

    Motor<MXSTEP, MXDIR, LSWITCHX, true, steps_per_mm_ * static_cast<uint32_t>(200)> x_motor_;
    Motor<MYSTEP, MYDIR, LSWITCHY, false, steps_per_mm_ * static_cast<uint32_t>(200)> y_motor_;

    // 200 full steps per rev
    // 16 microstepping
    // 20 teeth per rev pulley
    // 2mm travel per tooth


    Point<step_t> mm_to_steps(const Point<pos_t> &mm_point) {
        return Point<step_t>{static_cast<step_t>(mm_point.x * steps_per_mm_ + 0.5), static_cast<step_t>(mm_point.y * steps_per_mm_ + 0.5)};
    }

public:

    void init()
    {
      y_motor_.init();
      x_motor_.init();

      y_motor_.zero(200, 2000, 2000);
      x_motor_.zero(200, 2000, 2000);
    }

    bool chase_point(const Point<pos_t> &point_mm, const uint32_t cur_micros)
    {
        Point<step_t> point_steps = mm_to_steps(point_mm);
        bool x_success = x_motor_.chase_step(point_steps.x, cur_micros);
        bool y_success = y_motor_.chase_step(point_steps.y, cur_micros);
        return x_success && y_success;
    }
};

class HourHand
{
private:
    // 3:1 gear reduction
    // 200 full steps per revolution
    // 16:1 micro-steps to step

    static constexpr float steps_per_deg_ = (3.0*200.0*16.0)/360.0;

    Motor<MHSTEP, MHDIR, LSWITCHH, true, static_cast<uint32_t>(steps_per_deg_ * 150)> motor_;

    step_t angle_to_steps(const ang_t angle) {
        return steps_per_deg_ * angle + 0.5;
    }

public:
    void init()
    {
        motor_.init();
        motor_.zero(400, 2000, 2000);
    }

    bool chase_angle(const ang_t &angle, const uint32_t &cur_micros)
    {
        return motor_.chase_step(angle_to_steps(angle), cur_micros);
    }
};
