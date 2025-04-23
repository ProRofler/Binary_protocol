# Serializer

A C++17 implementation of a strict serialization and deserialization system for a set of custom types: `IntegerType`, `FloatType`, `StringType`, `VectorType`, and `Any`.

All types serialize to a binary format using **little-endian** byte order. The implementation follows strict architectural constraints, including **no heap allocations**, **no virtual functions**, and **no value duplication across types**.

### Description
The program expects a binary input file raw.bin to be present in the current directory. It will:
- Read and deserialize the binary content
- Serialize the result again
- Compare the original buffer and the re-serialized buffer.
- Print 1 if they match, otherwise 0.

## Features

- Supports nested structures (e.g., `VectorType` of `VectorType`)
- Compact, portable binary format
- Single `Any` wrapper type for all supported types
- Compile-time type checks for serialization/deserialization
- No dynamic memory usage (heap)

### Supported `TypeId`s

| Type        | ID     | Format                                 |
|-------------|--------|----------------------------------------|
| IntegerType | `0`    | `uint64_t`                             |
| FloatType   | `1`    | `double`                               |
| StringType  | `2`    | `uint64_t` length + raw bytes          |
| VectorType  | `3`    | `uint64_t` count + list of serialized values |

### Requirements

- C++17 compiler (e.g., `g++`, `clang++`, MSVC)
- CMake

### Build With CMake
```
mkdir build
cd build
cmake ..
make
```

### Usage
Run the compiled binary:
```
./Binary_protocol
```
