/*
Abstract:
  SDL backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifdef SDL_SUPPORT

#include "backend_impl.h"
#include "backend_wrapper.h"
#include "enumerator.h"

#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
#include <sound/backends_parameters.h>
#include <sound/error_codes.h>

#include <boost/noncopyable.hpp>

//platform-dependent
#include <SDL/SDL.h>

#include <sound/text/backends.h>
#include <sound/text/sound.h>

#define FILE_TAG 608CF986

namespace
{
  using namespace ZXTune::Sound;

  const std::string THIS_MODULE("SDL");

  const Char BACKEND_ID[] = {'s', 'd', 'l', 0};
  const String BACKEND_VERSION(FromStdString("$Rev$"));

  const uint_t BUFFERS_MIN = 2;
  const uint_t BUFFERS_MAX = 10;

  // backend description
  const BackendInformation BACKEND_INFO =
  {
    BACKEND_ID,
    Text::SDL_BACKEND_DESCRIPTION,
    BACKEND_VERSION,
  };

  void CheckCall(bool ok, Error::LocationRef loc)
  {
    if (!ok)
    {
      if (const char* txt = ::SDL_GetError())
      {
        throw MakeFormattedError(loc, BACKEND_PLATFORM_ERROR,
          Text::SOUND_ERROR_SDL_BACKEND_ERROR, FromStdString(txt));
      }
      throw Error(loc, BACKEND_PLATFORM_ERROR, Text::SOUND_ERROR_SDL_BACKEND_UNKNOWN_ERROR);
    }
  }

  class SDLBackend : public BackendImpl, private boost::noncopyable
  {
  public:
    SDLBackend()
      : WasInitialized(::SDL_WasInit(SDL_INIT_EVERYTHING))
      , BuffersCount(Parameters::ZXTune::Sound::Backends::SDL::BUFFERS_DEFAULT)
      , Buffers(BuffersCount)
      , FillIter(&Buffers.front(), &Buffers.back() + 1)
      , PlayIter(FillIter)
    {
      if (0 == WasInitialized)
      {
        Log::Debug(THIS_MODULE, "Initializing");
        CheckCall(::SDL_Init(SDL_INIT_AUDIO) == 0, THIS_LINE);
      }
      else if (0 == (WasInitialized & SDL_INIT_AUDIO))
      {
        Log::Debug(THIS_MODULE, "Initializing sound subsystem");
        CheckCall(::SDL_InitSubSystem(SDL_INIT_AUDIO) == 0, THIS_LINE);
      }
    }

    virtual ~SDLBackend()
    {
      if (0 == WasInitialized)
      {
        Log::Debug(THIS_MODULE, "Shutting down");
        ::SDL_Quit();
      }
      else if (0 == (WasInitialized & SDL_INIT_AUDIO))
      {
        Log::Debug(THIS_MODULE, "Shutting down sound subsystem");
        ::SDL_QuitSubSystem(SDL_INIT_AUDIO);
      }
    }

    virtual void GetInformation(BackendInformation& info) const
    {
      info = BACKEND_INFO;
    }

    virtual VolumeControl::Ptr GetVolumeControl() const
    {
      return VolumeControl::Ptr();
    }

    virtual void OnStartup()
    {
      Locker lock(BackendMutex);
      DoStartup();
    }

    virtual void OnShutdown()
    {
      Locker lock(BackendMutex);
      ::SDL_CloseAudio();
    }

    virtual void OnPause()
    {
      Locker lock(BackendMutex);
      ::SDL_PauseAudio(1);
    }

    virtual void OnResume()
    {
      Locker lock(BackendMutex);
      ::SDL_PauseAudio(0);
    }

    virtual void OnParametersChanged(const Parameters::Map& updates)
    {
      const Parameters::IntType* buffers =
        Parameters::FindByName<Parameters::IntType>(updates, Parameters::ZXTune::Sound::Backends::SDL::BUFFERS);
      const Parameters::IntType* freq =
        Parameters::FindByName<Parameters::IntType>(updates, Parameters::ZXTune::Sound::FREQUENCY);
      if (buffers || freq)
      {
        Locker lock(BackendMutex);
        const bool needStartup = SDL_AUDIO_STOPPED != ::SDL_GetAudioStatus();
        if (needStartup)
        {
          ::SDL_CloseAudio();
        }
        if (buffers)
        {
          if (!in_range<Parameters::IntType>(*buffers, BUFFERS_MIN, BUFFERS_MAX))
          {
            throw MakeFormattedError(THIS_LINE, BACKEND_INVALID_PARAMETER,
              Text::SOUND_ERROR_SDL_BACKEND_INVALID_BUFFERS, static_cast<int_t>(*buffers), BUFFERS_MIN, BUFFERS_MAX);
          }
          BuffersCount = *buffers;
        }
        if (needStartup)
        {
          DoStartup();
        }
      }
    }

    virtual void OnBufferReady(std::vector<MultiSample>& buffer)
    {
      Locker lock(BackendMutex);
      boost::unique_lock<boost::mutex> locker(BufferMutex);
      while (FillIter->ToPlay)
      {
        PlayedEvent.wait(locker);
      }
      FillIter->Data.swap(buffer);
      FillIter->ToPlay = FillIter->Data.size() * sizeof(FillIter->Data.front());
      ++FillIter;
      FilledEvent.notify_one();
    }
  private:
    void DoStartup()
    {
      Log::Debug(THIS_MODULE, "Starting playback");

      SDL_AudioSpec format;
      format.format = -1;
      switch (sizeof(Sample))
      {
      case 1:
        format.format = AUDIO_S8;
        break;
      case 2:
        format.format = isLE() ? AUDIO_S16LSB : AUDIO_S16MSB;
        break;
      default:
        assert(!"Invalid format");
      }

      format.freq = static_cast<int>(RenderingParameters.SoundFreq);
      format.channels = static_cast< ::Uint8>(OUTPUT_CHANNELS);
      format.samples = BuffersCount * RenderingParameters.SamplesPerFrame();
      //fix if size is not power of 2
      if (0 != (format.samples & (format.samples - 1)))
      {
        unsigned msk = 1;
        while (format.samples > msk)
        {
          msk <<= 1;
        }
        format.samples = msk;
      }
      format.callback = OnBuffer;
      format.userdata = this;
      Buffers.resize(BuffersCount);
      FillIter = cycled_iterator<Buffer*>(&Buffers.front(), &Buffers.back() + 1);
      PlayIter = FillIter;
      CheckCall(::SDL_OpenAudio(&format, 0) >= 0, THIS_LINE);
      ::SDL_PauseAudio(0);
    }

    static void OnBuffer(void* param, ::Uint8* stream, int len)
    {
      SDLBackend* const self = static_cast<SDLBackend*>(param);
      boost::unique_lock<boost::mutex> locker(self->BufferMutex);
      while (len)
      {
        //wait for data
        while (!self->PlayIter->ToPlay)
        {
          self->FilledEvent.wait(locker);
        }
        const uint_t inBuffer = self->PlayIter->Data.size() * sizeof(self->PlayIter->Data.front());
        const uint_t toCopy = std::min<uint_t>(len, self->PlayIter->ToPlay);
        const uint8_t* const src = safe_ptr_cast<const uint8_t*>(&self->PlayIter->Data.front());
        std::memcpy(stream, src + (inBuffer - self->PlayIter->ToPlay), toCopy);
        self->PlayIter->ToPlay -= toCopy;
        stream += toCopy;
        len -= toCopy;
        if (!self->PlayIter->ToPlay)
        {
          //buffer is played
          ++self->PlayIter;
          self->PlayedEvent.notify_one();
        }
      }
    }
  private:
    const ::Uint32 WasInitialized;
    boost::mutex BufferMutex;
    boost::condition_variable FilledEvent, PlayedEvent;
    struct Buffer
    {
      Buffer() : ToPlay()
      {
      }
      uint_t ToPlay;
      std::vector<MultiSample> Data;
    };
    uint_t BuffersCount;
    std::vector<Buffer> Buffers;
    cycled_iterator<Buffer*> FillIter, PlayIter;
  };

  Backend::Ptr SDLBackendCreator(const Parameters::Map& params)
  {
    return Backend::Ptr(new SafeBackendWrapper<SDLBackend>(params));
  }
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterSDLBackend(BackendsEnumerator& enumerator)
    {
      enumerator.RegisterBackend(BACKEND_INFO, &SDLBackendCreator);
    }
  }
}

#else //not supported

namespace ZXTune
{
  namespace Sound
  {
    void RegisterSDLBackend(class BackendsEnumerator& /*enumerator*/)
    {
      //do nothing
    }
  }
}

#endif
