/**
*
* @file     core/module_player.h
* @brief    Modules player interface definition
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#ifndef __CORE_MODULE_PLAYER_H_DEFINED__
#define __CORE_MODULE_PLAYER_H_DEFINED__

//for typedef'ed Parameters::Map
#include <parameters.h>

//for Module::Analyze::ChannelsState
#include <core/module_types.h>
//for Sound::MultichannelReceiver
#include <sound/receiver.h>

//forward declarations
class Error;

namespace ZXTune
{
  //forward declarations
  namespace Sound
  {
    struct RenderParameters;
  }
  
  namespace Module
  {
    class Holder;

    //! @brief %Module player interface
    class Player
    {
    public:
      //! @brief Generic pointer type
      typedef boost::shared_ptr<Player> Ptr;
      //! @brief Read-only pointer type
      typedef boost::shared_ptr<const Player> ConstPtr;
      //! @brief Read-only weak pointer type
      typedef boost::weak_ptr<const Player> ConstWeakPtr;

      virtual ~Player() {}

      //! @brief Retrieving module holder
      virtual const Holder& GetModule() const = 0;

      //! @brief Playing state
      enum PlaybackState
      {
        MODULE_PLAYING,
        MODULE_STOPPED
      };

      //! @brief Getting current playback state of loaded module
      //! @param timeState Reference to store frame number
      //! @param trackState Reference to store tracking position
      //! @param analyzeState Reference to store analyze result
      //! @return Error() in case of success
      virtual Error GetPlaybackState(unsigned& timeState, Tracking& trackState,
        Analyze::ChannelsState& analyzeState) const = 0;

      //! @brief Rendering single frame and modifying internal state
      //! @param params %Sound rendering-related parameters
      //! @param state Playback state player transitioned to
      //! @param receiver Output stream receiver
      //! @return Error() in case of success
      virtual Error RenderFrame(const Sound::RenderParameters& params, PlaybackState& state,
        Sound::MultichannelReceiver& receiver) = 0;

      //! @brief Performing reset to initial state
      //! @return Error() in case of success
      virtual Error Reset() = 0;

      //! @brief Seeking
      //! @param frame Number of specified frame
      //! @return Error() in case of success
      //! @note Seeking out of range is safe, but state will be MODULE_PLAYING untill next RenderFrame call happends.
      //! @note It produces only the flush
      virtual Error SetPosition(unsigned frame) = 0;

      //! @brief Changing runtime parameters
      //! @param params Map with parameters
      //! @return Error() in case of success
      virtual Error SetParameters(const Parameters::Map& params) = 0;
    };
  }
}

#endif //__CORE_MODULE_PLAYER_H_DEFINED__
