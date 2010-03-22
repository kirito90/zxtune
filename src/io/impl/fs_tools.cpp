/*
Abstract:
  Different io-related tools implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <error_tools.h>
#include <io/error_codes.h>
#include <io/fs_tools.h>

#include <cctype>

#include <text/io.h>

#define FILE_TAG 9AC2A0AC

namespace
{
  const Char FS_DELIMITERS[] = {
  #ifdef _WIN32
  '\\',
  #endif
  '/', '\0'};
  
  inline bool IsFSSymbol(Char sym)
  {
    return std::isalnum(sym) || sym == '_' || sym =='(' || sym == ')';
  }
}

namespace ZXTune
{
  namespace IO
  {
    std::string ConvertToFilename(const String& str)
    {
#ifdef UNICODE
      //TODO: ut8 convertor
      return std::string(str.begin(), str.end());
#else
      return str;
#endif
    }
    
    String ExtractFirstPathComponent(const String& path, String& restPart)
    {
      const String::size_type delimPos = path.find_first_of(FS_DELIMITERS);
      if (String::npos == delimPos)
      {
        restPart.clear();
        return path;
      }
      else
      {
        restPart = path.substr(delimPos + 1);
        return path.substr(0, delimPos);
      }
    }

    String ExtractLastPathComponent(const String& path, String& restPart)
    {
      const String::size_type delimPos = path.find_last_of(FS_DELIMITERS);
      if (String::npos == delimPos)
      {
        restPart.clear();
        return path;
      }
      else
      {
        restPart = path.substr(0, delimPos);
        return path.substr(delimPos + 1);
      }
    }
    
    String AppendPath(const String& path1, const String& path2)
    {
      static const String DELIMS(FS_DELIMITERS);
      String result = path1;
      if (!path1.empty() && String::npos == DELIMS.find(*path1.rbegin()) &&
          !path2.empty() && String::npos == DELIMS.find(*path2.begin()))
      {
        result += FS_DELIMITERS[0];
      }
      result += path2;
      return result;
    }

    String MakePathFromString(const String& input, Char replacing)
    {
      String result;
      for (String::const_iterator it = input.begin(), lim = input.end(); it != lim; ++it)
      {
        if (IsFSSymbol(*it))
        {
          result += *it;
        }
        else if (replacing != '\0' && !result.empty())
        {
          result += replacing;
        }
      }
      return replacing != '\0' ? result.substr(0, 1 + result.find_last_not_of(replacing)) : result;
    }

    std::auto_ptr<std::ofstream> CreateFile(const String& path, bool overwrite)
    {
      const std::string& pathC = ConvertToFilename(path);
      if (!overwrite && std::ifstream(pathC.c_str()))
      {
        throw MakeFormattedError(THIS_LINE, ERROR_FILE_EXISTS, TEXT_IO_ERROR_FILE_EXISTS, path);
      }
      std::auto_ptr<std::ofstream> res(new std::ofstream(pathC.c_str(), std::ios::binary));
      if (res.get() && res->good())
      {
        return res;
      }
      throw MakeFormattedError(THIS_LINE, ERROR_NOT_FOUND, TEXT_IO_ERROR_NOT_OPENED, path);
    }
  }
}
