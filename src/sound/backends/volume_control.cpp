/*
Abstract:
  Volume control delegate implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "volume_control.h"
//library includes
#include <l10n/api.h>
#include <sound/error_codes.h>
//boost includes
#include <boost/ref.hpp>
#include <boost/make_shared.hpp>

#define FILE_TAG B368C82C

namespace
{
  using namespace ZXTune::Sound;

  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound");

  class VolumeControlDelegate : public VolumeControl
  {
  public:
    explicit VolumeControlDelegate(const VolumeControl::Ptr& delegate)
      : Delegate(delegate)
    {
    }

    virtual Error GetVolume(MultiGain& volume) const
    {
      if (VolumeControl::Ptr delegate = VolumeControl::Ptr(Delegate))
      {
        return delegate->GetVolume(volume);
      }
      return Error(THIS_LINE, BACKEND_CONTROL_ERROR, translate("Failed to get volume in invalid state."));
    }

    virtual Error SetVolume(const MultiGain& volume)
    {
      if (VolumeControl::Ptr delegate = VolumeControl::Ptr(Delegate))
      {
        return delegate->SetVolume(volume);
      }
      return Error(THIS_LINE, BACKEND_CONTROL_ERROR, translate("Failed to set volume in invalid state."));
    }
  private:
    const VolumeControl::Ptr& Delegate;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    VolumeControl::Ptr CreateVolumeControlDelegate(const VolumeControl::Ptr& delegate)
    {
      return boost::make_shared<VolumeControlDelegate>(boost::cref(delegate));
    }
  }
}
