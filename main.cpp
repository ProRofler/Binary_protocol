#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <type_traits>
#include <vector>

using Id = uint64_t;
using Buffer = std::vector<std::byte>;

enum class TypeId : Id { Uint, Float, String, Vector };

// helper для чтения и записи в  little-endian
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

template<typename T, TypeId kId>
class ValueBase {
protected:
    T value_;

public:
    using value_type = T;
    static constexpr TypeId type_id = kId;

    ValueBase() = default;

    template<typename Arg,
             typename = std::enable_if_t<std::is_constructible_v<T, Arg&&>>>
    ValueBase(Arg&& val) : value_(std::forward<Arg>(val)) {}

    void serialize(Buffer& buff) const {
        helper::write_le<Id>(buff, static_cast<Id>(kId));
        if constexpr (std::is_same_v<T, std::string>) {
            helper::write_le<Id>(buff, value_.size());
            for (char ch : value_)
                buff.push_back(static_cast<std::byte>(ch));
        } else {
            helper::write_le<T>(buff, value_);
        }
    }

    Buffer::const_iterator deserialize(Buffer::const_iterator it, Buffer::const_iterator end) {
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

    static constexpr TypeId getId() {
        return kId;
    }
};



class IntegerType {
   public:
    template <typename... Args>
    IntegerType(Args&&...);
};

class FloatType {
   public:
    template <typename... Args>
    FloatType(Args&&...);
};

class StringType {
   public:
    template <typename... Args>
    StringType(Args&&...);
};

class VectorType {
   public:
    template <typename... Args>
    VectorType(Args&&...);

    template <typename Arg>
    void push_back(Arg&& _val);
};

class Any {
   public:
    template <typename... Args>
    Any(Args&&...);

    void serialize(Buffer& _buff) const;

    Buffer::const_iterator deserialize(Buffer::const_iterator _begin,
                                       Buffer::const_iterator _end);

    TypeId getPayloadTypeId() const;

    template <typename Type>
    auto& getValue() const;

    template <TypeId kId>
    auto& getValue() const;

    bool operator==(const Any& _o) const;
};

class Serializator {
   public:
    template <typename Arg>
    void push(Arg&& _val);

    Buffer serialize() const;

    static std::vector<Any> deserialize(const Buffer& _val);

    const std::vector<Any>& getStorage() const;
};

int main() {
    // std::ifstream raw;
    // raw.open("raw.bin", std::ios_base::in | std::ios_base::binary);
    // if (!raw.is_open())
    //     return 1;
    // raw.seekg(0, std::ios_base::end);
    // std::streamsize size = raw.tellg();
    // raw.seekg(0, std::ios_base::beg);

    // Buffer buff(size);
    // raw.read(reinterpret_cast<char*>(buff.data()), size);

    // auto res = Serializator::deserialize(buff);

    // Serializator s;
    // for (auto&& i : res)
    //     s.push(i);

    // std::cout << (buff == s.serialize()) << '\n';

    return 0;
}