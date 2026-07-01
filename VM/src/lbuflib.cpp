// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "lualib.h"

#include "lcommon.h"
#include "lbuffer.h"
#include "MinCrypt.hpp"

#if defined(LUAU_BIG_ENDIAN)
#include <endian.h>
#endif

LUAU_FASTFLAG(LuauIntegerLibrary)

#include <string.h>

// while C API returns 'size_t' for binary compatibility in case of future extensions,
// in the current implementation, length and offset are limited to 31 bits
// because offset is limited to an integer, a single 64bit comparison can be used and will not overflow
#define isoutofbounds(offset, len, accessize) (uint64_t(unsigned(offset)) + (accessize) > uint64_t(len))

static_assert(MAX_BUFFER_SIZE <= INT_MAX, "current implementation can't handle a larger limit");

#if defined(LUAU_BIG_ENDIAN)
template<typename T>
inline T buffer_swapbe(T v)
{
    if (sizeof(T) == 8)
        return htole64(v);
    else if (sizeof(T) == 4)
        return htole32(v);
    else if (sizeof(T) == 2)
        return htole16(v);
    else
        return v;
}
#endif

static int buffer_create(lua_State* L)
{
    int size = luaL_checkinteger(L, 1);

    luaL_argcheck(L, size >= 0, 1, "size");

    lua_newbuffer(L, size);
    return 1;
}

static int buffer_fromstring(lua_State* L)
{
    size_t len = 0;
    const char* val = luaL_checklstring(L, 1, &len);

    void* data = lua_newbuffer(L, len);
    memcpy(data, val, len);
    return 1;
}

static int buffer_tostring(lua_State* L)
{
    size_t len = 0;
    void* data = luaL_checkbuffer(L, 1, &len);

    lua_pushlstring(L, (char*)data, len);
    return 1;
}

template<typename T>
static int buffer_readinteger(lua_State* L)
{
    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int offset = luaL_checkinteger(L, 2);

    if (isoutofbounds(offset, len, sizeof(T)))
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    T val;
    memcpy(&val, (char*)buf + offset, sizeof(T));

#if defined(LUAU_BIG_ENDIAN)
    val = buffer_swapbe(val);
#endif

    lua_pushnumber(L, double(val));
    return 1;
}

template<typename T>
static int buffer_writeinteger(lua_State* L)
{
    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int offset = luaL_checkinteger(L, 2);
    int value = luaL_checkunsigned(L, 3);

    if (isoutofbounds(offset, len, sizeof(T)))
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    T val = T(value);

#if defined(LUAU_BIG_ENDIAN)
    val = buffer_swapbe(val);
#endif

    memcpy((char*)buf + offset, &val, sizeof(T));
    return 0;
}

static int buffer_readlong(lua_State* L)
{
    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int offset = luaL_checkinteger(L, 2);

    if (isoutofbounds(offset, len, sizeof(uint64_t)))
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    int64_t val;
    memcpy(&val, (char*)buf + offset, sizeof(int64_t));

#if defined(LUAU_BIG_ENDIAN)
    val = buffer_swapbe(val);
#endif

    lua_pushinteger64(L, val);
    return 1;
}

static int buffer_writelong(lua_State* L)
{
    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int offset = luaL_checkinteger(L, 2);
    int64_t value = luaL_checkinteger64(L, 3);

    if (isoutofbounds(offset, len, sizeof(int64_t)))
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

#if defined(LUAU_BIG_ENDIAN)
    value = buffer_swapbe(value);
#endif

    memcpy((char*)buf + offset, &value, sizeof(int64_t));
    return 0;
}

