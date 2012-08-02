/**
*
* @file     core/plugins_parameters.h
* @brief    Plugins parameters names
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __CORE_PLUGINS_PARAMETERS_H_DEFINED__
#define __CORE_PLUGINS_PARAMETERS_H_DEFINED__

//common includes
#include <parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    namespace Core
    {
      //! @brief Plugins-related parameters namespace
      namespace Plugins
      {
        //! @brief Parameters#ZXTune#Core#Plugins namespace prefix
        extern const NameType PREFIX;

        //! @brief RAW scaner parameters namespace
        namespace Raw
        {
          //! @brief Parameters#ZXTune#Core#Plugins#Raw namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name Perform double analysis of plain data containers

          //! Parameter name
          extern const NameType PLAIN_DOUBLE_ANALYSIS;
          //@}

          //@{
          //! @name Minimal scan size parameter

          //! Default value
          const IntType MIN_SIZE_DEFAULT = 128;
          //! Parameter name
          extern const NameType MIN_SIZE;
          //@}
        }

        //! @brief HRIP container parameters namespace
        namespace Hrip
        {
          //! @brief Parameters#ZXTune#Core#Plugins#Hrip namespace prefix
          extern const NameType PREFIX;

          //! @brief Ignore corrupted blocks
          //! @details 1 if do so
          extern const NameType IGNORE_CORRUPTED;
        }

        //! @brief AY container/player parameters namespace
        namespace AY
        {
          //! @brief Parameters#ZXTune#Core#Plugins#AY namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name Default song duration (if not specified exactly)

          //! Default value (3min for 50 fps)
          const IntType DEFAULT_DURATION_FRAMES_DEFAULT = 3 * 60 * 50;
          //! Parameter name
          extern const NameType DEFAULT_DURATION_FRAMES;
          //@}
        }

        //! @brief ZIP container parameters namespace
        namespace Zip
        {
          //! @brief Parameters#ZXTune#Core#Plugins#Zip namespace prefix
          extern const NameType PREFIX;

          //@{
          //! @name Maximal file size to be depacked in Mb

          //! Default value
          const IntType MAX_DEPACKED_FILE_SIZE_MB_DEFAULT = 32;
          //! Parameter name
          extern const NameType MAX_DEPACKED_FILE_SIZE_MB;
          //@}
        }
      }
    }
  }
}
#endif //__CORE_PLUGINS_PARAMETERS_H_DEFINED__
