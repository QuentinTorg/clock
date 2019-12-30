// x axis pins
const int MXSTEP = 3;
const int MXDIR = 2;

// y axis pins
const int MYSTEP = 5;
const int MYDIR = 4;

// hour axis pins
const int MHSTEP = A2; //A2
const int MHDIR = A3; //A3

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
  // x motor setup
  pinMode(MXSTEP, OUTPUT);
  pinMode(MXDIR, OUTPUT);

  // y motor setup
  pinMode(MYSTEP, OUTPUT);
  pinMode(MYDIR, OUTPUT);

  // hour motor setup
  pinMode(MHSTEP, OUTPUT);
  pinMode(MHDIR, OUTPUT);

  // zeroing switch setup
  pinMode(LSWITCHY, INPUT_PULLUP);
  pinMode(LSWITCHX, INPUT_PULLUP);
  pinMode(LSWITCHH, INPUT_PULLUP);

  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  pinMode(BUTTON3, INPUT_PULLUP);
}

// step the hour hand in a direction
void h_step(unsigned char dir)
{
  dir = dir == HIGH ? LOW : HIGH;
  Serial.println("in h_step");
  digitalWrite(MHDIR, dir);
  digitalWrite(MHSTEP, HIGH);
  digitalWrite(MHSTEP, LOW);
}

// step the gantry x-axis in a direction
void x_step(unsigned char dir)
{
 dir = dir == HIGH ? LOW : HIGH;
 digitalWrite(MXDIR, dir);
 digitalWrite(MXSTEP, HIGH);
 digitalWrite(MXSTEP, LOW);
}

// step the gantry y-axis in a direction
void y_step(unsigned char dir)
{
  digitalWrite(MYDIR, dir);
  digitalWrite(MYSTEP, HIGH);
  digitalWrite(MYSTEP, LOW);
}

// move y towards zero if not zeroed
bool zeroy(bool dir)
{
  if (digitalRead(LSWITCHY) == dir) 
  {
    y_step(dir);
    return false;
  }
  return true;
}

// move x towards zero if not zeroed
bool zerox(bool dir)
{
  if (digitalRead(LSWITCHX) == dir) {
    x_step(dir);
    return false;
  }
  return true;
}

// move h towards zero if not zeroed
bool zeroh(bool dir)
{
  if (digitalRead(LSWITCHH) == dir) {
    h_step(dir);
    return false;
  }
  return true;
}

// zero all axes
void zero_all(int fast_delay_micros, int slow_delay_micros)
{
  bool x_zeroed = false;
  bool y_zeroed = false;
  bool h_zeroed = false;
  bool directions[] = {DIR_MINUS, DIR_PLUS, DIR_MINUS};
  unsigned int delays[] = {fast_delay_micros, fast_delay_micros, slow_delay_micros};

  // will go towards limit quickly, away from limit quickly, then approach limit again slowly
  // makes sure that we did not start with limit switch alread depressed
  for (int i = 0; i < 3 ; i++)
  {
    while (!x_zeroed || !y_zeroed || !h_zeroed)
    {
      x_zeroed = zerox(directions[i]);
      y_zeroed = zeroy(directions[i]);
      h_zeroed = zeroh(directions[i]);
      delayMicroseconds(delays[i]);
    }
  }
}

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
    Point dst_ {0,0};
    Point pos_ {0,0};
    Point prev_ {0,0};
    unsigned long delta;
public:
    void zero()
    {
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
              x_step(DIR_PLUS);
              ++pos_.x;
          }
          else if (pos_.x > dst_.x)
          {
              x_step(DIR_MINUS);
              --pos_.x;
          }
        }

        if (pos_.y < dst_.y)
        {
            y_step(DIR_PLUS);
            ++pos_.y;
        }
        else if (pos_.y > dst_.y)
        {
            y_step(DIR_MINUS);
            --pos_.y;
        }
    }
};

class HourHand
{
private:
    pos_t pos_ {0}; // model of where we're supposed to be
    pos_t dst_ {0}; // actual cursor location
    unsigned long delta {0};
public:
    void zero()
    {
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
            h_step(DIR_PLUS);
            ++pos_;
        }
        else if (pos_ > dst_)
        {
            h_step(DIR_MINUS);
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
