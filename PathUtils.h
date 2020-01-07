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
    const int32_t S_;
    const int32_t D_;

public:
    LinePath(unsigned long Radius, unsigned long Duration) : S_(Radius), D_(Duration)
    {
    };

    Point<pos_t> getPos(int32_t micros)
    {
        micros = micros % D_;
        if (micros < D_/2)
            //return {micros*static_cast<float>(S_)/D_ - S_/2,S_/2};
            return {static_cast<pos_t>(S_/2.0),micros*static_cast<pos_t>(S_)/D_ - S_/2};
        else
            //return {S_/2 - micros*static_cast<float>(S_)/D_,S_/2};
            return {static_cast<pos_t>(S_/2.0),S_/2 - micros*static_cast<float>(S_)/D_};
    };
};

class SquarePath : Path
{
    const pos_t S_;
    const int32_t D_;
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
            return {-half_size_, -micros*speed_};
        }
        // top segment
        else if (micros <= side_dur_ + quarter_dur_)
        {
            return {(micros-quarter_dur_)*speed_ - half_size_,-half_size_};
        }
        // right segment
        else if (micros <= 2*side_dur_ + quarter_dur_)
        {
            return {half_size_, (micros-quarter_dur_-side_dur_)*speed_ - half_size_};
        }
        // bottom segment
        else if (micros <= 3*side_dur_+ quarter_dur_)
        {
            return {half_size_ - (micros-2*side_dur_-quarter_dur_)*speed_,half_size_};
        }
        // left segment, bottom-half
        else
        {
            return {-half_size_,-(micros-quarter_dur_-3*side_dur_)*speed_ + half_size_};
        }
    };
};

class MinutesPathMapper
{
private:
    // TODO: fix these constants
    static constexpr Point<pos_t> gridDims_mm_{300.0, 300.0};
    static constexpr Point<uint8_t> gridSize_{10, 6};
    static constexpr Point<pos_t> gridOffset_{25.0, 25.0};
    static constexpr Point<pos_t> gridSpacing_{gridDims_mm_.x / gridSize_.x, gridDims_mm_.y / gridSize_.y};

    SquarePath minuteSquare_{gridSpacing_.x, static_cast<uint32_t>(static_cast<uint32_t>(60000000) / 3.5)}; // 3.5 loops per minute
public:
    Point<pos_t> getPos(uint8_t minute, uint32_t micros)
    {
        // TODO: add some sort of return path for going from 59 seconds to 0.
        Point<pos_t> minuteCenter{gridSpacing_.x * (minute % gridSize_.x) + gridOffset_.x,
                                  gridSpacing_.y * (minute / gridSize_.x) + gridOffset_.y};
        return minuteCenter + minuteSquare_.getPos(micros);
    }
};

class HourPosMapper
{
private:
    // saving these to remember how we figured out timing
    // static constexpr unsigned long stepsPerHour = 167;
    // static constexpr unsigned long totalHSteps = 2000;

    // TODO: fix these constants
    static constexpr pos_t degOffset = 15.0;
    static constexpr pos_t degTotal = 145.0;
    static constexpr pos_t degPerHour = 13.0 / degTotal;
public:

    // TODO: add an animation every hour
    // will blend the skip over the 13th hour
    // will show off the clock motion too

    pos_t getPos(const Time &t, uint32_t micros)
    {
        uint8_t hour = t.Hour > 12 ? t.Hour - 12 : t.Hour; // remove 24 hour timekeeping

        if (hour < 8) { hour += 12; } // handle running from 8-8 range
        if (hour > 12) { hour += 1; } // skip the 13th hour
        hour -= 8; // index everything back so 8 is 0 index

        return hour*degPerHour + t.Min*degPerHour/60 + degOffset;
    };
};
