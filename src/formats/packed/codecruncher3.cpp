/*
Abstract:
  CodeCruncher v3 support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  (C) Based on XLook sources by HalfElf
*/

//local includes
#include "pack_utils.h"
//common includes
#include <byteorder.h>
#include <detector.h>
#include <tools.h>
//library includes
#include <formats/packed.h>
//std includes
#include <iterator>
//boost includes
#include <boost/bind.hpp>

namespace CodeCruncher3
{
  const std::size_t MAX_DECODED_SIZE = 0xc000;

  const std::string DEPACKER_PATTERN(
    //classic depacker
    "?"       // di/nop
    "???"     // usually 'K','S','A','!'
    "21??"    // ld hl,xxxx                                        754a/88d2
    "11??"    // ld de,xxxx                                        5b81/5b81
    "017f00"  // ld bc,0x007f rest depacker size
    "d5"      // push de
    "edb0"    // ldir
    "21??"    // ld hl,xxxx last of src packed       (data = +#11) a15b/c949
    "11??"    // ld de,xxxx last of target depack    (data = +#14) ffff/ffff
    "01??"    // ld bc,xxxx size of packed           (data = +#17) 2b93/3ff9
    "c9"      // ret
    //+0x1a
    "ed?"     // lddr/ldir
    "21??"    // ld hl,xxxx src of packed after move (data = +#1d) d46d/c007
    "11??"    // ld de,xxxx dst of depack            (data = +#20) 7530/88b8
    "7e"      // ld a,(hl)
    "cb3f"    // srl a
    "382b"    // jr c,xx
    "e607"    // and 7
    "47"      // ld b,a
    "ed6f"    // rld
    "23"      // inc hl
    "4e"      // ld c,(hl)
    "23"      // inc hl
    "e5"      // push hl
    "08"      // ex af,af'
    "7e"      // ld a,(hl)
    "08"      // ex af,af'
    "62"      // ld h,d
    "6b"      // ld l,e
    "ed42"    // sbc hl,bc
    "0600"    // ld b,xx
    "3C"      // inc a
    "4f"      // ld c,a
    "03"      // inc bc
    "03"      // inc bc
    "edb0"    // ldir
    "fe10"    // cp xx
    "200b"    // jr nz,xx
    "e3"      // ld (sp),hl
    "23"      // inc hl
    "7e"      // ld a,(hl)
    "e3"      // ld (sp),hl
    "08"      // ex af,af'
    "4f"      // ld c,a
    "b7"      // or a
    "2802"    // jr z,xx
    "edb0"    // ldir
    "08"      // ex af,af'
    "e1"      // pop hl
    "18d1"    // jr xxxx
/*
    "23"      // inc hl
    "cb3f"    // srl a
    "3823"    // jr c,xx
    "1f"      // rra
    "3813"    // jr c,xx
    "1f"      // rra
    "4f"      // ld c,a
    "3803"    // jr c,xx
    "41"      // ld b,c
    "4e"      // ld c,(hl)
    "23"      // inc hl
    "7e"      // ld a,(hl)
    "12"      // ld (de),a
    "23"      // inc hl
    "7e"      // ld a,(hl)
    "08"      // ex af,af'
    "e5"      // push hl
    "62"      // ld h,d
    "6b"      // ld l,e
    "13"      // inc de
    "7a"      // ld a,d
    "18ce"    // jr xxxx
    "cb3f"    // srl a
    "4f"      // ld c,a
    "3003"    // jr nc,xxxx
    "41"      // ld b,c
    "4e"      // ld c,(hl)
    "23"      // inc hl
    "03"      // inc bc
    "edb0"    // ldir
    "18a8"    // jr xxxx
    "cb3f"    // srl a
    "380a"    // jr c,xx
    "1f"      // rra
    "3014"    // jr nc,xxxx
    "3d"      // dec a
    "12"      // ld (de),a
    "13"      // inc de
    "12"      // ld (de),a
    "13"      // inc de
    "189a"    // jr xxxx
    "08"      // ex af,af'
    "7e"      // ld a,(hl)
    "08"      // ex af,af'
    "e5"      // push hl
    "62"      // ld h,d
    "6b"      // ld l,e
    "2b"      // dec hl
    "3d"      // dec a
    "20fc"    // jr nz,xx
    "48"      // ld c,b
    "18a6"    // jr xxxx
    "?"       // ei/nop
    "c3??"    // jp xxxx
*/
  );

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    //+0
    uint8_t Padding1[0x11];
    //+0x11
    uint16_t PackedSource;
    //+0x13
    uint8_t Padding2;
    //+0x14
    uint16_t PackedTarget;
    //+0x16
    uint8_t Padding3;
    //+0x17
    uint16_t SizeOfPacked;
    //+0x19
    uint8_t Padding4[2];
    //+0x1b
    uint8_t PackedDataCopyDirection;
    //+0x1c
    uint8_t Padding5;
    //+0x1d
    uint16_t FirstOfPacked;
    //+0x1f
    uint8_t Padding6[0x7a];
    //+0x99
    uint8_t Data[1];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 0x99 + 1);

  bool IsFinishMarker(uint_t data)
  {
    return 3 == (data & 0x0f);
  }

  class Container
  {
  public:
    Container(const void* data, std::size_t size)
      : Data(static_cast<const uint8_t*>(data))
      , Size(size)
    {
    }

    bool FastCheck() const
    {
      if (Size < sizeof(RawHeader))
      {
        return false;
      }
      const RawHeader& header = GetHeader();
      const DataMovementChecker checker(fromLE(header.PackedSource), fromLE(header.PackedTarget), fromLE(header.SizeOfPacked), header.PackedDataCopyDirection);
      if (!checker.IsValid())
      {
        return false;
      }
      if (checker.FirstOfMovedData() != fromLE(header.FirstOfPacked))
      {
        return false;
      }
      const uint_t usedSize = GetUsedSize();
      if (usedSize > Size ||
          !IsFinishMarker(Data[usedSize - 1]))
      {
        return false;
      }
      return true;
    }

    uint_t GetUsedSize() const
    {
      const RawHeader& header = GetHeader();
      return sizeof(header) + fromLE(header.SizeOfPacked) - sizeof(header.Data);
    }

    const RawHeader& GetHeader() const
    {
      assert(Size >= sizeof(RawHeader));
      return *safe_ptr_cast<const RawHeader*>(Data);
    }
  private:
    const uint8_t* const Data;
    const std::size_t Size;
  };

  class DataDecoder
  {
  public:
    explicit DataDecoder(const Container& container)
      : IsValid(container.FastCheck())
      , Header(container.GetHeader())
      , Stream(Header.Data, fromLE(Header.SizeOfPacked))
    {
      assert(IsValid && !Stream.Eof());
    }

    Dump* GetDecodedData()
    {
      if (IsValid && !Stream.Eof())
      {
        IsValid = DecodeData();
      }
      return IsValid ? &Decoded : 0;
    }
  private:
    bool DecodeData()
    {
      // The main concern is to decode data as much as possible, skipping defenitely invalid structure
      Decoded.reserve(2 * fromLE(Header.SizeOfPacked));
      //assume that first byte always exists due to header format
      while (!Stream.Eof() && Decoded.size() < MAX_DECODED_SIZE)
      {
        const uint_t data = Stream.GetByte();
        if (IsFinishMarker(data))
        {
          //exit
          //do not check if real decoded size is equal to calculated;
          //do not check if eof is reached
          break;
        }
        else if (Stream.Eof())
        {
          return false;
        }
        const bool res = (0 == (data & 1))
          ? ProcessBackReference(data)
          : ProcessCommand(data);
        if (!res)
        {
          return false;
        }
      }
      //copy rest bytes to stream
      while (!Stream.Eof())
      {
        Decoded.push_back(Stream.GetByte());
      }
      return true;
    }

    bool ProcessBackReference(uint_t data)
    {
      const uint_t loNibble = data & 0x0f;
      const uint_t hiNibble = (data & 0xf0) >> 4;
      const uint_t shortLen = hiNibble + 3;
      const uint_t offset = 128 * loNibble + Stream.GetByte();
      const uint_t len = 0x0f == hiNibble ? shortLen + Stream.GetByte() : shortLen;
      return CopyFromBack(offset, Decoded, len);
    }

    bool ProcessCommand(uint_t data)
    {
      const uint_t loNibble = data & 0x0f;
      const uint_t hiNibble = (data & 0xf0) >> 4;

      switch (loNibble)
      {
      case 0x01://long RLE
        {
          const uint_t len = 256 * hiNibble + Stream.GetByte() + 3;
          std::fill_n(std::back_inserter(Decoded), len, Stream.GetByte());
        }
        return true;
      //case 0x03://exit
      case 0x05://short copy
        std::generate_n(std::back_inserter(Decoded), hiNibble + 1, boost::bind(&ByteStream::GetByte, &Stream));
        return true;
      case 0x09://short RLE
        std::fill_n(std::back_inserter(Decoded), hiNibble + 3, Stream.GetByte());
        return true;
      case 0x0b://2 bytes
        std::fill_n(std::back_inserter(Decoded), 2, hiNibble - 1);
        return true;
      case 0x0d://long copy
        {
          const uint_t len = 256 * hiNibble + Stream.GetByte() + 1;
          std::generate_n(std::back_inserter(Decoded), len, boost::bind(&ByteStream::GetByte, &Stream));
        }
        return true;
      default://short backref
        return CopyFromBack((data & 0xf8) >> 3, Decoded, 2);
      }
    }
  private:
    bool IsValid;
    const RawHeader& Header;
    ByteStream Stream;
    Dump Decoded;
  };
}

