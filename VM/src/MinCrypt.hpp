#pragma once

#include <cstddef>
#include <array>
#include <string>
#include <string_view>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <cstdint>

namespace MinCrypt {

template <size_t N>
struct FixedString {
    constexpr FixedString() = default;
    constexpr FixedString(const char (&str)[N]) {
        for (size_t i = 0; i < N; ++i) value[i] = str[i];
    }
    char value[N]{};
    static constexpr size_t size = N;
};

template <size_t N>
constexpr FixedString<N> encrypt_string(const char (&str)[N]) {
    FixedString<N> enc{};
    for (size_t i = 0; i < N; ++i) {
        enc.value[i] = static_cast<char>(256 - static_cast<int>(str[i]));
    }
    return enc;
}

template <size_t N>
__declspec(noinline) void decrypt_runtime(const char* src, char* dst) {
    volatile int key = 256;
    for (size_t i = 0; i < N; ++i) {
        dst[i] = static_cast<char>(key - static_cast<int>(src[i]));
    }
}

template <size_t N>
struct uintptr_array {
    uintptr_t value[N]{};
    static constexpr size_t size = N;
};

template <size_t CharSize>
constexpr auto get_uintptr_encrypted(const char* encrypted_data) {
    constexpr size_t word_size = sizeof(uintptr_t);
    constexpr size_t array_size = (CharSize + word_size - 1) / word_size;
    uintptr_array<array_size> result{};

    for (size_t i = 0; i < CharSize; ++i) {
        size_t word_idx = i / word_size;
        size_t byte_idx = i % word_size;
        result.value[word_idx] |= (static_cast<uintptr_t>(static_cast<unsigned char>(encrypted_data[i])) << (byte_idx * 8));
    }
    return result;
}

template <FixedString EncryptedStr>
class StackDecryptedString {
public:
    StackDecryptedString() {
        decrypt_runtime<EncryptedStr.size>(EncryptedStr.value, decrypted_data_.data());
    }

    const char* get_data() const { return decrypted_data_.data(); }
    std::string_view get() const { return std::string_view(decrypted_data_.data(), EncryptedStr.size - 1); }

    operator std::string_view() const { return get(); }
    operator const char*() const { return get_data(); }

private:
    std::array<char, EncryptedStr.size> decrypted_data_{};
};

template <FixedString EncryptedStr>
class StackCodeDecryptedString {
    static constexpr size_t word_size = sizeof(uintptr_t);
    static constexpr size_t padded_size = ((EncryptedStr.size + word_size - 1) / word_size) * word_size;

public:
    __forceinline StackCodeDecryptedString() {
        constexpr auto uintptr_data = get_uintptr_encrypted<EncryptedStr.size>(EncryptedStr.value);

        volatile uintptr_t* dest = reinterpret_cast<volatile uintptr_t*>(decrypted_data_.data());
        [&]<size_t... Is>(std::index_sequence<Is...>) {
            ((dest[Is] = uintptr_data.value[Is]), ...);
        }(std::make_index_sequence<uintptr_data.size>{});

        volatile int key = 256;
        for (size_t i = 0; i < EncryptedStr.size; ++i) {
            decrypted_data_[i] = static_cast<char>(key - static_cast<int>(decrypted_data_[i]));
        }
    }

    const char* get_data() const { return decrypted_data_.data(); }
    std::string_view get() const { return std::string_view(decrypted_data_.data(), EncryptedStr.size - 1); }

    operator std::string_view() const { return get(); }
    operator const char*() const { return get_data(); }

private:
    alignas(uintptr_t) std::array<char, padded_size> decrypted_data_{};
};

} // namespace MinCrypt

#define MINCRYPT_STACK(str) (::MinCrypt::StackDecryptedString<::MinCrypt::encrypt_string(str)>())

#define MINCRYPT_STACK_CODE(str) (::MinCrypt::StackCodeDecryptedString<::MinCrypt::encrypt_string(str)>())

#define MINCRYPT(str) MINCRYPT_STACK_CODE(str).get_data()

#define MINCRYPT_LAZY(str) []() { static const auto s = []{ std::string t = MINCRYPT(str); t.reserve(16); return t; }(); return s.c_str(); }

using GetStrFunc = const char* (*)();

// -- LUAU --//

#if defined(LUAU_FASTFLAGVARIABLE)

#define LUAU_FASTFLAGVARIABLE_CRYPT(flag) \
    namespace FFlag { Luau::FValue<bool> flag(MINCRYPT_LAZY(#flag)(), false, false); } \
    static Luau::FValueVersionSetter flag##_VersionSetter(MINCRYPT_LAZY(#flag)(), 2);

#define LUAU_FASTINTVARIABLE_CRYPT(flag, def) \
    namespace FInt { Luau::FValue<int> flag(MINCRYPT_LAZY(#flag)(), def, false); } \
    static Luau::FValueVersionSetter flag##_VersionSetter(MINCRYPT_LAZY(#flag)(), 2);

#define LUAU_DYNAMIC_FASTFLAGVARIABLE_CRYPT(flag, def) \
    namespace DFFlag { Luau::FValue<bool> flag(MINCRYPT_LAZY(#flag)(), def, true); } \
    static Luau::FValueVersionSetter flag##_VersionSetter(MINCRYPT_LAZY(#flag)(), 2);

#endif
