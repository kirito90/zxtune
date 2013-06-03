/**
*
* @file      sound/mixer.h
* @brief     Defenition of mixing-related functionality
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_MATRIC_MIXER_H_DEFINED
#define SOUND_MATRIC_MIXER_H_DEFINED

//library includes
#include <sound/gain.h>
#include <sound/mixer.h>

namespace Sound
{
  template<unsigned Channels>
  class FixedChannelsMatrixStreamMixer : public FixedChannelsStreamMixer<Channels>
  {
  public:
    typedef boost::shared_ptr<FixedChannelsMatrixStreamMixer> Ptr;
    typedef boost::array< ::Sound::Gain, Channels> Matrix;

    virtual void SetMatrix(const Matrix& data) = 0;

    static Ptr Create();
  };

  template<unsigned Channels>
  class FixedChannelsMatrixMixer : public FixedChannelsMixer<Channels>
  {
  public:
    typedef boost::shared_ptr<FixedChannelsMatrixMixer> Ptr;
    typedef boost::array< ::Sound::Gain, Channels> Matrix;

    virtual void SetMatrix(const Matrix& data) = 0;

    static Ptr Create();
  };
}

#endif //SOUND_MATRIC_MIXER_H_DEFINED
