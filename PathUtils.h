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
    constexpr LinePath(unsigned long Radius, unsigned long Duration) : S_(Radius), D_(Duration)
    {
    };

    Point<pos_t> getPos(int32_t micros) const
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

class SquarePath
{
    const pos_t V_;
    const pos_t H_;
    const int32_t D_;
    int32_t v_side_dur_;
    int32_t h_side_dur_;
    int32_t quarter_dur_;
    pos_t v_half_size_;
    pos_t h_half_size_;
    pos_t speed_;
public:
    SquarePath(pos_t Horizontal, pos_t Vertical, int32_t Duration) : V_(Vertical), H_(Horizontal), D_(Duration)
    {
        int32_t total_len = (V_+H_)*2;
        v_side_dur_ = V_*D_/total_len;
        h_side_dur_ = H_*D_/total_len;
        quarter_dur_ = v_side_dur_/2;
        v_half_size_ = V_/2;
        h_half_size_ = H_/2;
        speed_ = static_cast<pos_t>(total_len)/D_;
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

    Point<pos_t> getPos(int32_t micros, bool rev)
    {
        micros = micros % D_;
        
        Point<pos_t> outPoint;
        // left segment, top-half
        if (micros <= quarter_dur_)
        {
            outPoint = {-h_half_size_, -micros*speed_};
        }
        // top segment
        else if (micros <= h_side_dur_ + quarter_dur_)
        {
            outPoint = {(micros-quarter_dur_)*speed_ - h_half_size_,-v_half_size_};
        }
        // right segment
        else if (micros <= h_side_dur_ + v_side_dur_ + quarter_dur_)
        {
            outPoint = {h_half_size_, (micros-quarter_dur_-h_side_dur_)*speed_ - v_half_size_};
        }
        // bottom segment
        else if (micros <= 2*h_side_dur_+ v_side_dur_ + quarter_dur_)
        {
            outPoint = {h_half_size_ - (micros-h_side_dur_- v_side_dur_ - quarter_dur_)*speed_,v_half_size_};
        }
        // left segment, bottom-half
        else
        {
            outPoint = {-h_half_size_,-(micros-quarter_dur_- 2*h_side_dur_ - v_side_dur_)*speed_ + v_half_size_};
        }

        if (rev)
            outPoint.y = -outPoint.y;

        return outPoint;
    };
};

class MinutesPathMapper
{
private:
    // 23.6639 mm spacing between numbers in X
    // 37.04 mm spacing in y
    static inline constexpr Point<uint8_t> gridSize_{10, 6};
    static constexpr Point<pos_t> gridOffset_{40.0, 28.0}; // tune up
    static constexpr Point<pos_t> gridSpacing_{23.6639, 37.04};
    static constexpr float revNum_ = 4.5;
    static constexpr uint32_t revDur_ = static_cast<uint32_t>(static_cast<uint32_t>(60000000) / revNum_);
    static constexpr int64_t zigDur_ = revDur_*2l;

    SquarePath minuteSquare_{gridSpacing_.x, gridSpacing_.y, revDur_}; // 3.5 loops per minute

    constexpr Point<pos_t> minuteCenter(const uint8_t minute) const
    {
        return {gridSpacing_.x * ((minute-1) % gridSize_.x) + gridOffset_.x,
           gridSpacing_.y * ((minute-1) / gridSize_.x) + gridOffset_.y};
    }

    Point<pos_t> carriageReturn(int32_t min, int32_t micros)
    {
        // our path from the start and end
        //             A-|
        // |-------------|
        // |-B

        Point<pos_t> start = minuteCenter(min-1);
        Point<pos_t> end = minuteCenter(min);

        start.x += gridSpacing_.x/2;
        end.x -= gridSpacing_.x/2;
        
        static constexpr pos_t vDist = gridSpacing_.y;
        static constexpr pos_t hDist = 10*gridSpacing_.x;
        static constexpr pos_t travelDist = hDist + vDist;
        static constexpr pos_t speed = travelDist/revDur_;

        static constexpr int64_t dur1 = vDist/2/speed;
        static constexpr int64_t dur2 = (hDist+vDist/2)/speed;

        if (micros < dur1)
            return {start.x, start.y+speed*micros};
        if (micros < dur2)
        {
            static constexpr pos_t yVal = vDist/2;
            return {start.x - (micros-dur1)*speed, start.y+yVal};
        }
        else
        {
            static constexpr pos_t yVal = vDist/2;
            return {end.x, start.y+yVal+(micros-dur2)*speed};
        }
    }

