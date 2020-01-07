#pragma once

#include "ClockAPI.h"

// a path is a mapping of time to location
class Path
{
    virtual Point<pos_t> getPos(int32_t micros) = 0;
};

// class CirclePath : Path
// {
// private:
//     unsigned long R_;
//     unsigned long D_;
// public:
//     CirclePath(unsigned long Radius, unsigned long Duration) : R_(Radius), D_(Duration) {};
//     Point<float> getPos(const Time &t)
//     {
//         // do magic
//     };
// };


class LinePath : Path
{
private:
    int32_t S_;
    int32_t D_;

public:
    LinePath(unsigned long Radius, unsigned long Duration) : S_(Radius), D_(Duration)
    {
    };

    Point<float> getPos(int32_t micros)
    {
        micros = micros % D_;
        if (micros < D_/2)
            //return {micros*static_cast<float>(S_)/D_ - S_/2,S_/2};
            return {S_/2,micros*static_cast<float>(S_)/D_ - S_/2};
        else
            //return {S_/2 - micros*static_cast<float>(S_)/D_,S_/2};
            return {S_/2,S_/2 - micros*static_cast<float>(S_)/D_};
    };
};

class SquarePath : Path
{
    pos_t S_;
    int32_t D_;
    int32_t side_dur_;
    int32_t quarter_dur_;
    pos_t half_size_;
    pos_t speed_;
public:
    SquarePath(pos_t Size, int32_t Duration) : S_(Size), D_(Duration) 
    {
        side_dur_ = D_/4;
        quarter_dur_ = side_dur_/2;
        half_size_ = S_/2;
        speed_ = static_cast<pos_t>(S_*4)/D_;
    };

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

    Point<pos_t> getPos(int32_t micros)
    {
        micros = micros % D_;
        // left segment, top-half
        if (micros <= quarter_dur_)
        {
            // Serial.println("left_top");
            return {-half_size_, -micros*speed_};
        }
        // top segment
        else if (micros <= side_dur_ + quarter_dur_)
        {
            // Serial.println("top");
            return {(micros-quarter_dur_)*speed_ - half_size_,-half_size_};
        }
        // right segment
        else if (micros <= 2*side_dur_ + quarter_dur_)
        {
            // Serial.println("right");
            return {half_size_, (micros-quarter_dur_-side_dur_)*speed_ - half_size_};
        }
        // bottom segment
        else if (micros <= 3*side_dur_+ quarter_dur_)
        {
            // Serial.println("bottom");
            //return {(micros-quarter_dur_-2*side_dur_)*-speed_ - half_size_,half_size_};
            return {half_size_ - (micros-2*side_dur_-quarter_dur_)*speed_,half_size_};
        }
        // left segment, bottom-half
        else 
        {
            // Serial.println("left_bottom");
            return {-half_size_,-(micros-quarter_dur_-3*side_dur_)*speed_ + half_size_};
        }
    };
};

// class BoardPathMapper : Path
// {
// private:
//     inline static constexpr Point<float> boardOffset_{1600,2400};
//     inline static constexpr Point<float> minuteSquareOffset_{946,1480};
//     inline static constexpr Point<float> minutePathX{0,1000};
//     inline static constexpr Point<float> minutePathY{400,0};
//     SquarePath minutePath{200,15000};
// public:
//     Point<float> getPos(const Time &t)
//     {
//        return minuteSquareOffset_ + minutePath.getPos(t)
//            + minutePathX*(t.Min%10) + minutePathY*(t.Min/10);
//     };
// };
// 
// class HourPosMapper
// {
// private:
//     inline static constexpr unsigned long stepsPerHour = 167;
//     inline static constexpr unsigned long totalHSteps = 2000;
// public:
//     pos_t getPos(const Time &t)
//     {
//         unsigned hour = t.Hour > 12 ? t.Hour - 12 : t.Hour;
//         if ( hour > 1 && hour < 8)
//         {
//             return (hour*stepsPerHour + t.Min*stepsPerHour/60 + stepsPerHour);
//         }
//         else
//         {
//             return (hour*stepsPerHour + t.Min*stepsPerHour/60);
//         }
//     };
// };
