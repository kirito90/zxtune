/**
*
* @file      sound/mixer.h
* @brief     Defenition of mixing-related functionality
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __SOUND_MIXER_H_DEFINED__
#define __SOUND_MIXER_H_DEFINED__

//common includes
#include <error.h>
//library includes
#include <sound/receiver.h>

namespace ZXTune
{
  namespace Sound
  {
    //! @brief %Mixer interface
    class Mixer : public DataTransceiver<std::vector<Sample>, MultiSample>
    {
    public:
      //! @brief Pointer type
      typedef boost::shared_ptr<Mixer> Ptr;

      //! @brief Setting up the mixing matrix
      //! @param data Mixing matrix
      //! @return Error() in case of success
      virtual Error SetMatrix(const std::vector<MultiGain>& data) = 0;
    };

    //! @brief Creating mixer instance
    //! @param channels Input channels count
    //! @param result Reference to result value
    //! @return Error() in case of success
    //! @note For any of the created mixers, SetMatrix parameter size should be equal to channels parameter while creating
    Error CreateMixer(uint_t channels, Mixer::Ptr& result);
  }
}

#endif //__SOUND_MIXER_H_DEFINED__
