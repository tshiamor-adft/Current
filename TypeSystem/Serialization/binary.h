/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Maxim Zhurovich <zhurovich@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#ifndef CURRENT_TYPE_SYSTEM_SERIALIZATION_BINARY_H
#define CURRENT_TYPE_SYSTEM_SERIALIZATION_BINARY_H

#include "exceptions.h"

#include "../struct.h"

namespace current {
namespace serialization {

// Using platform-independent size type for binary (de)serialization.
typedef uint64_t BINARY_FORMAT_SIZE_TYPE;

inline void SaveSizeIntoBinary(std::ostream& ostream, const size_t size) {
  BINARY_FORMAT_SIZE_TYPE save_size = size;
  size_t bytes_written =
      ostream.rdbuf()->sputn(reinterpret_cast<const char*>(&save_size), sizeof(BINARY_FORMAT_SIZE_TYPE));
  if (bytes_written != sizeof(BINARY_FORMAT_SIZE_TYPE)) {
    throw BinarySaveToStreamException(sizeof(BINARY_FORMAT_SIZE_TYPE), bytes_written);
  }
};

inline BINARY_FORMAT_SIZE_TYPE LoadSizeFromBinary(std::istream& istream) {
  BINARY_FORMAT_SIZE_TYPE result;
  size_t bytes_read = istream.rdbuf()->sgetn(reinterpret_cast<char*>(&result), sizeof(BINARY_FORMAT_SIZE_TYPE));
  if (bytes_read != sizeof(BINARY_FORMAT_SIZE_TYPE)) {
    throw BinaryLoadFromStreamException(sizeof(BINARY_FORMAT_SIZE_TYPE), bytes_read);
  }
  return result;
}

template <typename T>
struct SaveIntoBinaryImpl;

struct SavePrimitiveTypeIntoBinary {
  template <typename T>
  static void Save(std::ostream& ostream, const T& value) {
    const size_t bytes_written =
        ostream.rdbuf()->sputn(reinterpret_cast<const char*>(std::addressof(value)), sizeof(T));
    if (bytes_written != sizeof(T)) {
      throw BinarySaveToStreamException(sizeof(T), bytes_written);
    }
  }

  static void Save(std::ostream& ostream, const std::string& value) {
    SaveSizeIntoBinary(ostream, value.size());
    const size_t bytes_written = ostream.rdbuf()->sputn(value.data(), value.size());
    if (bytes_written != value.size()) {
      throw BinarySaveToStreamException(value.size(), bytes_written);
    }
  }
};

#define CURRENT_DECLARE_PRIMITIVE_TYPE(unused_typeid_index, cpp_type, unused_current_type) \
  template <>                                                                              \
  struct SaveIntoBinaryImpl<cpp_type> {                                                    \
    static void Save(std::ostream& ostream, const cpp_type& value) {                       \
      SavePrimitiveTypeIntoBinary::Save(ostream, value);                                   \
    }                                                                                      \
  };
#include "../primitive_types.dsl.h"
#undef CURRENT_DECLARE_PRIMITIVE_TYPE

template <typename T>
struct SaveIntoBinaryImpl<std::vector<T>> {
  static void Save(std::ostream& ostream, const std::vector<T>& value) {
    SaveSizeIntoBinary(ostream, value.size());
    for (const auto& element : value) {
      SaveIntoBinaryImpl<T>::Save(ostream, element);
    }
  }
};

template <typename TF, typename TS>
struct SaveIntoBinaryImpl<std::pair<TF, TS>> {
  static void Save(std::ostream& ostream, const std::pair<TF, TS>& value) {
    SaveIntoBinaryImpl<TF>::Save(ostream, value.first);
    SaveIntoBinaryImpl<TS>::Save(ostream, value.second);
  }
};

template <typename TK, typename TV>
struct SaveIntoBinaryImpl<std::map<TK, TV>> {
  static void Save(std::ostream& ostream, const std::map<TK, TV>& value) {
    SaveSizeIntoBinary(ostream, value.size());
    for (const auto& element : value) {
      SaveIntoBinaryImpl<TK>::Save(ostream, element.first);
      SaveIntoBinaryImpl<TV>::Save(ostream, element.second);
    }
  }
};

template <typename T>
struct SaveIntoBinaryImpl {
  struct SaveFieldVisitor {
    std::ostream& ostream_;

    explicit SaveFieldVisitor(std::ostream& ostream) : ostream_(ostream) {}

    template <typename U>
    void operator()(const char*, const U& source) const {
      SaveIntoBinaryImpl<U>::Save(ostream_, source);
    }
  };

  // `CURRENT_STRUCT`.
  template <typename TT = T>
  static ENABLE_IF<IS_CURRENT_STRUCT(TT)> Save(std::ostream& ostream, const T& source) {
    SaveFieldVisitor visitor(ostream);
    current::reflection::VisitAllFields<bricks::decay<T>,
                                        current::reflection::FieldNameAndImmutableValue>::WithObject(source,
                                                                                                     visitor);
  }

