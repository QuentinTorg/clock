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
const unsigned char DIR_MINUS = HIGH;
const unsigned char DIR_PLUS = LOW;

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

template <unsigned step_pin, unsigned dir_pin, unsigned lim_pin, bool flipped>
class Motor
{
  public:
    void init()
    {
        pinMode(step_pin, OUTPUT);
        pinMode(dir_pin, OUTPUT);
        pinMode(lim_pin, INPUT_PULLUP);
    }

    void step(unsigned char dir)
    {
        if (flipped)
        {
            dir = dir == HIGH ? LOW : HIGH;
        }
        digitalWrite(dir_pin, dir);
        digitalWrite(step_pin, HIGH);
        digitalWrite(step_pin, LOW);
    }

    bool zero_step(unsigned char dir)
    {
        if (digitalRead(lim_pin) == dir)
        {
            step(dir);
            return false;
        }
        return true;
    }
};

// higher level abstractions
using pos_t=unsigned long;

struct Point
{
    pos_t x,y;

    Point operator+(const Point &o) const
    {
        return {x+o.x,y+o.y};
    }

    Point operator*(const pos_t o) const
    {
        return {x*o,y*o};
    }
};

class Gantry
{
private:
    Motor<MXSTEP, MXDIR, LSWITCHX, true> x_motor_;
    Motor<MYSTEP, MYDIR, LSWITCHY, false> y_motor_;
    Point dst_ {0,0};
    Point pos_ {0,0};
    Point prev_ {0,0};
    unsigned long delta;
public:

    void init()
    {
      x_motor_.init();
      y_motor_.init();
      zero();
    }

    void zero()
    {
        bool directions[] = {DIR_MINUS, DIR_PLUS, DIR_MINUS};
        unsigned int delays[] = {200, 200, 2000};

        // will go towards limit quickly, away from limit quickly, then approach limit again slowly
        // makes sure that we did not start with limit switch alread depressed
        for (int i = 0; i < 3 ; i++)
        {
            bool x_zeroed = false;
            bool y_zeroed = false;
            while (!x_zeroed || !y_zeroed)
            {
                x_zeroed = x_motor_.zero_step(directions[i]);
                y_zeroed = y_motor_.zero_step(directions[i]);
                delayMicroseconds(delays[i]);
            }
        }

        pos_ = {0,0};
    }

    void moveTo(const Point &pt)
    {
    }

    void update(unsigned dt)
    {
        for (int i=0; i<7; ++i)
        {
          if (pos_.x < dst_.x)
          {
              x_motor_.step(DIR_PLUS);
              ++pos_.x;
          }
          else if (pos_.x > dst_.x)
          {
              x_motor_.step(DIR_MINUS);
              --pos_.x;
          }
        }

        if (pos_.y < dst_.y)
        {
            y_motor_.step(DIR_PLUS);
            ++pos_.y;
        }
        else if (pos_.y > dst_.y)
        {
            y_motor_.step(DIR_MINUS);
            --pos_.y;
        }
    }
};

class HourHand
{
private:
    Motor<MHSTEP, MHDIR, LSWITCHH, true> motor_;
    pos_t pos_ {0}; // model of where we're supposed to be
    pos_t dst_ {0}; // actual cursor location
    unsigned long delta {0};
public:
    void init()
    {
        motor_.init();
        zero();
    }

    void zero()
    {
        bool directions[] = {DIR_MINUS, DIR_PLUS, DIR_MINUS};
        unsigned int delays[] = {400, 200, 2000};

        // will go towards limit quickly, away from limit quickly, then approach limit again slowly
        // makes sure that we did not start with limit switch alread depressed
        for (int i = 0; i < 3 ; i++)
        {
            bool zeroed = false;
            while (!zeroed)
            {
                zeroed = motor_.zero_step(directions[i]);
                delayMicroseconds(delays[i]);
            }
        }

        pos_ = 0;
    }

    void moveTo(pos_t dst)
    {
        dst_ = dst;
    }

    void update(unsigned dt)
    {
        if (pos_ < dst_)
        {
            motor_.step(DIR_PLUS);
            ++pos_;
        }
        else if (pos_ > dst_)
        {
            motor_.step(DIR_MINUS);
            --pos_;
        }
    }
};

//unsafe and janky
template<typename T, unsigned S>
class FixedVec
{
private:
    T dat_[S];
    unsigned size_{0};
public:
    void push_back(const T& o)
    {
        dat_[size_] = o;
        ++size_;
    };

    T& operator[](unsigned i)
    {
        return dat_[i];
    }
};
