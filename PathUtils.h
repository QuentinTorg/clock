
// a path is a mapping of time to location
class Path
{
    virtual Point<float> getPos(const Time &t) = 0;
};

class CirclePath : Path
{
private:
    unsigned long R_;
    unsigned long D_;
public:
    CirclePath(unsigned long Radius, unsigned long Duration) : R_(Radius), D_(Duration) {};
    Point<float> getPos(const Time &t)
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

    Point<float> getPos(const Time &t)
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
    inline static constexpr Point<float> boardOffset_{1600,2400};
    inline static constexpr Point<float> minuteSquareOffset_{946,1480};
    inline static constexpr Point<float> minutePathX{0,1000};
    inline static constexpr Point<float> minutePathY{400,0};
    SquarePath minutePath{200,15000};
public:
    Point<float> getPos(const Time &t)
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
