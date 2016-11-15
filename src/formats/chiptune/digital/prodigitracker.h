/**
* 
* @file
*
* @brief  ProDigiTracker support interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"
#include "formats/chiptune/objects.h"
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace ProDigiTracker
    {
      typedef LinesObject<int_t> Ornament;
    
      class Builder
      {
      public:
        virtual ~Builder() = default;

        virtual MetaBuilder& GetMetaBuilder() = 0;
        //common properties
        virtual void SetInitialTempo(uint_t tempo) = 0;
        //samples
        virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr sample) = 0;
        virtual void SetOrnament(uint_t index, Ornament ornament) = 0;
        //patterns
        virtual void SetPositions(std::vector<uint_t> positions, uint_t loop) = 0;

        virtual PatternBuilder& StartPattern(uint_t index) = 0;

        //! @invariant Channels are built sequentally
        virtual void StartChannel(uint_t index) = 0;
        virtual void SetRest() = 0;
        virtual void SetNote(uint_t note) = 0;
        virtual void SetSample(uint_t sample) = 0;
        virtual void SetOrnament(uint_t ornament) = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }

    Decoder::Ptr CreateProDigiTrackerDecoder();
  }
}
