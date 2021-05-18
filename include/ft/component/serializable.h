// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_COMPONENT_SERIALIZABLE_H_
#define FT_INCLUDE_FT_COMPONENT_SERIALIZABLE_H_

#include <algorithm>
#include <stdexcept>
#include <string>

#include "cereal/archives/binary.hpp"
#include "cereal/cereal.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/unordered_map.hpp"

namespace cereal {

class BinaryStringOutputArchive
    : public OutputArchive<BinaryStringOutputArchive, AllowEmptyClassElision> {
 public:
  explicit BinaryStringOutputArchive(std::string* output)
      : OutputArchive<BinaryStringOutputArchive, AllowEmptyClassElision>(this), output_(output) {
    if (!output_) {
      throw std::runtime_error("BinaryStringOutputArchive::saveBinary: failed to write to output");
    }
    output_->clear();
  }

  ~BinaryStringOutputArchive() CEREAL_NOEXCEPT = default;

  //! Writes size bytes of data to the output stream
  void saveBinary(const void* data, std::streamsize size) {
    output_->append(reinterpret_cast<const char*>(data), static_cast<int>(size));
  }

 private:
  std::string* output_;
};

class BinaryStringInputArchive
    : public InputArchive<BinaryStringInputArchive, AllowEmptyClassElision> {
 public:
  //! Construct, loading from the provided stream
  explicit BinaryStringInputArchive(const std::string& data)
      : BinaryStringInputArchive(data.data(), data.size()) {}

  BinaryStringInputArchive(const char* data, std::size_t size)
      : InputArchive<BinaryStringInputArchive, AllowEmptyClassElision>(this),
        data_(data),
        size_(size) {
    if (!data_) {
      throw std::runtime_error("BinaryStringInputArchive::loadBinary: failed to read from input");
    }
  }

  ~BinaryStringInputArchive() CEREAL_NOEXCEPT = default;

  //! Reads size bytes of data from the input stream
  void loadBinary(void* const data, std::streamsize size) {
    if (pos_ + static_cast<std::size_t>(size) > size_) {
      throw std::runtime_error("BinaryStringInputArchive::loadBinary: failed to read from input");
    }
    // TODO(Kevin): 消除无用的拷贝
    std::copy(data_ + pos_, data_ + pos_ + size, reinterpret_cast<char*>(data));
    pos_ += size;
  }

 private:
  const char* data_;
  std::size_t size_ = 0;
  std::size_t pos_ = 0;
};

template <class T>
inline typename std::enable_if<std::is_arithmetic<T>::value, void>::type CEREAL_SAVE_FUNCTION_NAME(
    BinaryStringOutputArchive& ar, T const& t) {
  ar.saveBinary(std::addressof(t), sizeof(t));
}

//! Loading for POD types from binary
template <class T>
inline typename std::enable_if<std::is_arithmetic<T>::value, void>::type CEREAL_LOAD_FUNCTION_NAME(
    BinaryStringInputArchive& ar, T& t) {
  ar.loadBinary(std::addressof(t), sizeof(t));
}

//! Serializing NVP types to binary
template <class Archive, class T>
inline CEREAL_ARCHIVE_RESTRICT(BinaryStringInputArchive, BinaryStringOutputArchive)
    CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar, NameValuePair<T>& t) {
  ar(t.value);
}

//! Serializing SizeTags to binary
template <class Archive, class T>
inline CEREAL_ARCHIVE_RESTRICT(BinaryStringInputArchive, BinaryStringOutputArchive)
    CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar, SizeTag<T>& t) {
  ar(t.size);
}

//! Saving binary data
template <class T>
inline void CEREAL_SAVE_FUNCTION_NAME(BinaryStringOutputArchive& ar, BinaryData<T> const& bd) {
  ar.saveBinary(bd.data, static_cast<std::streamsize>(bd.size));
}

//! Loading binary data
template <class T>
inline void CEREAL_LOAD_FUNCTION_NAME(BinaryStringInputArchive& ar, BinaryData<T>& bd) {
  ar.loadBinary(bd.data, static_cast<std::streamsize>(bd.size));
}

}  // namespace cereal

CEREAL_REGISTER_ARCHIVE(cereal::BinaryStringInputArchive);
CEREAL_REGISTER_ARCHIVE(cereal::BinaryStringOutputArchive);
CEREAL_SETUP_ARCHIVE_TRAITS(cereal::BinaryStringInputArchive, cereal::BinaryStringOutputArchive);

namespace ft::pubsub {

template <class Derived>
class Serializable {
 public:
  Serializable() {}

  void SerializeToString(std::string* output) const {
    cereal::BinaryStringOutputArchive serializer(output);
    serializer(*static_cast<const Derived*>(this));
  }

  void ParseFromString(const char* data, std::size_t size) {
    cereal::BinaryStringInputArchive deserializer(data, size);
    deserializer(*static_cast<Derived*>(this));
  }

  void ParseFromString(const std::string& data) { ParseFromString(data.data(), data.size()); }
};

#define SERIALIZABLE_FIELDS(...) \
  template <class Archive>       \
  void serialize(Archive& ar) {  \
    ar(__VA_ARGS__);             \
  }

}  // namespace ft::pubsub

#endif  // FT_INCLUDE_FT_COMPONENT_SERIALIZABLE_H_
