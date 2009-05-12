#include "plugin_enumerator.h"
#include "../devices/data_source.h"
#include "../devices/aym/aym.h"

#include <tools.h>

#include <sound_attrs.h>
#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cassert>

namespace
{
  using namespace ZXTune;

  const String TEXT_PSG_INFO("PSG modules support");
  const String TEXT_PSG_VERSION("0.1");

  const uint8_t PSG_SIGNATURE[] = {'P', 'S', 'G'};
  const uint8_t PSG_MARKER = 0x1a;

  const uint8_t INT_BEGIN = 0xff;
  const uint8_t INT_SKIP = 0xfe;
  const uint8_t MUS_END = 0xfd;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct PSGHeader
  {
    uint8_t Sign[3];
    uint8_t Marker;
    uint8_t Version;
    uint8_t Interrupt;
    uint8_t Padding[10];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(PSGHeader) == 16);

  void Information(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_AYM;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_PSG_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_PSG_VERSION));
  }

  bool Checking(const String& /*filename*/, const Dump& data)
  {
    if (data.size() <= sizeof(PSGHeader))
    {
      return false;
    }
    const PSGHeader* header(safe_ptr_cast<const PSGHeader*>(&data[0]));
    return (0 == std::memcmp(header->Sign, PSG_SIGNATURE, sizeof(PSG_SIGNATURE)) && PSG_MARKER == header->Marker);
  }

  class PlayerImpl : public ModulePlayer
  {
  public:
    PlayerImpl(const String& filename, const Dump& data)
      : Device(AYM::CreateChip()), CurrentState(MODULE_STOPPED), Filename(filename), TickCount(), Position()
    {
      assert(data.size() > sizeof(PSGHeader));
      //const PSGHeader* header(safe_ptr_cast<const PSGHeader*>(&data[0]));
      //workaround for some emulators
      std::size_t offset = data[4] == INT_BEGIN ? 4 : sizeof(PSGHeader);
      std::size_t size = data.size() - offset;
      Dump::const_iterator bdata = data.begin() + offset;
      if (INT_BEGIN != *bdata)
      {
        throw 1;//TODO
      }
      AYM::DataChunk dummy;
      AYM::DataChunk* chunk = &dummy;
      while (size)
      {
        uint8_t reg = *bdata;
        ++bdata;
        --size;
        if (INT_BEGIN == reg)
        {
          Storage.push_back(dummy);
          chunk = &Storage.back();
        }
        else if (INT_SKIP == reg)
        {
          if (size < 1)
          {
            throw 1;//TODO
          }
          std::size_t count = 4 * *bdata;
          while (count--)
          {
            //empty chunk
            Storage.push_back(dummy);
          }
          chunk = &Storage.back();
          ++bdata;
          --size;
        }
        else if (MUS_END == reg)
        {
          break;
        }
        else if (reg <= 15) //register
        {
          if (size < 1)
          {
            throw 1;//TODO
          }
          chunk->Data[reg] = *bdata;
          chunk->Mask |= 1 << reg;
          ++bdata;
          --size;
        }
      }
    }

    virtual void GetInfo(Info& info) const
    {
      info.Capabilities = CAP_AYM;
      info.Properties.clear();
    }

    /// Retrieving information about loaded module
    virtual void GetModuleInfo(Module::Information& info) const
    {
      info.Capabilities = 0;
      info.Properties.clear();
      info.Loop = 0;
      info.Statistic.Channels = 3;
      info.Statistic.Note = info.Statistic.Frame = static_cast<uint32_t>(Storage.size());
      info.Statistic.Pattern = 1;
      info.Statistic.Position = 1;
      info.Statistic.Tempo = 1;
    }

    /// Retrieving current state of loaded module
    virtual State GetModuleState(std::size_t& timeState, Module::Tracking& trackState) const
    {
      timeState = static_cast<uint32_t>(Position);
      trackState.Channels = 3;
      trackState.Note = Position;
      trackState.Pattern = 0;
      trackState.Position = 0;
      trackState.Tempo = 1;
      return CurrentState;
    }

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::Volume& volState, Sound::Analyze::Spectrum& spectrumState) const
    {
      assert(Device.get());
      Device->GetState(volState, spectrumState);
      return CurrentState;
    }

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
    {
      if (Position >= Storage.size())
      {
        if (params.Flags & Sound::MOD_LOOP)
        {
          Position = 0;
        }
        else
        {
          receiver.Flush();
          return CurrentState = MODULE_STOPPED;
        }
      }
      assert(Device.get());
      AYM::DataChunk& data(Storage[Position++]);
      data.Tick = (TickCount += uint64_t(params.ClockFreq) * params.FrameDuration / 1000);
      Device->RenderData(params, data, receiver);
      return CurrentState = MODULE_PLAYING;
    }

    /// Controlling
    virtual State Reset()
    {
      assert(Device.get());
      Position = 0;
      TickCount = 0;
      Device->Reset();
      return CurrentState = MODULE_STOPPED;
    }
    virtual State SetPosition(const uint32_t& frame)
    {
      if (frame >= Storage.size())
      {
        return Reset();
      }
      assert(Device.get());
      Position = frame;
      TickCount = 0;
      Device->Reset();
      return CurrentState;
    }

  private:
    AYM::Chip::Ptr Device;
    State CurrentState;
    String Filename;
    uint64_t TickCount;
    std::size_t Position;
    std::vector<AYM::DataChunk> Storage;
  };


  ModulePlayer::Ptr Creating(const String& filename, const Dump& data)
  {
    assert(Checking(filename, data) || !"Attempt to create player on invalid data");
    return ModulePlayer::Ptr(new PlayerImpl(filename, data));
  }

  PluginAutoRegistrator psgReg(Checking, Creating, Information);
}
