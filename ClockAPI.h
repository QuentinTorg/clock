// x axis pins
const int MXSTEP = 3;
const int MXDIR = 2;

// y axis pins
const int MYSTEP = 5;
const int MYDIR = 4;

// hour axis pins
const int MHSTEP = 7; //A2
const int MHDIR = 6; //A3

// motor directions
const bool DIR_MINUS = HIGH;
const bool DIR_PLUS = LOW;

// buttons and switches
const int LSWITCHY = 10;
const int LSWITCHX = 9;
const int LSWITCHH = 8;

const int BUTTON1 = A3;
const int BUTTON2 = A6;
const int BUTTON3 = A7;

// initializes motor pins - must be called before any of the following functions
void initClockPins()
{
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(BUTTON3, INPUT_PULLUP);
}

// higher level abstractions
using step_t=uint32_t;
using pos_t=float;
using ang_t=float;

template <uint8_t step_pin, uint8_t dir_pin, uint8_t lim_pin, bool flipped, uint32_t max_speed, uint32_t max_accel>
class Motor
{
private:
    static constexpr uint16_t min_step_time = {static_cast<uint16_t>(1000000.0/max_speed + 0.5)}; // micros per step at max speed

    uint32_t prev_micros = {0};
    step_t cur_step = {0};
    float cur_speed = {0};
    // max speed units: steps per second
    // max accel units: steps per second per second

    void step(bool dir, uint32_t micros)
    {
        cur_step = dir == DIR_PLUS ? cur_step + 1 : cur_step - 1;
        bool step_sign = dir == DIR_PLUS ? 1 : -1;

        if (flipped)
        {
            dir = dir == HIGH ? LOW : HIGH;
        }
        digitalWrite(dir_pin, dir);
        digitalWrite(step_pin, HIGH);
        digitalWrite(step_pin, LOW);

        prev_micros = micros;
        cur_speed = step_sign * 1000000.0 / micros;

        // it's possible that we may accelerate to higher than max speed for a signle step
        // allow it but pretend it didn't happen becuase it will only happen once
        if (cur_speed > max_speed)
        {
            cur_speed = max_speed;
        }
        else if (cur_speed < -max_speed)
        {
            cur_speed = -max_speed;
        }
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

    float calc_step_accel(bool step_direction, uint32_t delta_micros)
    {
        // calculate acceleration if we would step right now towards delta
        int8_t one_step = step_direction == DIR_PLUS ? 1 : -1;
        return (one_step - cur_speed * delta_micros / 1000000.0) * (2e12 / (delta_micros * delta_micros));
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

        int32_t signed_max_speed = delta_distance > 0 ? max_speed : -max_speed;
        if (signed_max_speed > static_cast<int>(cur_speed + 0.5)) {
            // already moving max speed, don't waste time on accel math
            step(delta_distance > 0 ? DIR_PLUS : DIR_MINUS, cur_micros);
            return delta_distance < 1 && delta_distance > -1;
        }

        bool test_dir = delta_distance > 0 ? DIR_PLUS : DIR_MINUS;
        float test_accel =  calc_step_accel(test_dir, delta_micros);

        if (cur_speed != 0 && delta_distance < 0 != cur_speed < 0 &&
            (test_accel > max_accel || test_accel < -max_accel))
        {
            // current speed is going away from the destination, maybe have to step away first to slow down
            test_dir = !test_dir;
            test_accel =  calc_step_accel(test_dir, delta_micros);
        }

        if (test_accel <= max_accel  && test_accel >= -max_accel) {
            step(test_dir, cur_micros);
            return (test_dir && delta_distance == 1) || (!test_dir && delta_distance == -1);
        }

        // don't step if it would violate max acceleration limit
        return false;
    }

};


template <typename T>
struct Point
{
    T x,y;

    Point<T> operator+(const Point<T> &o) const
    {
        return {x+o.x,y+o.y};
    }

    template <typename ScalarT>
    Point<T> operator*(const ScalarT o) const
    {
        return {x*o,y*o};
    }
};

class Gantry
{
private:
    static constexpr uint8_t steps_per_mm_ = {200 * 16 / 20 / 2};

    Motor<MXSTEP, MXDIR, LSWITCHX, true, steps_per_mm_ * static_cast<uint32_t>(200), steps_per_mm_ * static_cast<uint32_t>(100)> x_motor_;
    Motor<MYSTEP, MYDIR, LSWITCHY, false, steps_per_mm_ * static_cast<uint32_t>(200), steps_per_mm_ * static_cast<uint32_t>(100)> y_motor_;

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

      y_motor_.zero(200, 5000, 5000);
      x_motor_.zero(200, 5000, 5000);
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
    static constexpr float steps_per_deg_ = 100; // TODO fix this

    Motor<MHSTEP, MHDIR, LSWITCHH, true, static_cast<uint32_t>(steps_per_deg_ * 100 + 0.5), static_cast<uint32_t>(steps_per_deg_ * 100 + 0.5)> motor_;

    step_t angle_to_steps(const ang_t angle) {
        return steps_per_deg_ * angle + 0.5;
    }

public:
    void init()
    {
        motor_.init();
        motor_.zero(400, 5000, 5000);
    }

    bool chase_angle(const ang_t &angle, const uint32_t &cur_micros)
    {
        return motor_.chase_step(angle_to_steps(angle), cur_micros);
    }
};