template<typename T, typename StorageType>
static int buffer_readfp(lua_State* L)
{
    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int offset = luaL_checkinteger(L, 2);

    if (isoutofbounds(offset, len, sizeof(T)))
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    T val;

#if defined(LUAU_BIG_ENDIAN)
    static_assert(sizeof(T) == sizeof(StorageType), "type size must match to reinterpret data");
    StorageType tmp;
    memcpy(&tmp, (char*)buf + offset, sizeof(tmp));
    tmp = buffer_swapbe(tmp);

    memcpy(&val, &tmp, sizeof(tmp));
#else
    memcpy(&val, (char*)buf + offset, sizeof(T));
#endif

    lua_pushnumber(L, double(val));
    return 1;
}

template<typename T, typename StorageType>
static int buffer_writefp(lua_State* L)
{
    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int offset = luaL_checkinteger(L, 2);
    double value = luaL_checknumber(L, 3);

    if (isoutofbounds(offset, len, sizeof(T)))
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    T val = T(value);

#if defined(LUAU_BIG_ENDIAN)
    static_assert(sizeof(T) == sizeof(StorageType), "type size must match to reinterpret data");
    StorageType tmp;
    memcpy(&tmp, &val, sizeof(tmp));
    tmp = buffer_swapbe(tmp);

    memcpy((char*)buf + offset, &tmp, sizeof(tmp));
#else
    memcpy((char*)buf + offset, &val, sizeof(T));
#endif

    return 0;
}

static int buffer_readstring(lua_State* L)
{
    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int offset = luaL_checkinteger(L, 2);
    int size = luaL_checkinteger(L, 3);

    luaL_argcheck(L, size >= 0, 3, "size");

    if (isoutofbounds(offset, len, unsigned(size)))
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    lua_pushlstring(L, (char*)buf + offset, size);
    return 1;
}

static int buffer_writestring(lua_State* L)
{
    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int offset = luaL_checkinteger(L, 2);
    size_t size = 0;
    const char* val = luaL_checklstring(L, 3, &size);
    int count = luaL_optinteger(L, 4, int(size));

    luaL_argcheck(L, count >= 0, 4, MINCRYPT_LAZY("count")());

    if (size_t(count) > size)
        luaL_error(L, MINCRYPT("string length overflow"));

    // string size can't exceed INT_MAX at this point
    if (isoutofbounds(offset, len, unsigned(count)))
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    memcpy((char*)buf + offset, val, count);
    return 0;
}

static int buffer_len(lua_State* L)
{
    size_t len = 0;
    luaL_checkbuffer(L, 1, &len);

    lua_pushnumber(L, double(unsigned(len)));
    return 1;
}

static int buffer_copy(lua_State* L)
{
    size_t tlen = 0;
    void* tbuf = luaL_checkbuffer(L, 1, &tlen);
    int toffset = luaL_checkinteger(L, 2);

    size_t slen = 0;
    void* sbuf = luaL_checkbuffer(L, 3, &slen);
    int soffset = luaL_optinteger(L, 4, 0);

    int size = luaL_optinteger(L, 5, int(slen) - soffset);

    if (size < 0)
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    if (isoutofbounds(soffset, slen, unsigned(size)))
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    if (isoutofbounds(toffset, tlen, unsigned(size)))
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    memmove((char*)tbuf + toffset, (char*)sbuf + soffset, size);
    return 0;
}

static int buffer_fill(lua_State* L)
{
    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int offset = luaL_checkinteger(L, 2);
    unsigned value = luaL_checkunsigned(L, 3);
    int size = luaL_optinteger(L, 4, int(len) - offset);

    if (size < 0)
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    if (isoutofbounds(offset, len, unsigned(size)))
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    memset((char*)buf + offset, value & 0xff, size);
    return 0;
}

