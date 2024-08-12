// from https://github.com/telecomadm1145/CasioEmuMsvc
// g++ conversion by ChatGPT

#ifndef BINARY_H
#define BINARY_H

#include <fstream>
#include <stdexcept>
#include <vector>
#include <map>
#include <type_traits>
#include <concepts>
#include <cstring>

template <class T>
concept trivial = std::is_trivial_v<T>;

template <class T>
concept trivial_pair = requires {
    typename T::first_type;
    typename T::second_type;
    trivial<typename T::first_type>;
    trivial<typename T::second_type>;
};

template <class T>
concept BinaryData = trivial<T> || trivial_pair<T>;

template <class T>
concept BinaryClass =
    requires(T& t, std::ostream& os, std::istream& is) {
        { t.Write(os) } -> std::same_as<void>;
        { t.Read(is) } -> std::same_as<void>;
    };

template <class T>
using ContainerChild = std::remove_cvref_t<decltype(*std::declval<T>().cbegin())>;

template <class T>
concept BinaryVector =
    requires(T v, typename T::value_type& val, size_t sz) {
        // Needs size operation
        { v.size() } -> std::convertible_to<std::size_t>;
        { v.reserve(sz) } -> std::same_as<void>;

        // Needs const iterators
        { v.cbegin() } -> std::same_as<typename T::const_iterator>;
        { v.cend() } -> std::same_as<typename T::const_iterator>;

        { v.push_back(val) } -> std::same_as<void>;
    };

template <class T>
concept BinaryMap =
    requires(T m) {
        // Needs key-value pair type
        typename T::key_type;
        typename T::mapped_type;
        typename T::value_type;

        // Needs const iterators
        { m.cbegin() } -> std::same_as<typename T::const_iterator>;
        { m.cend() } -> std::same_as<typename T::const_iterator>;
    };

/// <summary>
/// Provides structured binary and STL conversion
/// </summary>
class Binary {
public:
    static void Write(std::ostream& stm, const BinaryData auto& dat) {
        stm.write(reinterpret_cast<const char*>(&dat), sizeof(dat));
#ifdef _BIN_DBG
        stm.flush();
#endif //  _BIN_DBG
    }

    static void Read(std::istream& stm, BinaryData auto& dat) {
        stm.read(reinterpret_cast<char*>(&dat), sizeof(dat));
    }

    static void Write(std::ostream& stm, const BinaryClass auto& cls) {
        cls.Write(stm);
    }

    static void Read(std::istream& stm, BinaryClass auto& cls) {
        cls.Read(stm);
    }

    static void Read(std::istream& stm, BinaryVector auto& vec) {
        using ChildType = ContainerChild<decltype(vec)>;
        unsigned long long size = 0;
        Read(stm, size);
        if (size > 1ULL << 48) {
            __builtin_trap();
        }
        vec.reserve(size);
        for (size_t i = 0; i < size; i++) {
            if (stm.eof())
                return;
            ChildType data{};
            Read(stm, data);
            vec.push_back(data);
        }
    }

    static void Write(std::ostream& stm, const BinaryVector auto& vec) {
        unsigned long long sz = vec.size();
        Write(stm, sz);
        for (const auto& data : vec) {
            Write(stm, data);
            sz--;
        }
        if (sz != 0)
            __builtin_trap();
    }

    static void Read(std::istream& stm, BinaryMap auto& map) {
        using ChildType = ContainerChild<decltype(map)>;
        unsigned long long size = 0;
        Read(stm, size);
        if (size > 1ULL << 48) {
            __builtin_trap();
        }
        for (size_t i = 0; i < size; i++) {
            if (stm.eof())
                return;
            std::remove_cvref_t<typename ChildType::first_type> key{};
            Read(stm, key);
            std::remove_cvref_t<typename ChildType::second_type> val{};
            Read(stm, val);
            map[key] = val;
        }
    }

    static void Write(std::ostream& stm, const BinaryMap auto& map) {
        unsigned long long sz = map.size();
        Write(stm, sz);
        for (const auto& kv : map) {
            Write(stm, kv.first);
            Write(stm, kv.second);
            sz--;
        }
        if (sz != 0)
            __builtin_trap();
    }
};

#endif // BINARY_H
