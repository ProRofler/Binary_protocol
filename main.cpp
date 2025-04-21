#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>

using Id = uint64_t;
using Buffer = std::vector<std::byte>;

enum class TypeId : Id {
    Uint,
    Float,
    String,
    Vector
};

class IntegerType {
public:
    template<typename ...Args>
    IntegerType(Args&& ...);
};

class FloatType {
public:
    template<typename ...Args>
    FloatType(Args&& ...);
};

class StringType {
public:
    template<typename ...Args>
    StringType(Args&& ...);
};

class VectorType {
public:
    template<typename ...Args>
    VectorType(Args&& ...);

    template<typename Arg>
    void push_back(Arg&& _val);
};

class Any {
public:
    template<typename ...Args>
    Any(Args&& ...);

    void serialize(Buffer& _buff) const;

    Buffer::const_iterator deserialize(Buffer::const_iterator _begin, Buffer::const_iterator _end);

    TypeId getPayloadTypeId() const;

    template<typename Type>
    auto& getValue() const;

    template<TypeId kId>
    auto& getValue() const;

    bool operator == (const Any& _o) const;
};

class Serializator {
public:
    template<typename Arg>
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