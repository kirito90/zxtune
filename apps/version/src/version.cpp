/*
Abstract:
  Version api implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <apps/version/api.h>
//library includes
#include <strings/format.h>
//text includes
#include "../text/text.h"

namespace
{
#define TOSTRING(a) #a
#define STR(a) TOSTRING(a)

  const std::string DATE(__DATE__);

// Used information from http://sourceforge.net/p/predef/wiki/Home/

// version definition-related
#ifndef BUILD_VERSION
  const std::string VERSION("develop");
#else
  const std::string VERSION(STR(BUILD_VERSION));
#endif

//detect platform
#if defined(_WIN32)
  const std::string PLATFORM("windows");
#elif defined(__linux__)
  const std::string PLATFORM("linux");
#elif defined(__FreeBSD__)
  const std::string PLATFORM("freebsd");
#elif defined(__NetBSD__)
  const std::string PLATFORM("netbsd");
#elif defined(__OpenBSD__)
  const std::string PLATFORM("openbsd")
#elif defined(__unix__)
  const std::string PLATFORM("unix");
#elif defined(__CYGWIN__)
  const std::string PLATFORM("cygwin");
#else
  const std::string PLATFORM("unknown-platform");
#endif

//detect arch
#if defined(_M_IX86) || defined(__i386__)
  const std::string ARCH("x86");
#elif defined(_M_AMD64) || defined(__amd64__)
  const std::string ARCH("x86_64");
#elif defined(_M_IA64) || defined(__ia64__)
  const std::string ARCH("ia64");
#elif defined(_M_ARM) || defined(__arm__)
  const std::string ARCH("arm");
#elif defined(_MIPSEL)
  const std::string ARCH("mipsel");
#elif defined(__powerpc64__)
  const std::string ARCH("ppc64");
#elif defined(_M_PPC) || defined(__powerpc__)
  const std::string ARCH("ppc");
#else
  const std::string ARCH("unknown-arch");
#endif

//detect toolset
#if defined(_MSC_VER)
  const std::string TOOLSET("msvs");
#elif defined(__MINGW32__)
  const std::string TOOLSET("mingw");
#elif defined(__GNUC__)
  const std::string TOOLSET("gnuc");
#elif defined(__clang__)
  const std::string TOOLSET("clang");
#else
  const std::string TOOLSET("unknown-toolset");
#endif
}

namespace Text
{
  extern const Char PROGRAM_NAME[];
}

String GetProgramTitle()
{
  return Text::PROGRAM_NAME;
}

String GetProgramVersion()
{
  return FromStdString(VERSION);
}

String GetBuildDate()
{
  return FromStdString(DATE);
}

String GetBuildPlatform()
{
  //some business-logic
  if (PLATFORM == "linux" && ARCH == "mipsel")
  {
    return FromStdString("dingux");
  }
  else if (PLATFORM == "windows" && TOOLSET == "mingw")
  {
    return FromStdString(TOOLSET);
  }
  else
  {
    return FromStdString(PLATFORM);
  }
}

String GetBuildArchitecture()
{
  return FromStdString(ARCH);
}

String GetProgramVersionString()
{
  return Strings::Format(Text::PROGRAM_VERSION_STRING,
    GetProgramTitle(),
    GetProgramVersion(),
    GetBuildDate(),
    GetBuildPlatform(),
    GetBuildArchitecture());
}