    Point<pos_t> zigZagReturn(int64_t micros)
    {
        // our path from the start and end
        // B-------------|
        // ************* |
        // |-------------|
        // | *************
        // |-------------A
        //
        //
        static constexpr Point<pos_t> start = {gridSpacing_.x*9+gridSpacing_.x/2+gridOffset_.x,
                                               gridSpacing_.y*5+gridOffset_.y};
        static constexpr Point<pos_t> end = {gridOffset_.x-gridSpacing_.x/2,
                                             gridOffset_.y};
        //static bool firstLoop = true;

        static constexpr pos_t vDist = start.y - end.y;
        static constexpr pos_t hDist = start.x - end.x;
        static constexpr pos_t travelDist = 3*hDist + vDist;
        static constexpr pos_t speed = travelDist/zigDur_;

        static constexpr int64_t dur1 = hDist/speed;
        static constexpr int64_t dur2 = (hDist+2.5*gridSpacing_.y)/speed;
        static constexpr int64_t dur3 = (2*hDist+2.5*gridSpacing_.y)/speed;
        static constexpr int64_t dur4 = (2*hDist+5*gridSpacing_.y)/speed;

        if (micros < dur1)
        {
            // from start, horizontally left across bottom of frame
            return {start.x - speed*micros, start.y};
        }
        if (micros < dur2)
        {
            // up left side from bottom left corner up to middle left side
            static constexpr int64_t timeDur = dur1;
            return {end.x, start.y-(micros-timeDur)*speed};
        }
        if (micros < dur3)
        {
            // horizontally right across middle of frame
            static constexpr int64_t timeDur = dur2;
            static constexpr pos_t yVal = start.y-2.5*gridSpacing_.y;
            return {end.x + (micros-timeDur)*speed, yVal};
        }
        if (micros < dur4)
        {
            // vertically up from the middle right to top right corner
            static constexpr int64_t timeDur = dur3;
            static constexpr pos_t ySpacing = start.y - 2.5*gridSpacing_.y;
            return {start.x, ySpacing - (micros-timeDur)*speed};
        }
        {
            // horizontally left across top
            static constexpr int64_t timeDur = dur4;
            static constexpr pos_t yVal = start.y - gridSpacing_.y*5;
            return {start.x - (micros - timeDur)*speed, yVal};
        }
    }

public:
    Point<pos_t> getPos(uint8_t minute, uint32_t micros)
    {
        if (minute == 0) minute = 60;

        if ((minute-1)%10 == 0 && minute != 1 && micros < revDur_)
            return carriageReturn(minute, micros);

        if (minute == 1 && micros < zigDur_)
            return zigZagReturn(micros);

        return minuteCenter(minute) + minuteSquare_.getPos(micros, minute%2 != 0);
    }
};

class HourPosMapper
{
private:
    // saving these to remember how we figured out timing
    // static constexpr unsigned long stepsPerHour = 167;
    // static constexpr unsigned long totalHSteps = 2000;

    // 12.5189 degrees between hours
    
    static constexpr pos_t degOffset = 5.5;
    static constexpr pos_t degPerHour = 12.5189;
    static constexpr uint32_t sweepDur = 10000000l;

    pos_t doSweep(const Time &t, uint32_t micros, uint8_t hour)
    {

        // half the time will be the full right-sweep, the rest of the time will be the 1st left-sweep and reurn-sweep
        static constexpr uint32_t quarterDur = sweepDur/4;
        if (hour == 6) // change from 12-1 skips 13
        {
            hour--;
        }
        else if (hour == 0)
        {
            hour = 13;
        }
        static constexpr float sweepSpeed = (13.0*degPerHour)/quarterDur;

        const uint32_t leftInterval = degPerHour*hour/sweepSpeed;

        // go from our current pos, all the way to the left
        if (micros <  leftInterval)
            return hour*degPerHour - micros*sweepSpeed;

        // go all the way to the right
        if (micros < leftInterval+quarterDur)
            return (micros-leftInterval)*sweepSpeed;

        // all way left
        if (micros < leftInterval+2*quarterDur)
            return 13*degPerHour-(micros-leftInterval-quarterDur)*sweepSpeed;

        // all way right
        if (micros < leftInterval+3*quarterDur)
            return (micros-leftInterval-2*quarterDur)*sweepSpeed;

        // go to the same hour position
        return (13*degPerHour)-(micros-leftInterval-3*quarterDur)*sweepSpeed;
    }

    pos_t getHourPos(const Time &t, uint8_t hour)
    {
        return hour*degPerHour + t.Min*degPerHour/60 + t.Sec*degPerHour/3600 + degOffset;
    }
public:
    pos_t getPos(const Time &t, uint32_t micros)
    {
        uint8_t hour = t.Hour > 12 ? t.Hour - 12 : t.Hour; // remove 24 hour timekeeping
        if (hour < 8) { hour += 12; } // handle running from 8-8 range
        if (hour > 12) { hour += 1; } // skip the 13th hour
        hour -= 8; // index everything back so 8 is 0 index
        
        // skipping weird hour transitions
        if (hour == 6 && t.Min == 0) { hour -= 1; }
        if (hour == 0 && t.Min == 0) { hour = 13; }

        static constexpr uint32_t sweepDurZero = static_cast<uint64_t>(sweepDur) * 3 / 4; // skip 1/4 of the sweep
        static constexpr uint32_t sweepDurSix = sweepDur - (sweepDur / (4*13)); // skip last hour so it ends @ 1

        // sweep condition
        if (t.Min == 1 && 
            ((micros < sweepDur && hour != 0 && hour != 6) || 
             (micros < sweepDurZero && hour == 0) ||
             (micros < sweepDurSix && hour == 6)
             ))
            return degOffset + doSweep(t,micros,hour);

        return getHourPos(t,hour);
    };
};