static int buffer_readbits(lua_State* L)
{
    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int64_t bitoffset = (int64_t)luaL_checknumber(L, 2);
    int bitcount = luaL_checkinteger(L, 3);

    if (bitoffset < 0)
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    if (unsigned(bitcount) > 32)
        luaL_error(L, MINCRYPT("bit count is out of range of [0; 32]"));

    if (uint64_t(bitoffset + bitcount) > uint64_t(len) * 8)
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    unsigned startbyte = unsigned(bitoffset / 8);
    unsigned endbyte = unsigned((bitoffset + bitcount + 7) / 8);

    uint64_t data = 0;

#if defined(LUAU_BIG_ENDIAN)
    for (int i = int(endbyte) - 1; i >= int(startbyte); i--)
        data = (data << 8) + uint8_t(((char*)buf)[i]);
#else
    memcpy(&data, (char*)buf + startbyte, endbyte - startbyte);
#endif

    uint64_t subbyteoffset = bitoffset & 0x7;
    uint64_t mask = (1ull << bitcount) - 1;

    lua_pushunsigned(L, unsigned((data >> subbyteoffset) & mask));
    return 1;
}

static int buffer_writebits(lua_State* L)
{
    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int64_t bitoffset = (int64_t)luaL_checknumber(L, 2);
    int bitcount = luaL_checkinteger(L, 3);
    unsigned value = luaL_checkunsigned(L, 4);

    if (bitoffset < 0)
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    if (unsigned(bitcount) > 32)
        luaL_error(L, MINCRYPT("bit count is out of range of [0; 32]"));

    if (uint64_t(bitoffset + bitcount) > uint64_t(len) * 8)
        luaL_error(L, MINCRYPT("buffer access out of bounds"));

    unsigned startbyte = unsigned(bitoffset / 8);
    unsigned endbyte = unsigned((bitoffset + bitcount + 7) / 8);

    uint64_t data = 0;

#if defined(LUAU_BIG_ENDIAN)
    for (int i = int(endbyte) - 1; i >= int(startbyte); i--)
        data = data * 256 + uint8_t(((char*)buf)[i]);
#else
    memcpy(&data, (char*)buf + startbyte, endbyte - startbyte);
#endif

    uint64_t subbyteoffset = bitoffset & 0x7;
    uint64_t mask = ((1ull << bitcount) - 1) << subbyteoffset;

    data = (data & ~mask) | ((uint64_t(value) << subbyteoffset) & mask);

#if defined(LUAU_BIG_ENDIAN)
    for (int i = int(startbyte); i < int(endbyte); i++)
    {
        ((char*)buf)[i] = data & 0xff;
        data >>= 8;
    }
#else
    memcpy((char*)buf + startbyte, &data, endbyte - startbyte);
#endif
    return 0;
}

