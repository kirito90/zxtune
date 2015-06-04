/**
* 
* @file
*
* @brief  UMX containers support
* @note   http://wiki.beyondunreal.com/Legacy:Package_File_Format
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "decoders.h"
//common includes
#include <byteorder.h>
#include <contract.h>
//library includes
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <debug/log.h>
//std includes
#include <map>
//boost includes
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/archived.h>

namespace
{
  const Debug::Stream Dbg("Formats::Archived::UMX");
}

namespace UMX
{
  const std::string FORMAT(
    "c1832a9e"  //signature
    "? 00"      //version
    "??"        //license mode
    "????"      //package flags
    "??0000 ????" //names
    "??0000 ????" //exports
    "??0000 ????" //imports
  );

  using namespace Formats;

  typedef boost::array<uint8_t, 4> SignatureType;

  const SignatureType SIGNATURE = {{0xc1, 0x83, 0x2a, 0x9e}};

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct TableDescription
  {
    uint32_t Count;
    uint32_t Offset;
  } PACK_POST;

  PACK_PRE struct RawHeader
  {
    SignatureType Signature;
    uint16_t PackageVersion;
    uint16_t LicenseMode;
    uint32_t PackageFlags;
    TableDescription Names;
    TableDescription Exports;
    TableDescription Imports;
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 36);

  struct Index
  {
    int_t Value;

    Index()
      : Value()
    {
    }
  };

  struct ClassName
  {
    const std::string Name;

    explicit ClassName(const std::string& name)
      : Name(boost::algorithm::to_lower_copy(name))
    {
    }

    bool IsMusic() const
    {
      return Name == "music"
          || Name == "sound"
      ;
    }
  };

  struct Property
  {
    const std::string Name;

    explicit Property(const std::string& name)
      : Name(boost::algorithm::to_lower_copy(name))
    {
    }

    bool IsLimiter() const
    {
      return Name == "none";
    }
  };

  struct NameEntry
  {
    std::string Name;
    uint32_t Flags;

    NameEntry()
      : Flags()
    {
    }
  };

  struct ExportEntry
  {
    Index Class;
    Index Super;
    uint32_t Group;
    Index ObjectName;
    uint32_t ObjectFlags;
    Index SerialSize;
    Index SerialOffset;

    ExportEntry()
      : Group()
      , ObjectFlags()
    {
    }
  };

  struct ImportEntry
  {
    Index ClassPackage;
    Index ClassName;
    uint32_t Package;
    Index ObjectName;

    ImportEntry()
      : Package()
    {
    }
  };

  class InputStream
  {
  public:
    InputStream(uint_t version, const Binary::Container& data)
      : Version(version)
      , Data(data)
      , Start(static_cast<const uint8_t*>(Data.Start()))
      , Cursor(Start)
      , Limit(Start + Data.Size())
      , MaxUsedSize()
    {
    }

    //primitives
    template<class T>
    void Read(T& res)
    {
      Require(Cursor + sizeof(T) <= Limit);
      res = fromLE(*safe_ptr_cast<const T*>(Cursor));
      Cursor += sizeof(T);
    }

    void Read(Index& res)
    {
      int_t result = 0;
      bool negative = false;
      for (uint_t i = 0; i < 5; ++i)
      {
        const uint8_t data = ReadByte();
        if (i == 0)
        {
          negative = 0 != (data & 0x80);
          result = data & 0x3f;
          if (0 == (data & 0x40))
          {
            break;
          }
        }
        else if (i == 4)
        {
          result |= int_t(data & 0x1f) << 27;
        }
        else
        {
          result |= int_t(data & 0x7f) << (7 * i - 1);
          if (0 == (data & 0x80))
          {
            break;
          }
        }
      }
      res.Value = negative ? -result : +result;
    }

    void Read(std::string& res)
    {
      const uint8_t* const limit = Version >= 64
        ? Cursor + ReadByte()
        : Limit;
      Require(limit <= Limit);
      const uint8_t* const strEnd = std::find(Cursor, limit, 0);
      Require(strEnd != Limit);
      res.assign(Cursor, strEnd);
      Cursor = strEnd + 1;
    }

    //complex types
    void Read(NameEntry& res)
    {
      Read(res.Name);
      Read(res.Flags);
    }

    void Read(ExportEntry& res)
    {
      Read(res.Class);
      Read(res.Super);
      Read(res.Group);
      Read(res.ObjectName);
      Read(res.ObjectFlags);
      Read(res.SerialSize);
      if (res.SerialSize.Value > 0)
      {
        Read(res.SerialOffset);
      }
    }

    void Read(ImportEntry& res)
    {
      Read(res.ClassPackage);
      Read(res.ClassName);
      Read(res.Package);
      Read(res.ObjectName);
    }

    void Read(Property& res)
    {
      Require(!res.IsLimiter());
      Require(!"TODO: implement");
    }

    Binary::Container::Ptr ReadContainer(std::size_t size)
    {
      Require(Cursor + size <= Limit);
      Cursor += size;
      return Data.GetSubcontainer(Cursor - size - Start, size);
    }

    Binary::Container::Ptr ReadRestContainer()
    {
      return ReadContainer(Limit - Cursor);
    }

    void Seek(std::size_t pos)
    {
      Require(Start + pos < Limit);
      MaxUsedSize = GetMaxUsedSize();
      Cursor = Start + pos;
    }

    std::size_t GetMaxUsedSize()
    {
      return std::max<std::size_t>(MaxUsedSize, Cursor - Start);
    }
  private:
    uint8_t ReadByte()
    {
      Require(Cursor != Limit);
      return *Cursor++;
    }
  private:
    const uint_t Version;
    const Binary::Container& Data;
    const uint8_t* const Start;
    const uint8_t* Cursor;
    const uint8_t* const Limit;
    std::size_t MaxUsedSize;
  };

  class Format
  {
  public:
    explicit Format(const Binary::Container& data)
      : Data(data)
      , Header(*static_cast<const RawHeader*>(data.Start()))
      , UsedSize(sizeof(Header))
    {
      Require(Header.Signature == SIGNATURE);
      InputStream stream(fromLE(Header.PackageVersion), Data);
      ReadNames(stream);
      ReadExports(stream);
      ReadImports(stream);
      UsedSize = stream.GetMaxUsedSize();
    }

    std::size_t GetUsedSize() const
    {
      return UsedSize;
    }

    uint_t GetEntriesCount() const
    {
      return Exports.size();
    }

    String GetEntryName(uint_t idx) const
    {
      const ExportEntry& exp = Exports.at(idx);
      const std::string& name = GetName(exp.ObjectName);
      return FromStdString(name);
    }

    Binary::Container::Ptr GetEntryData(uint_t idx) const
    {
      try
      {
        const ExportEntry& exp = Exports.at(idx);
        const std::size_t offset = exp.SerialOffset.Value;
        const std::size_t size = exp.SerialSize.Value;
        const Binary::Container::Ptr entryData = Data.GetSubcontainer(offset, size);
        Require(entryData.get() != 0);
        InputStream stream(fromLE(Header.PackageVersion), *entryData);
        ReadProperties(stream);
        const ClassName& cls = GetClass(exp.Class);
        Dbg("Entry[%1%] data at %2% size=%3% class=%4%", idx, offset, size, cls.Name);
        const Binary::Container::Ptr result = cls.IsMusic()
           ? ExtractMusicData(stream)
           : stream.ReadRestContainer();
        UsedSize = std::max(UsedSize, offset + stream.GetMaxUsedSize());
        return result;
      }
      catch (const std::exception&)
      {
        return Binary::Container::Ptr();
      }
    }
  private:
    void ReadNames(InputStream& stream)
    {
      stream.Seek(fromLE(Header.Names.Offset));
      const uint_t count = fromLE(Header.Names.Count);
      Names.resize(count);
      for (uint_t idx = 0; idx != count; ++idx)
      {
        NameEntry& entry = Names[idx];
        stream.Read(entry);
        Dbg("Names[%1%] = '%2%'", idx, entry.Name);
      }
    }

    void ReadExports(InputStream& stream)
    {
      stream.Seek(fromLE(Header.Exports.Offset));
      const uint_t count = fromLE(Header.Exports.Count);
      Exports.resize(count);
      for (uint_t idx = 0; idx != count; ++idx)
      {
        ExportEntry& entry = Exports[idx];
        stream.Read(entry);
        Dbg("Exports[%1%] = {class=%2% name=%3% super=%4% size=%5%}",
          idx, entry.Class.Value, entry.ObjectName.Value, entry.Super.Value, entry.SerialSize.Value);
      }
    }

    void ReadImports(InputStream& stream)
    {
      stream.Seek(fromLE(Header.Imports.Offset));
      const uint_t count = fromLE(Header.Imports.Count);
      Imports.resize(count);
      for (uint_t idx = 0; idx != count; ++idx)
      {
        ImportEntry& entry = Imports[idx];
        stream.Read(entry);
        Dbg("Imports[%1%] = {pkg=%2% class=%3% name=%4%}",
          idx, entry.ClassPackage.Value, entry.ClassName.Value, entry.ObjectName.Value);
      }
    }

    const std::string& GetName(Index idx) const
    {
      return Names.at(idx.Value).Name;
    }

    ClassName GetClass(Index idx) const
    {
      if (idx.Value == 0)
      {
        return ClassName("(null)");
      }
      else if (idx.Value < 0)
      {
        const ImportEntry& entry = Imports.at(-idx.Value - 1);
        return ClassName(GetName(entry.ObjectName));
      }
      else
      {
        const ExportEntry& entry = Exports.at(idx.Value - 1);
        return ClassName(GetName(entry.ObjectName));
      }
    }

    void ReadProperties(InputStream& stream) const
    {
      for (;;)
      {
        Index nameIdx;
        stream.Read(nameIdx);
        Property prop(GetName(nameIdx));
        if (prop.IsLimiter())
        {
          break;
        }
        stream.Read(prop);
      }
    }

    Binary::Container::Ptr ExtractMusicData(InputStream& stream) const
    {
      Index format;
      format.Value = -1000;
      const uint_t version = fromLE(Header.PackageVersion);
      if (version >= 120)
      {
        uint32_t flags, aux;
        stream.Read(format);
        stream.Read(flags);
        stream.Read(aux);
      }
      else if (version >= 100)
      {
        uint32_t flags, aux;
        stream.Read(flags);
        stream.Read(format);
        stream.Read(aux);
      }
      else if (version >= 62)
      {
        uint32_t flags;
        stream.Read(format);
        stream.Read(flags);
      }
      else
      {
        stream.Read(format);
      }
      Index size;
      stream.Read(size);
      Dbg("Extract music data from container (version=%1% format=%2% size=%3%)", version, format.Value, size.Value);
      return stream.ReadContainer(size.Value);
    }
  private:
    const Binary::Container& Data;
    const RawHeader Header;
    mutable std::size_t UsedSize;
    std::vector<NameEntry> Names;
    std::vector<ExportEntry> Exports;
    std::vector<ImportEntry> Imports;
  };

  typedef std::map<String, Binary::Container::Ptr> NamedDataMap;

  class File : public Archived::File
  {
  public:
    explicit File(NamedDataMap::const_iterator it)
      : Name(it->first)
      , Data(it->second)
    {
    }

    virtual String GetName() const
    {
      return Name;
    }

    virtual std::size_t GetSize() const
    {
      return Data->Size();
    }

    virtual Binary::Container::Ptr GetData() const
    {
      return Data;
    }
  private:
    const String Name;
    const Binary::Container::Ptr Data;
  };

  class Container : public Archived::Container
  {
  public:
    Container(Binary::Container::Ptr delegate, NamedDataMap::const_iterator begin, NamedDataMap::const_iterator end)
      : Delegate(delegate)
      , Files(begin, end)
    {
    }

    //Binary::Container
    virtual const void* Start() const
    {
      return Delegate->Start();
    }

    virtual std::size_t Size() const
    {
      return Delegate->Size();
    }

    virtual Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      return Delegate->GetSubcontainer(offset, size);
    }

    //Archive::Container
    virtual void ExploreFiles(const Archived::Container::Walker& walker) const
    {
      for (NamedDataMap::const_iterator it = Files.begin(), lim = Files.end(); it != lim; ++it)
      {
        const File file(it);
        walker.OnFile(file);
      }
    }

    virtual Archived::File::Ptr FindFile(const String& name) const
    {
      const NamedDataMap::const_iterator it = Files.find(name);
      return it != Files.end()
        ? boost::make_shared<File>(it)
        : Archived::File::Ptr();
    }

    virtual uint_t CountFiles() const
    {
      return static_cast<uint_t>(Files.size());
    }
  private:
    const Binary::Container::Ptr Delegate;
    NamedDataMap Files;
  };
}

namespace Formats
{
  namespace Archived
  {
    class UMXDecoder : public Decoder
    {
    public:
      UMXDecoder()
        : Format(Binary::CreateFormat(UMX::FORMAT))
      {
      }

      virtual String GetDescription() const
      {
        return Text::UMX_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual Container::Ptr Decode(const Binary::Container& data) const
      {
        if (!Format->Match(data))
        {
          return Container::Ptr();
        }
        const ::UMX::Format format(data);
        ::UMX::NamedDataMap datas;
        for (uint_t idx = 0, lim = format.GetEntriesCount(); idx != lim; ++idx)
        {
          if (const Binary::Container::Ptr data = format.GetEntryData(idx))
          {
            const String& name = format.GetEntryName(idx);
            datas[name] = data;
          }
        }
        if (!datas.empty())
        {
          const Binary::Container::Ptr archive = data.GetSubcontainer(0, format.GetUsedSize());
          return boost::make_shared< ::UMX::Container>(archive, datas.begin(), datas.end());
        }
        Dbg("No files found");
        return Container::Ptr();
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Decoder::Ptr CreateUMXDecoder()
    {
      return boost::make_shared<UMXDecoder>();
    }
  }
}