namespace Formats
{
  namespace Packed
  {
    class CodeCruncher3Decoder : public Decoder
    {
    public:
      CodeCruncher3Decoder()
        : Depacker(DataFormat::Create(CodeCruncher3::DEPACKER_PATTERN))
      {
      }

      virtual DataFormat::Ptr GetFormat() const
      {
        return Depacker;
      }

      virtual bool Check(const void* data, std::size_t availSize) const
      {
        const CodeCruncher3::Container container(data, availSize);
        return container.FastCheck() && Depacker->Match(data, availSize);
      }

      virtual std::auto_ptr<Dump> Decode(const void* data, std::size_t availSize, std::size_t& usedSize) const
      {
        const CodeCruncher3::Container container(data, availSize);
        if (!container.FastCheck() || !Depacker->Match(data, availSize))
        {
          return std::auto_ptr<Dump>();
        }
        CodeCruncher3::DataDecoder decoder(container);
        if (Dump* decoded = decoder.GetDecodedData())
        {
          usedSize = container.GetUsedSize();
          std::auto_ptr<Dump> res(new Dump());
          res->swap(*decoded);
          return res;
        }
        return std::auto_ptr<Dump>();
      }
    private:
      const DataFormat::Ptr Depacker;
    };

    Decoder::Ptr CreateCodeCruncher3Decoder()
    {
      return Decoder::Ptr(new CodeCruncher3Decoder());
    }
  }
}
