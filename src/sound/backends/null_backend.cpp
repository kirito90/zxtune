/*
Abstract:
  Null backend implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "backend_impl.h"
#include "backend_wrapper.h"
#include "enumerator.h"

#include <sound/error_codes.h>

#include <boost/noncopyable.hpp>

#include <text/sound.h>
#include <text/backends.h>

#define FILE_TAG 9A6FD87F

namespace
{
  using namespace ZXTune::Sound;

  const Char BACKEND_ID[] = {'n', 'u', 'l', 'l', 0};
  const String BACKEND_VERSION(FromChar("$Rev$"));
  
  static const Backend::Info BACKEND_INFO =
  {
    BACKEND_ID,
    BACKEND_VERSION,
    TEXT_NULL_BACKEND_DESCRIPTION
  };
  
  class NullBackend : public BackendImpl, private boost::noncopyable
  {
  public:
    NullBackend()
    {
    }

    virtual void GetInfo(Info& info) const
    {
      info = BACKEND_INFO;
    }

    virtual Error GetVolume(MultiGain& /*volume*/) const
    {
      return Error(THIS_LINE, BACKEND_UNSUPPORTED_FUNC, TEXT_SOUND_ERROR_BACKEND_UNSUPPORTED_VOLUME);
    }
    
    virtual Error SetVolume(const MultiGain& /*volume*/)
    {
      return Error(THIS_LINE, BACKEND_UNSUPPORTED_FUNC, TEXT_SOUND_ERROR_BACKEND_UNSUPPORTED_VOLUME);
    }

    virtual void OnStartup()
    {
    }
    
    virtual void OnShutdown()
    {
    }
    
    virtual void OnPause()
    {
    }
    
    virtual void OnResume()
    {
    }
    
    virtual void OnParametersChanged(const ParametersMap& /*updates*/)
    {
    }
    
    virtual void OnBufferReady(std::vector<MultiSample>& /*buffer*/)
    {
    }
  };

  Backend::Ptr NullBackendCreator()
  {
    return Backend::Ptr(new SafeBackendWrapper<NullBackend>());
  }
}

namespace ZXTune
{
  namespace Sound
  {
    void RegisterNullBackend(BackendsEnumerator& enumerator)
    {
      enumerator.RegisterBackend(BACKEND_INFO, NullBackendCreator);
    }
  }
}
