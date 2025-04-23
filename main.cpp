#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <type_traits>
#include <variant>
#include <vector>

using Id = uint64_t;
using Buffer = std::vector<std::byte>;

enum class TypeId : Id { Uint, Float, String, Vector };

namespace helper {

template <typename T>
void write_le(Buffer& out, const T& val) {
    static_assert(std::is_trivially_copyable_v<T>);
    std::array<std::byte, sizeof(T)> tmp;
    std::memcpy(tmp.data(), &val, sizeof(T));
    for (std::byte b : tmp) out.push_back(b);
}

template <typename T>
T read_le(Buffer::const_iterator& it) {
    static_assert(std::is_trivially_copyable_v<T>);
    T val{};
    std::memcpy(&val, &*it, sizeof(T));
    it += sizeof(T);
    return val;
}

}  // namespace helper

template <typename T, TypeId kId>
class ValueBase {
   protected:
    T value_;

   public:
    using value_type = T;
    static constexpr TypeId type_id = kId;

    ValueBase() = default;

    template <typename Arg,
              typename = std::enable_if_t<std::is_constructible_v<T, Arg&&>>>
    ValueBase(Arg&& val) : value_(std::forward<Arg>(val)) {}

    void serialize(Buffer& buff) const {
        helper::write_le<Id>(buff, static_cast<Id>(kId));
        if constexpr (std::is_same_v<T, std::string>) {
            helper::write_le<Id>(buff, value_.size());
            for (char ch : value_) buff.push_back(static_cast<std::byte>(ch));
        } else {
            helper::write_le<T>(buff, value_);
        }
    }

    Buffer::const_iterator deserialize(Buffer::const_iterator it,
                                       Buffer::const_iterator end) {
        if (it + sizeof(Id) > end) return end;
        Id read_id = helper::read_le<Id>(it);
        if (read_id != static_cast<Id>(kId)) return end;

        if constexpr (std::is_same_v<T, std::string>) {
            Id sz = helper::read_le<Id>(it);
            if (it + sz > end) return end;
            value_.assign(reinterpret_cast<const char*>(&*it), sz);
            it += sz;
        } else {
            value_ = helper::read_le<T>(it);
        }
        return it;
    }

    const T& get() const { return value_; }

    bool operator==(const ValueBase& other) const {
        return value_ == other.value_;
    }

    static constexpr TypeId getId() { return kId; }
};

class IntegerType final : public ValueBase<uint64_t, TypeId::Uint> {
   public:
    using Base = ValueBase<uint64_t, TypeId::Uint>;
    using Base::ValueBase;
};

class FloatType final : public ValueBase<double, TypeId::Float> {
   public:
    using Base = ValueBase<double, TypeId::Float>;
    using Base::ValueBase;
};

class StringType final : public ValueBase<std::string, TypeId::String> {
   public:
    using Base = ValueBase<std::string, TypeId::String>;
    using Base::ValueBase;
};

class Any;
class VectorType final : public ValueBase<std::vector<Any>, TypeId::Vector> {
   public:
    using Base = ValueBase<std::vector<Any>, TypeId::Vector>;
    using Base::ValueBase;

    VectorType() = default;

    template <typename... Args,
              typename = std::enable_if_t<
                  (std::conjunction_v<std::disjunction<
                       std::is_same<std::decay_t<Args>, IntegerType>,
                       std::is_same<std::decay_t<Args>, FloatType>,
                       std::is_same<std::decay_t<Args>, StringType>,
                       std::is_same<std::decay_t<Args>, VectorType>>...>)>>
    VectorType(Args&&... args) {
        (push_back(std::forward<Args>(args)), ...);
    }

    template <typename Arg>
    void push_back(Arg&& val) {
        if constexpr (std::is_same_v<std::decay_t<Arg>, IntegerType> ||
                      std::is_same_v<std::decay_t<Arg>, FloatType> ||
                      std::is_same_v<std::decay_t<Arg>, StringType> ||
                      std::is_same_v<std::decay_t<Arg>, VectorType>) {
            Base::value_.emplace_back(std::forward<Arg>(val));
        }
    }

    void serialize(Buffer& buff) const;

    Buffer::const_iterator deserialize(Buffer::const_iterator it,
                                       Buffer::const_iterator end);
};

class Any final {
   public:
    using Variant =
        std::variant<IntegerType, FloatType, StringType, VectorType>;

    Any() = default;

    template <typename T, typename = std::enable_if_t<std::disjunction_v<
                              std::is_same<std::decay_t<T>, IntegerType>,
                              std::is_same<std::decay_t<T>, FloatType>,
                              std::is_same<std::decay_t<T>, StringType>,
                              std::is_same<std::decay_t<T>, VectorType>>>>
    Any(T&& val) : value_(std::forward<T>(val)) {}

