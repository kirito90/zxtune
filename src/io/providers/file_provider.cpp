/*
Abstract:
  Providers enumerator

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//include first due to strange problems with curl includes
#include <io/error_codes.h>

//local includes
#include "enumerator.h"
#include "providers_factories.h"
//common includes
#include <contract.h>
#include <debug_log.h>
#include <error_tools.h>
#include <tools.h>
//library includes
#include <io/fs_tools.h>
#include <io/providers_parameters.h>
#include <l10n/api.h>
//std includes
#include <fstream>
//boost includes
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_array.hpp>
//text includes
#include <io/text/io.h>

#define FILE_TAG 0D4CB3DA

#undef min

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::IO;

  const Debug::Stream Dbg("IO::Provider::File");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("io");

  class FileProviderParameters
  {
  public:
    explicit FileProviderParameters(const Parameters::Accessor& accessor)
      : Accessor(accessor)
    {
    }

    std::streampos GetMMapThreshold() const
    {
      Parameters::IntType intVal = Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD_DEFAULT;
      Accessor.FindValue(Parameters::ZXTune::IO::Providers::File::MMAP_THRESHOLD, intVal);
      return static_cast<std::streampos>(intVal);
    }
  private:
    const Parameters::Accessor& Accessor;
  };

  // uri-related constants
  const Char SCHEME_SIGN[] = {':', '/', '/', 0};
  const Char SCHEME_FILE[] = {'f', 'i', 'l', 'e', 0};
  const Char SUBPATH_DELIMITER = '\?';

  class FileDataContainer : public Binary::Container
  {
    // basic interface for internal data storing
    class Holder
    {
    public:
      typedef boost::shared_ptr<Holder> Ptr;
      virtual ~Holder() {}
      
      virtual std::size_t Size() const = 0;
      virtual const uint8_t* Data() const = 0;
    };

    // memory-mapping holder implementation
    class MMapHolder : public Holder
    {
    public:
      explicit MMapHolder(const String& path)
      try
        : File(ConvertToFilename(path).c_str(), boost::interprocess::read_only), Region(File, boost::interprocess::read_only)
      {
      }
      catch (const boost::interprocess::interprocess_exception& e)
      {
        throw Error(THIS_LINE, ERROR_IO_ERROR, FromStdString(e.what()));
      }

      virtual std::size_t Size() const
      {
        return Region.get_size();
      }
      
      virtual const uint8_t* Data() const
      {
        return static_cast<const uint8_t*>(Region.get_address());
      }
    private:
      boost::interprocess::file_mapping File;
      boost::interprocess::mapped_region Region;
    };

    // simple buffer implementation
    class DumpHolder : public Holder
    {
    public:
      DumpHolder(const boost::shared_array<uint8_t>& dump, std::size_t size)
        : Dump(dump), Length(size)
      {
      }
      
      virtual std::size_t Size() const
      {
        return Length;
      }
      
      virtual const uint8_t* Data() const
      {
        return Dump.get();
      }
    private:
      const boost::shared_array<uint8_t> Dump;
      const std::size_t Length;
    };
  public:
    FileDataContainer(const String& path, const Parameters::Accessor& params)
      : CoreHolder()
      , Offset(0), Length(0)
    {
      //TODO: possible use boost.filesystem to determine file size
      std::ifstream file(ConvertToFilename(path).c_str(), std::ios::binary);
      if (!file)
      {
        throw Error(THIS_LINE, ERROR_NO_ACCESS, translate("Failed to get access."));
      }
      file.seekg(0, std::ios::end);
      const std::streampos fileSize = file.tellg();
      if (!fileSize || !file)
      {
        throw Error(THIS_LINE, ERROR_IO_ERROR, translate("File is empty."));
      }
      const FileProviderParameters providerParams(params);
      const std::streampos threshold = providerParams.GetMMapThreshold();

      if (fileSize >= threshold)
      {
        Dbg("Using memory-mapped i/o for '%1%'.", path);
        file.close();
        //use mmap
        CoreHolder.reset(new MMapHolder(path));
      }
      else
      {
        Dbg("Reading '%1%' to memory.", path);
        boost::shared_array<uint8_t> buffer(new uint8_t[fileSize]);
        file.seekg(0);
        file.read(safe_ptr_cast<char*>(buffer.get()), std::streamsize(fileSize));
        file.close();
        //use dump
        CoreHolder.reset(new DumpHolder(buffer, fileSize));
      }
      Length = fileSize;
    }
    
    virtual std::size_t Size() const
    {
      return Length;
    }
    
    virtual const void* Data() const
    {
      return CoreHolder->Data() + Offset;
    }
    
    virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      size = std::min(size, Length - offset);
      return Ptr(new FileDataContainer(CoreHolder, offset + Offset, size));
    }
  private:
    FileDataContainer(Holder::Ptr holder, std::size_t offset, std::size_t size)
      : CoreHolder(holder), Offset(offset), Length(size)
    {
    }
    
  private:
    Holder::Ptr CoreHolder;
    const std::size_t Offset;
    std::size_t Length;
  };

  class FileIdentifier : public Identifier
  {
  public:
    FileIdentifier(const String& path, const String& subpath)
      : PathValue(path)
      , SubpathValue(subpath)
      , FullValue(Serialize())
    {
      Require(!PathValue.empty());
    }

    virtual String Full() const
    {
      return FullValue;
    }

    virtual String Scheme() const
    {
      return SCHEME_FILE;
    }

    virtual String Path() const
    {
      return PathValue;
    }

    virtual String Filename() const
    {
      String rest;
      return ZXTune::IO::ExtractLastPathComponent(PathValue, rest);
    }

    virtual String Extension() const
    {
      const String& filename = Filename();
      const String::size_type dotPos = filename.find_last_of('.');
      return dotPos != String::npos
        ? filename.substr(dotPos + 1)
        : String();
    }

    virtual String Subpath() const
    {
      return SubpathValue;
    }

    virtual Ptr WithSubpath(const String& subpath) const
    {
      return boost::make_shared<FileIdentifier>(PathValue, subpath);
    }
  private:
    String Serialize() const
    {
      //do not place scheme
      String res = PathValue;
      if (!SubpathValue.empty())
      {
        res += SUBPATH_DELIMITER;
        res += SubpathValue;
      }
      return res;
    }
  private:
    const String PathValue;
    const String SubpathValue;
    const String FullValue;
  };


  inline bool IsOrdered(String::size_type lh, String::size_type rh)
  {
    return String::npos == rh ? true : lh < rh;
  }

  ///////////////////////////////////////
  class FileDataProvider : public DataProvider
  {
  public:
    virtual String Id() const
    {
      return Text::IO_FILE_PROVIDER_ID;
    }

    virtual String Description() const
    {
      return translate("Local files and file:// scheme support");
    }

    virtual Error Status() const
    {
      return Error();
    }

    virtual StringSet Schemes() const
    {
      static const Char* SCHEMES[] = 
      {
        SCHEME_FILE
      };
      return StringSet(SCHEMES, ArrayEnd(SCHEMES));
    }

    virtual Identifier::Ptr Resolve(const String& uri) const
    {
      const String::size_type schemePos = uri.find(SCHEME_SIGN);
      const String::size_type hierPos = String::npos == schemePos ? 0 : schemePos + ArraySize(SCHEME_SIGN) - 1;
      const String::size_type subPos = uri.find_first_of(SUBPATH_DELIMITER, hierPos);

      const String scheme = String::npos == schemePos ? String(SCHEME_FILE) : uri.substr(0, schemePos);
      const String path = String::npos == subPos ? uri.substr(hierPos) : uri.substr(hierPos, subPos - hierPos);
      const String subpath = String::npos == subPos ? String() : uri.substr(subPos + 1);
      return !path.empty() && scheme == SCHEME_FILE
        ? boost::make_shared<FileIdentifier>(path, subpath)
        : Identifier::Ptr();
    }

    virtual Binary::Container::Ptr Open(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& /*cb*/) const
    {
      try
      {
        return Binary::Container::Ptr(new FileDataContainer(path, params));
      }
      catch (const Error& e)
      {
        throw MakeFormattedError(THIS_LINE, ERROR_NOT_OPENED, translate("Failed to open file '%1%'."), path).AddSuberror(e);
      }
    }
  };
}

namespace ZXTune
{
  namespace IO
  {
    DataProvider::Ptr CreateFileDataProvider()
    {
      return boost::make_shared<FileDataProvider>();
    }

    void RegisterFileProvider(ProvidersEnumerator& enumerator)
    {
      enumerator.RegisterProvider(CreateFileDataProvider());
    }
  }
}
