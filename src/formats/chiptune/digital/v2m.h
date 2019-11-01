/**
* 
* @file
*
* @brief  V2M parser interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "formats/chiptune/builder_meta.h"
//library includes
#include <formats/chiptune.h>
#include <time/stamp.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace V2m
    {
      //Use simplified parsing due to thirdparty library used
      class Builder
      {
      public:
        virtual ~Builder() = default;

        virtual MetaBuilder& GetMetaBuilder() = 0;

        virtual void SetTotalDuration(Time::Milliseconds duration) = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }
  }
}