int luaopen_buffer(lua_State* L)
{
    auto n_create = MINCRYPT_LAZY("create")();
    auto n_fromstring = MINCRYPT_LAZY("fromstring")();
    auto n_tostring = MINCRYPT_LAZY("tostring")();
    auto n_readi8 = MINCRYPT_LAZY("readi8")();
    auto n_readu8 = MINCRYPT_LAZY("readu8")();
    auto n_readi16 = MINCRYPT_LAZY("readi16")();
    auto n_readu16 = MINCRYPT_LAZY("readu16")();
    auto n_readi32 = MINCRYPT_LAZY("readi32")();
    auto n_readu32 = MINCRYPT_LAZY("readu32")();
    auto n_readf32 = MINCRYPT_LAZY("readf32")();
    auto n_readf64 = MINCRYPT_LAZY("readf64")();
    auto n_writei8 = MINCRYPT_LAZY("writei8")();
    auto n_writeu8 = MINCRYPT_LAZY("writeu8")();
    auto n_writei16 = MINCRYPT_LAZY("writei16")();
    auto n_writeu16 = MINCRYPT_LAZY("writeu16")();
    auto n_writei32 = MINCRYPT_LAZY("writei32")();
    auto n_writeu32 = MINCRYPT_LAZY("writeu32")();
    auto n_writef32 = MINCRYPT_LAZY("writef32")();
    auto n_writef64 = MINCRYPT_LAZY("writef64")();
    auto n_readstring = MINCRYPT_LAZY("readstring")();
    auto n_writestring = MINCRYPT_LAZY("writestring")();
    auto n_len = MINCRYPT_LAZY("len")();
    auto n_copy = MINCRYPT_LAZY("copy")();
    auto n_fill = MINCRYPT_LAZY("fill")();
    auto n_readbits = MINCRYPT_LAZY("readbits")();
    auto n_writebits = MINCRYPT_LAZY("writebits")();

    if (FFlag::LuauIntegerLibrary)
    {
        auto n_readinteger = MINCRYPT_LAZY("readinteger")();
        auto n_writeinteger = MINCRYPT_LAZY("writeinteger")();

        luaL_Reg bufferlib[] = {
            {n_create, buffer_create},
            {n_fromstring, buffer_fromstring},
            {n_tostring, buffer_tostring},
            {n_readi8, buffer_readinteger<int8_t>},
            {n_readu8, buffer_readinteger<uint8_t>},
            {n_readi16, buffer_readinteger<int16_t>},
            {n_readu16, buffer_readinteger<uint16_t>},
            {n_readi32, buffer_readinteger<int32_t>},
            {n_readu32, buffer_readinteger<uint32_t>},
            {n_readf32, buffer_readfp<float, uint32_t>},
            {n_readf64, buffer_readfp<double, uint64_t>},
            {n_writei8, buffer_writeinteger<int8_t>},
            {n_writeu8, buffer_writeinteger<uint8_t>},
            {n_writei16, buffer_writeinteger<int16_t>},
            {n_writeu16, buffer_writeinteger<uint16_t>},
            {n_writei32, buffer_writeinteger<int32_t>},
            {n_writeu32, buffer_writeinteger<uint32_t>},
            {n_writef32, buffer_writefp<float, uint32_t>},
            {n_writef64, buffer_writefp<double, uint64_t>},
            {n_readstring, buffer_readstring},
            {n_writestring, buffer_writestring},
            {n_len, buffer_len},
            {n_copy, buffer_copy},
            {n_fill, buffer_fill},
            {n_readbits, buffer_readbits},
            {n_writebits, buffer_writebits},
            {n_readinteger, buffer_readlong},
            {n_writeinteger, buffer_writelong},
            {NULL, NULL},
        };

        luaL_register(L, MINCRYPT(LUA_BUFFERLIBNAME), bufferlib);
    }
    else
    {
        luaL_Reg bufferlib_NOINTEGER[] = {
            {n_create, buffer_create},
            {n_fromstring, buffer_fromstring},
            {n_tostring, buffer_tostring},
            {n_readi8, buffer_readinteger<int8_t>},
            {n_readu8, buffer_readinteger<uint8_t>},
            {n_readi16, buffer_readinteger<int16_t>},
            {n_readu16, buffer_readinteger<uint16_t>},
            {n_readi32, buffer_readinteger<int32_t>},
            {n_readu32, buffer_readinteger<uint32_t>},
            {n_readf32, buffer_readfp<float, uint32_t>},
            {n_readf64, buffer_readfp<double, uint64_t>},
            {n_writei8, buffer_writeinteger<int8_t>},
            {n_writeu8, buffer_writeinteger<uint8_t>},
            {n_writei16, buffer_writeinteger<int16_t>},
            {n_writeu16, buffer_writeinteger<uint16_t>},
            {n_writei32, buffer_writeinteger<int32_t>},
            {n_writeu32, buffer_writeinteger<uint32_t>},
            {n_writef32, buffer_writefp<float, uint32_t>},
            {n_writef64, buffer_writefp<double, uint64_t>},
            {n_readstring, buffer_readstring},
            {n_writestring, buffer_writestring},
            {n_len, buffer_len},
            {n_copy, buffer_copy},
            {n_fill, buffer_fill},
            {n_readbits, buffer_readbits},
            {n_writebits, buffer_writebits},
            {NULL, NULL},
        };

        luaL_register(L, MINCRYPT(LUA_BUFFERLIBNAME), bufferlib_NOINTEGER);
    }

    return 1;
}