  template <typename TT = T>
  static ENABLE_IF<std::is_enum<TT>::value> Save(std::ostream& ostream, const T& value) {
    SavePrimitiveTypeIntoBinary::Save(ostream, static_cast<typename std::underlying_type<T>::type>(value));
  }
};

template <typename T>
inline void SaveIntoBinary(std::ostream& ostream, const T& source) {
  using DECAYED_T = bricks::decay<T>;
  SaveIntoBinaryImpl<DECAYED_T>::Save(ostream, source);
}

template <typename T>
struct LoadFromBinaryImpl;

struct LoadPrimitiveTypeFromBinary {
  template <typename T>
  static void Load(std::istream& istream, T& destination) {
    const size_t bytes_read =
        istream.rdbuf()->sgetn(reinterpret_cast<char*>(std::addressof(destination)), sizeof(T));
    if (bytes_read != sizeof(T)) {
      throw BinaryLoadFromStreamException(sizeof(T), bytes_read);
    }
  }

  static void Load(std::istream& istream, std::string& destination) {
    BINARY_FORMAT_SIZE_TYPE size = LoadSizeFromBinary(istream);
    destination.resize(static_cast<size_t>(size));
    // Should be legitimate since c++11 requires internal buffer to be contiguous.
    const size_t bytes_read = istream.rdbuf()->sgetn(const_cast<char*>(destination.data()), size);
    if (bytes_read != static_cast<size_t>(size)) {
      throw BinaryLoadFromStreamException(size, bytes_read);
    }
  }
};

#define CURRENT_DECLARE_PRIMITIVE_TYPE(unused_typeid_index, cpp_type, unused_current_type) \
  template <>                                                                              \
  struct LoadFromBinaryImpl<cpp_type> {                                                    \
    static void Load(std::istream& istream, cpp_type& destination) {                       \
      LoadPrimitiveTypeFromBinary::Load(istream, destination);                             \
    }                                                                                      \
  };
#include "../primitive_types.dsl.h"
#undef CURRENT_DECLARE_PRIMITIVE_TYPE

template <typename T>
struct LoadFromBinaryImpl {
  struct LoadFieldVisitor {
    std::istream& istream_;

    explicit LoadFieldVisitor(std::istream& istream) : istream_(istream) {}

    template <typename U>
    void operator()(const char*, U& value) const {
      LoadFromBinaryImpl<U>::Load(istream_, value);
    }
  };

  // `CURRENT_STRUCT`.
  template <typename TT = T>
  static ENABLE_IF<IS_CURRENT_STRUCT(TT)> Load(std::istream& istream, T& destination) {
    LoadFieldVisitor visitor(istream);
    current::reflection::VisitAllFields<bricks::decay<T>,
                                        current::reflection::FieldNameAndMutableValue>::WithObject(destination,
                                                                                                   visitor);
  }

  template <typename TT = T>
  static ENABLE_IF<std::is_enum<TT>::value> Load(std::istream& istream, T& destination) {
    using T_UNDERLYING = typename std::underlying_type<T>::type;
    const size_t bytes_read =
        istream.rdbuf()->sgetn(reinterpret_cast<char*>(std::addressof(destination)), sizeof(T_UNDERLYING));
    if (bytes_read != sizeof(T_UNDERLYING)) {
      throw BinaryLoadFromStreamException(sizeof(T_UNDERLYING), bytes_read);
    }
  }
};

template <typename T>
struct LoadFromBinaryImpl<std::vector<T>> {
  static void Load(std::istream& istream, std::vector<T>& destination) {
    BINARY_FORMAT_SIZE_TYPE size = LoadSizeFromBinary(istream);
    destination.resize(static_cast<size_t>(size));
    for (size_t i = 0; i < static_cast<size_t>(size); ++i) {
      LoadFromBinaryImpl<T>::Load(istream, destination[i]);
    }
  }
};

template <typename TF, typename TS>
struct LoadFromBinaryImpl<std::pair<TF, TS>> {
  static void Load(std::istream& istream, std::pair<TF, TS>& destination) {
    LoadFromBinaryImpl<TF>::Load(istream, destination.first);
    LoadFromBinaryImpl<TS>::Load(istream, destination.second);
  }
};

template <typename TK, typename TV>
struct LoadFromBinaryImpl<std::map<TK, TV>> {
  static void Load(std::istream& istream, std::map<TK, TV>& destination) {
    BINARY_FORMAT_SIZE_TYPE size = LoadSizeFromBinary(istream);
    std::pair<TK, TV> entry;
    for (size_t i = 0; i < static_cast<size_t>(size); ++i) {
      LoadFromBinaryImpl<TK>::Load(istream, entry.first);
      LoadFromBinaryImpl<TV>::Load(istream, entry.second);
      destination.insert(entry);
    }
  }
};

template <typename T>
inline T LoadFromBinary(std::istream& istream) {
  T result;
  LoadFromBinaryImpl<T>::Load(istream, result);
  return result;
}

}  // namespace serialization
}  // namespace current

using current::serialization::SaveIntoBinary;
using current::serialization::LoadFromBinary;
using current::serialization::BinarySaveToStreamException;
using current::serialization::BinaryLoadFromStreamException;

#endif  // CURRENT_TYPE_SYSTEM_SERIALIZATION_BINARY_H
