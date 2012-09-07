/**
*
* @file     time_oscillator.h
* @brief    Time tools interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __TIME_OSCILLATOR_H_DEFINED__
#define __TIME_OSCILLATOR_H_DEFINED__

//common includes
#include <time_stamp.h>
#include <tools.h>
//std includes
#include <cassert>

namespace Time
{
  template<class T, class TimeStamp>
  class Oscillator
  {
  public:
    Oscillator()
      : LastFreqChangeTime()
      , LastFreqChangeTick()
      , Frequency()
      , CurTick()
      , ScaleToTime(0, TimeStamp::PER_SECOND)
      , ScaleToTick(TimeStamp::PER_SECOND, 0)
      , CurTimeCache()
    {
    }

    void Reset()
    {
      LastFreqChangeTime = CurTimeCache = 0;
      Frequency = 0;
      ScaleToTime = ScaleFunctor<typename TimeStamp::ValueType>(0, TimeStamp::PER_SECOND);
      ScaleToTick = ScaleFunctor<typename TimeStamp::ValueType>(TimeStamp::PER_SECOND, 0);
      LastFreqChangeTick = CurTick = 0;
    }

    void SetFrequency(T freq)
    {
      if (freq != Frequency)
      {
        LastFreqChangeTime = GetCurrentTime().Get();
        LastFreqChangeTick = GetCurrentTick();
        Frequency = freq;
        ScaleToTime = ScaleFunctor<typename TimeStamp::ValueType>(Frequency, TimeStamp::PER_SECOND);
        ScaleToTick = ScaleFunctor<typename TimeStamp::ValueType>(TimeStamp::PER_SECOND, Frequency);
      }
    }

    void AdvanceTick(T delta)
    {
      CurTick += delta;
      CurTimeCache = 0;
    }

    T GetCurrentTick() const
    {
      return CurTick;
    }

    TimeStamp GetCurrentTime() const
    {
      if (!CurTimeCache && CurTick)
      {
        const T relTick = CurTick - LastFreqChangeTick;
        const typename TimeStamp::ValueType relTime = ScaleToTime(relTick);
        CurTimeCache = LastFreqChangeTime + relTime;
      }
      return TimeStamp(CurTimeCache);
    }

    T GetTickAtTime(const TimeStamp& time) const
    {
      const typename TimeStamp::ValueType relTime = time.Get() - LastFreqChangeTime;
      const T relTick = ScaleToTick(relTime);
      return LastFreqChangeTick + relTick;
    }
  private:
    typename TimeStamp::ValueType LastFreqChangeTime;
    T LastFreqChangeTick;
    T Frequency;
    T CurTick;
    ScaleFunctor<typename TimeStamp::ValueType> ScaleToTime;
    ScaleFunctor<typename TimeStamp::ValueType> ScaleToTick;
    mutable typename TimeStamp::ValueType CurTimeCache;
  };

  typedef Oscillator<uint64_t, Microseconds> MicrosecOscillator;
  typedef Oscillator<uint64_t, Nanoseconds> NanosecOscillator;
}

#endif //__TIME_OSCILLATOR_H_DEFINED__