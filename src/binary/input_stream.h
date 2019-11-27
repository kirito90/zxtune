/**
*
* @file
*
* @brief  Input binary stream helper
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <binary/container.h>
#include <binary/data_view.h>
//common includes
#include <byteorder.h>
#include <contract.h>
#include <pointers.h>
#include <types.h>
//std includes
#include <algorithm>
#include <cstring>

namespace Binary
{
  //! @brief Sequental stream adapter
  class DataInputStream
  {
  public:
    explicit DataInputStream(Binary::DataView data)
      : Start(static_cast<const uint8_t*>(data.Start()))
      , Finish(Start + data.Size())
      , Cursor(Start)
    {
    }

    //! @brief Simple adapter to read specified type data
    template<class T>
    const T& ReadField()
    {
      static_assert(!std::is_integral<T>::value, "Use ReadByte/ReadLE/ReadBE");
      return *safe_ptr_cast<const T*>(ReadRawData(sizeof(T)));
    }

    uint8_t ReadByte()
    {
      return *ReadRawData(1);
    }

    template<class T>
    T ReadLE()
    {
      return ::ReadLE<T>(ReadRawData(sizeof(T)));
    }

    template<class T>
    T ReadBE()
    {
      return ::ReadBE<T>(ReadRawData(sizeof(T)));
    }

    //! @brief Read ASCIIZ string with specified maximal size
    StringView ReadCString(std::size_t maxSize)
    {
      static_assert(sizeof(StringView::value_type) == sizeof(uint8_t), "Invalid char size");
      const auto limit = std::min(Cursor + maxSize, Finish);
      const auto strEnd = std::find(Cursor, limit, 0);
      Require(strEnd != limit);
      StringView res(safe_ptr_cast<const Char*>(Cursor), safe_ptr_cast<const Char*>(strEnd));
      Cursor = strEnd + 1;
      return res;
    }

    //! @brief Read string till EOL
    StringView ReadString()
    {
      const uint8_t CR = 0x0d;
      const uint8_t LF = 0x0a;
      const uint8_t EOT = 0x00;
      static const uint8_t EOLCODES[3] = {CR, LF, EOT};

      Require(Cursor != Finish);
      const auto eolPos = std::find_first_of(Cursor, Finish, EOLCODES, EOLCODES + 3);
      auto nextLine = eolPos;
      if (nextLine != Finish && CR == *nextLine++)
      {
        if (nextLine != Finish && LF == *nextLine)
        {
          ++nextLine;
        }
      }
      StringView result(safe_ptr_cast<const Char*>(Cursor), safe_ptr_cast<const Char*>(eolPos));
      Cursor = nextLine;
      return result;
    }

    const uint8_t* PeekRawData(std::size_t size)
    {
      if (Cursor + size <= Finish)
      {
        return Cursor;
      }
      else
      {
        return nullptr;
      }
    }

    //! @brief Read as much data as possible
    std::size_t Read(void* buf, std::size_t len)
    {
      const std::size_t res = std::min(len, GetRestSize());
      std::memcpy(buf, ReadRawData(res), res);
      return res;
    }

    DataView ReadData(std::size_t size)
    {
      return DataView(ReadRawData(size), size);
    }

    DataView ReadRestData()
    {
      Require(Cursor < Finish);
      return ReadData(GetRestSize());
    }

    void Skip(std::size_t size)
    {
      Require(Cursor + size <= Finish);
      Cursor += size;
    }
    
    void Seek(std::size_t pos)
    {
      Require(Start + pos <= Finish);
      Cursor = Start + pos;
    }

    //! @brief Return absolute read position
    std::size_t GetPosition() const
    {
      return Cursor - Start;
    }

    //! @brief Return available data size
    std::size_t GetRestSize() const
    {
      return Finish - Cursor;
    }
  private:
    const uint8_t* ReadRawData(std::size_t size)
    {
      Require(Cursor + size <= Finish);
      const uint8_t* const res = Cursor;
      Cursor += size;
      return res;
    }
    
  protected:
    const uint8_t* const Start;
    const uint8_t* const Finish;
    const uint8_t* Cursor;
  };
  
  //TODO: rename
  class InputStream : public DataInputStream
  {
  public:
    explicit InputStream(const Container& rawData)
      : DataInputStream(rawData)
      , Data(rawData)
    {
    }
    
    Container::Ptr ReadContainer(std::size_t size)
    {
      Require(Cursor + size <= Finish);
      const std::size_t offset = GetPosition();
      Cursor += size;
      return Data.GetSubcontainer(offset, size);
    }

    //! @brief Read rest data in source container
    Container::Ptr ReadRestContainer()
    {
      Require(Cursor < Finish);
      const std::size_t offset = GetPosition();
      const std::size_t size = GetRestSize();
      Cursor = Finish;
      return Data.GetSubcontainer(offset, size);
    }

    //! @brief Return data that is already read
    Container::Ptr GetReadContainer() const
    {
      return Data.GetSubcontainer(0, GetPosition());
    }
  private:
    const Container& Data;
  };
}
