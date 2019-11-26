/**
* 
* @file
*
* @brief  ProDigiTracker support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/chiptune/digital/prodigitracker.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <indices.h>
#include <make_ptr.h>
#include <range_checker.h>
//library includes
#include <binary/data_adapter.h>
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <strings/optimize.h>
//std includes
#include <array>
#include <cstring>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace ProDigiTracker
  {
    const Debug::Stream Dbg("Formats::Chiptune::ProDigiTracker");

    const uint_t ORNAMENTS_COUNT = 11;
    const uint_t SAMPLES_COUNT = 16;
    const uint_t POSITIONS_COUNT = 240;
    const uint_t PAGES_COUNT = 5;
    const uint_t CHANNELS_COUNT = 4;
    const uint_t PATTERN_SIZE = 64;
    const uint_t PATTERNS_COUNT = 32;
    const std::size_t ZX_PAGE_SIZE = 0x4000;
    const std::size_t PAGES_START = 0xc000;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    typedef std::array<int8_t, 16> RawOrnament;

    PACK_PRE struct RawOrnamentLoop
    {
      uint8_t Begin;
      uint8_t End;
    } PACK_POST;

    PACK_PRE struct RawSample
    {
      std::array<char, 8> Name;
      uint16_t Start;
      uint16_t Size;
      uint16_t Loop;
      uint8_t Page;
      uint8_t Padding;
    } PACK_POST;

    const uint_t NOTE_EMPTY = 0;

    enum
    {
      CMD_SPECIAL = 0, //see parameter
      CMD_SPEED = 1,   //parameter- speed
      CMD_1 = 2,       //????
      CMD_2 = 3,       //????

      COMMAND_NOORNAMENT = 15,
      COMMAND_BLOCKCHANNEL = 14,
      COMMAND_ENDPATTERN = 13,
      COMMAND_CONTSAMPLE = 12,
      COMMAND_NONE = 0
      //else ornament + 1
    };

    PACK_PRE struct RawNote
    {
      //ccnnnnnn
      //sssspppp
      //c- command
      //n- note
      //p- parameter
      //s- sample
      uint_t GetNote() const
      {
        return NoteComm & 63;
      }

      uint_t GetCommand() const
      {
        return (NoteComm & 192) >> 6;
      }

      uint_t GetParameter() const
      {
        return ParamSample & 15;
      }

      uint_t GetSample() const
      {
        return (ParamSample & 240) >> 4;
      }

      bool IsEnd() const
      {
        return GetCommand() == CMD_SPECIAL && GetParameter() == COMMAND_ENDPATTERN;
      }

      uint8_t NoteComm;
      uint8_t ParamSample;
    } PACK_POST;

    typedef std::array<RawNote, CHANNELS_COUNT> RawLine;

    typedef std::array<RawLine, PATTERN_SIZE> RawPattern;

    PACK_PRE struct RawHeader
    {
      std::array<RawOrnament, ORNAMENTS_COUNT> Ornaments;
      std::array<RawOrnamentLoop, ORNAMENTS_COUNT> OrnLoops;
      uint8_t Padding1[6];
      std::array<char, 32> Title;
      uint8_t Tempo;
      uint8_t Start;
      uint8_t Loop;
      uint8_t Length;
      uint8_t Padding2[16];
      std::array<RawSample, SAMPLES_COUNT> Samples;
      std::array<uint8_t, POSITIONS_COUNT> Positions;
      uint16_t LastDatas[PAGES_COUNT];
      uint8_t FreeRAM;
      uint8_t Padding3[5];
      std::array<RawPattern, PATTERNS_COUNT> Patterns;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    static_assert(sizeof(RawOrnament) == 16, "Invalid layout");
    static_assert(sizeof(RawOrnamentLoop) == 2, "Invalid layout");
    static_assert(sizeof(RawSample) == 16, "Invalid layout");
    static_assert(sizeof(RawNote) == 2, "Invalid layout");
    static_assert(sizeof(RawPattern) == 512, "Invalid layout");
    static_assert(sizeof(RawHeader) == 0x4300, "Invalid layout");

    const std::size_t MODULE_SIZE = sizeof(RawHeader) + PAGES_COUNT * ZX_PAGE_SIZE;

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }
      void SetInitialTempo(uint_t /*tempo*/) override {}
      void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::DataView /*content*/) override {}
      void SetOrnament(uint_t /*index*/, Ornament /*ornament*/) override {}
      void SetPositions(Positions /*positions*/) override {}
      PatternBuilder& StartPattern(uint_t /*index*/) override
      {
        return GetStubPatternBuilder();
      }
      void StartChannel(uint_t /*index*/) override {}
      void SetRest() override {}
      void SetNote(uint_t /*note*/) override {}
      void SetSample(uint_t /*sample*/) override {}
      void SetOrnament(uint_t /*ornament*/) override {}
    };

    class StatisticCollectionBuilder : public Builder
    {
    public:
      explicit StatisticCollectionBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, PATTERNS_COUNT - 1)
        , UsedSamples(0, SAMPLES_COUNT - 1)
        , UsedOrnaments(0, ORNAMENTS_COUNT - 1)
      {
      }

      MetaBuilder& GetMetaBuilder() override
      {
        return Delegate.GetMetaBuilder();
      }

      void SetInitialTempo(uint_t tempo) override
      {
        return Delegate.SetInitialTempo(tempo);
      }

      void SetSample(uint_t index, std::size_t loop, Binary::DataView data) override
      {
        return Delegate.SetSample(index, loop, data);
      }

      void SetOrnament(uint_t index, Ornament ornament) override
      {
        return Delegate.SetOrnament(index, std::move(ornament));
      }

      void SetPositions(Positions positions) override
      {
        UsedPatterns.Assign(positions.Lines.begin(), positions.Lines.end());
        Require(!UsedPatterns.Empty());
        return Delegate.SetPositions(std::move(positions));
      }

      PatternBuilder& StartPattern(uint_t index) override
      {
        return Delegate.StartPattern(index);
      }

      void StartChannel(uint_t index) override
      {
        return Delegate.StartChannel(index);
      }

      void SetRest() override
      {
        return Delegate.SetRest();
      }

      void SetNote(uint_t note) override
      {
        return Delegate.SetNote(note);
      }

      void SetSample(uint_t sample) override
      {
        UsedSamples.Insert(sample);
        return Delegate.SetSample(sample);
      }

      void SetOrnament(uint_t ornament) override
      {
        UsedOrnaments.Insert(ornament);
        return Delegate.SetOrnament(ornament);
      }

      const Indices& GetUsedPatterns() const
      {
        return UsedPatterns;
      }

      const Indices& GetUsedSamples() const
      {
        Require(!UsedSamples.Empty());
        return UsedSamples;
      }

      const Indices& GetUsedOrnaments() const
      {
        return UsedOrnaments;
      }
    private:
      Builder& Delegate;
      Indices UsedPatterns;
      Indices UsedSamples;
      Indices UsedOrnaments;
    };

    class Format
    {
    public:
      explicit Format(const Binary::Container& rawData)
        : RawData(rawData)
        , Source(*static_cast<const RawHeader*>(RawData.Start()))
        , FixedRanges(RangeChecker::Create(RawData.Size()))
      {
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = target.GetMetaBuilder();
        meta.SetTitle(Strings::OptimizeAscii(Source.Title));
        meta.SetProgram(Text::PRODIGITRACKER_DECODER_DESCRIPTION);
        Strings::Array names(Source.Samples.size());
        for (uint_t idx = 0; idx != Source.Samples.size(); ++idx)
        {
          names[idx] = Strings::OptimizeAscii(Source.Samples[idx].Name);
        }
        meta.SetStrings(std::move(names));
      }

      void ParsePositions(Builder& target) const
      {
        Positions positions;
        positions.Loop = Source.Loop;
        positions.Lines.assign(Source.Positions.begin(), Source.Positions.begin() + Source.Length);
        Dbg("Positions: %1%, loop to %2%", positions.GetSize(), positions.GetLoop());
        target.SetPositions(std::move(positions));
      }

      void ParsePatterns(const Indices& pats, Builder& target) const
      {
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern %1%", patIndex);
          ParsePattern(patIndex, target);
        }
      }

      void ParseSamples(const Indices& sams, Builder& target) const
      {
        const uint8_t* const moduleStart = safe_ptr_cast<const uint8_t*>(&Source);
        const uint8_t* const samplesStart = safe_ptr_cast<const uint8_t*>(&Source + 1);
        for (Indices::Iterator it = sams.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          const auto& descr = Source.Samples[samIdx];
          const std::size_t start = fromLE(descr.Start);
          const std::size_t loop = fromLE(descr.Loop);
          std::size_t size = fromLE(descr.Size);
          if (descr.Page < PAGES_COUNT && start >= PAGES_START && size != 0)
          {
            Dbg("Sample %1%: start=#%2$04x loop=#%3$04x size=#%4$04x",
              samIdx, start, loop, size);
            const uint8_t* const sampleData = samplesStart + ZX_PAGE_SIZE * GetPageOrder(descr.Page) + (start - PAGES_START);
            while (--size && sampleData[size] == 0) {};
            ++size;
            if (const auto content = RawData.GetSubcontainer(sampleData - moduleStart, size))
            {
              target.SetSample(samIdx, loop >= start ? loop - start : size, *content);
              continue;
            }
          }
          Dbg(" Stub sample %1%", samIdx);
          const uint8_t dummy = 128;
          target.SetSample(samIdx, 0, Binary::DataAdapter(&dummy, sizeof(dummy)));
        }
      }
      
      void ParseOrnaments(const Indices& orns, Builder& target) const
      {
        for (Indices::Iterator it = orns.Items(); it; ++it)
        {
          if (const uint_t ornIdx = *it)
          {
            const auto& orn = Source.Ornaments[ornIdx - 1];
            const auto& loop = Source.OrnLoops[ornIdx - 1];
            Ornament res;
            res.Loop = loop.Begin;
            res.Lines.resize(loop.End);
            std::transform(orn.begin(), orn.begin() + loop.End, res.Lines.begin(), std::bind2nd(std::divides<int8_t>(), 2));
            target.SetOrnament(ornIdx, std::move(res));
          }
          else
          {
            target.SetOrnament(ornIdx, Ornament());
          }
        }
      }

      RangeChecker::Range GetFixedArea() const
      {
        return FixedRanges->GetAffectedRange();
      }
    private:
      static uint_t GetPageOrder(uint_t page)
      {
        //1,3,4,6,7
        switch (page)
        {
        case 1:
          return 0;
        case 3:
          return 1;
        case 4:
          return 2;
        case 6:
          return 3;
        case 7:
          return 4;
        default:
          assert(!"Invalid page");
          return 0;
        }
      }

      void ParsePattern(uint_t idx, Builder& target) const
      {
        PatternBuilder& patBuilder = target.StartPattern(idx);
        const auto& src = Source.Patterns[idx];
        uint_t lineNum = 0;
        for (; lineNum < PATTERN_SIZE; ++lineNum)
        {
          const auto& srcLine = src[lineNum];
          if (IsLastLine(srcLine))
          {
            patBuilder.Finish(lineNum);
            break;
          }
          patBuilder.StartLine(lineNum);
          ParseLine(srcLine, patBuilder, target);
        }
        const std::size_t patStart = offsetof(RawHeader, Patterns) + idx * sizeof(RawPattern);
        const std::size_t patSize = lineNum * sizeof(RawLine);
        AddFixedRange(patStart, patSize);
      }

      static bool IsLastLine(const RawLine& line)
      {
        return line[0].IsEnd() || line[1].IsEnd() || line[2].IsEnd() || line[3].IsEnd();
      }

      static void ParseLine(const RawLine& line, PatternBuilder& patBuilder, Builder& target)
      {
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
        {
          target.StartChannel(chanNum);
          const auto& note = line[chanNum];
          const uint_t halftones = note.GetNote();
          int_t sample = -1;
          if (halftones != NOTE_EMPTY)
          {
            target.SetNote(halftones - 1);
            sample = note.GetSample();
          }

          switch (note.GetCommand())
          {
          case CMD_SPEED:
            patBuilder.SetTempo(note.GetParameter());
            break;
          case CMD_SPECIAL:
            {
              switch (uint_t param = note.GetParameter())
              {
              case COMMAND_NONE:
                if (halftones == NOTE_EMPTY)
                {
                  break;
                }
              case COMMAND_NOORNAMENT:
                target.SetOrnament(0);
                break;
              case COMMAND_CONTSAMPLE:
                sample = -1;
                break;
              case COMMAND_ENDPATTERN:
                break;
              case COMMAND_BLOCKCHANNEL:
                target.SetRest();
                break;
              default:
                target.SetOrnament(param);
                break;
              }
            }
          }

          if (sample != -1)
          {
            target.SetSample(sample);
          }
        }
      }

      void AddFixedRange(std::size_t start, std::size_t size) const
      {
        Require(FixedRanges->AddRange(start, size));
      }
    private:
      const Binary::Container& RawData;
      const RawHeader& Source;
      const RangeChecker::Ptr FixedRanges;
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      const std::size_t size(rawData.Size());
      if (sizeof(RawHeader) > size)
      {
        return false;
      }
      const auto& header = *static_cast<const RawHeader*>(rawData.Start());
      if (header.Loop > header.Length)
      {
        return false;
      }
      return true;
    }

    const std::string FORMAT(
      //std::array<PDTOrnament, ORNAMENTS_COUNT> Ornaments;
      "(%xxxxxxx0{16}){11}"
      //std::array<PDTOrnamentLoop, ORNAMENTS_COUNT> OrnLoops;
      "?{22}"
      //uint8_t Padding1[6];
      "?{6}"
      //char Title[32];
      "?{32}"
      //uint8_t Tempo;
      "03-63"
      //uint8_t Start;
      "00-ef"
      //uint8_t Loop;
      "00-ef"
      //uint8_t Length;
      "01-f0"
      //uint8_t Padding2[16];
      "00{16}"
      //std::array<PDTSample, SAMPLES_COUNT> Samples;
      /*
      uint8_t Name[8];
      uint16_t Start;
      uint16_t Size;
      uint16_t Loop;
      uint8_t Page;
      uint8_t Padding;
      */
      "(???????? ?5x|3x|c0-ff ?00-40 ?5x|3x|c0-ff 00|01|03|04|06|07 00){16}"
      //std::array<uint8_t, POSITIONS_COUNT> Positions;
      "(00-1f){240}"
      //uint16_t LastDatas[PAGES_COUNT];
      "(?c0-ff){5}"
      /*
      uint8_t FreeRAM;
      uint8_t Padding3[5];
      */
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MODULE_SIZE))
      {
      }

      String GetDescription() const override
      {
        return Text::PRODIGITRACKER_DECODER_DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(const Binary::Container& rawData) const override
      {
        return FastCheck(rawData) && Format->Match(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (!Format->Match(rawData))
        {
          return Formats::Chiptune::Container::Ptr();
        }
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
      if (!FastCheck(data))
      {
        return Formats::Chiptune::Container::Ptr();
      }

      try
      {
        const Format format(data);

        format.ParseCommonProperties(target);

        StatisticCollectionBuilder statistic(target);
        format.ParsePositions(statistic);
        const Indices& usedPatterns = statistic.GetUsedPatterns();
        format.ParsePatterns(usedPatterns, statistic);
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, target);
        const Indices& usedOrnaments = statistic.GetUsedOrnaments();
        format.ParseOrnaments(usedOrnaments, target);

        const Binary::Container::Ptr subData = data.GetSubcontainer(0, MODULE_SIZE);
        const RangeChecker::Range fixedRange = format.GetFixedArea();
        return CreateCalculatingCrcContainer(subData, fixedRange.first, fixedRange.second - fixedRange.first);
      }
      catch (const std::exception&)
      {
        Dbg("Failed to create");
        return Formats::Chiptune::Container::Ptr();
      }
    }

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }//namespace ProDigiTracker

  Decoder::Ptr CreateProDigiTrackerDecoder()
  {
    return MakePtr<ProDigiTracker::Decoder>();
  }
} //namespace Chiptune
} //namespace Formats