    void serialize(Buffer& buff) const {
        std::visit([&buff](const auto& val) { val.serialize(buff); }, value_);
    }

    Buffer::const_iterator deserialize(Buffer::const_iterator it,
                                       Buffer::const_iterator end) {
        if (std::distance(it, end) < sizeof(Id)) return end;
        Id type_id = helper::read_le<Id>(it);

        switch (static_cast<TypeId>(type_id)) {
            case TypeId::Uint: {
                IntegerType v;
                it = v.deserialize(it, end);
                value_ = std::move(v);
                break;
            }
            case TypeId::Float: {
                FloatType v;
                it = v.deserialize(it, end);
                value_ = std::move(v);
                break;
            }
            case TypeId::String: {
                StringType v;
                it = v.deserialize(it, end);
                value_ = std::move(v);
                break;
            }
            case TypeId::Vector: {
                VectorType v;
                it = v.deserialize(it, end);
                value_ = std::move(v);
                break;
            }
            default:
                throw std::runtime_error(
                    "Unknown TypeId during deserialization");
        }
        return it;
    }

    template <typename T>
    const T& getValue() const {
        return std::get<T>(value_);
    }

    template <TypeId kId>
    const auto& getValue() const {
        if constexpr (kId == TypeId::Uint) return std::get<IntegerType>(value_);
        if constexpr (kId == TypeId::Float) return std::get<FloatType>(value_);
        if constexpr (kId == TypeId::String)
            return std::get<StringType>(value_);
        if constexpr (kId == TypeId::Vector)
            return std::get<VectorType>(value_);
    }

    TypeId getPayloadTypeId() const {
        return static_cast<TypeId>(value_.index());
    }

    bool operator==(const Any& other) const { return value_ == other.value_; }

   private:
    Variant value_;
};

class Serializer {
   private:
    std::vector<Any> data_;

   public:
    template <typename Arg>
    void push(Arg&& _val) {
        if constexpr (std::is_same_v<std::decay_t<Arg>, IntegerType> ||
                      std::is_same_v<std::decay_t<Arg>, FloatType> ||
                      std::is_same_v<std::decay_t<Arg>, StringType> ||
                      std::is_same_v<std::decay_t<Arg>, VectorType> ||
                      std::is_same_v<std::decay_t<Arg>, Any>) {
            data_.emplace_back(std::forward<Arg>(_val));
        }
    }

    Buffer serialize() const {
        Buffer buff;
        helper::write_le<Id>(buff, data_.size());
        for (const auto& el : data_) el.serialize(buff);
        return buff;
    }

    static std::vector<Any> deserialize(const Buffer& _val) {
        auto it = _val.begin();
        auto end = _val.end();
        Id sz = helper::read_le<Id>(it);

        std::vector<Any> result;
        result.reserve(sz);
        for (Id i = 0; i < sz; ++i) {
            Any val;
            it = val.deserialize(it, end);
            result.push_back(std::move(val));
        }
        return result;
    }

    const std::vector<Any>& getStorage() const { return data_; }
};

int main() {
    std::ifstream raw;
    raw.open("raw.bin", std::ios_base::in | std::ios_base::binary);
    if (!raw.is_open()) return 1;
    raw.seekg(0, std::ios_base::end);
    std::streamsize size = raw.tellg();
    raw.seekg(0, std::ios_base::beg);

    Buffer buff(size);
    raw.read(reinterpret_cast<char*>(buff.data()), size);

    auto res = Serializer::deserialize(buff);

    Serializer s;
    for (auto&& i : res) s.push(i);

    std::cout << (buff == s.serialize()) << '\n';

    return 0;
}

// Definitions for VectorType which requires full Any class
void VectorType::serialize(Buffer& buff) const {
    helper::write_le<Id>(buff, static_cast<Id>(TypeId::Vector));
    helper::write_le<Id>(buff, Base::value_.size());
    for (const auto& el : Base::value_) {
        el.serialize(buff);
    }
}

Buffer::const_iterator VectorType::deserialize(Buffer::const_iterator it,
                                               Buffer::const_iterator end) {
    if (it + sizeof(Id) * 2 > end) return end;
    Id read_id = helper::read_le<Id>(it);
    if (read_id != static_cast<Id>(TypeId::Vector)) return end;

    Id sz = helper::read_le<Id>(it);
    Base::value_.clear();
    Base::value_.reserve(sz);

    for (Id i = 0; i < sz; ++i) {
        Any val;
        it = val.deserialize(it, end);
        Base::value_.push_back(std::move(val));
    }
    return it;
}
// end of VectorType definitions