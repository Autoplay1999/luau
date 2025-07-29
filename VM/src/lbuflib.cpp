// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "lualib.h"

#include "lcommon.h"
#include "lbuffer.h"

#if defined(LUAU_BIG_ENDIAN)
#include <endian.h>
#endif

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
    /*size*/ scrypt_def(STR_0, "\x8d\x97\x86\x9b");

    int size = luaL_checkinteger(L, 1);

    luaL_argcheck(L, size >= 0, 1, STR_0->c_str());

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

    if (isoutofbounds(offset, len, sizeof(T))) {
        #define STR_0 /*buffer access out of bounds*/ scrypt("\x9e\x8b\x9a\x9a\x9b\x8e\xe0\x9f\x9d\x9d\x9b\x8d\x8d\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x9e\x91\x8b\x92\x9c\x8d").c_str()
        luaL_error(L, STR_0);
        #undef STR_0
    }

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

    if (isoutofbounds(offset, len, sizeof(T))) {
        #define STR_0 /*buffer access out of bounds*/ scrypt("\x9e\x8b\x9a\x9a\x9b\x8e\xe0\x9f\x9d\x9d\x9b\x8d\x8d\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x9e\x91\x8b\x92\x9c\x8d").c_str()
        luaL_error(L, STR_0);
        #undef STR_0
    }

    T val = T(value);

#if defined(LUAU_BIG_ENDIAN)
    val = buffer_swapbe(val);
#endif

    memcpy((char*)buf + offset, &val, sizeof(T));
    return 0;
}

template<typename T, typename StorageType>
static int buffer_readfp(lua_State* L)
{
    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int offset = luaL_checkinteger(L, 2);

    if (isoutofbounds(offset, len, sizeof(T))) {
        #define STR_0 /*buffer access out of bounds*/ scrypt("\x9e\x8b\x9a\x9a\x9b\x8e\xe0\x9f\x9d\x9d\x9b\x8d\x8d\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x9e\x91\x8b\x92\x9c\x8d").c_str()
        luaL_error(L, STR_0);
        #undef STR_0
    }

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

    if (isoutofbounds(offset, len, sizeof(T))) {
        #define STR_0 /*buffer access out of bounds*/ scrypt("\x9e\x8b\x9a\x9a\x9b\x8e\xe0\x9f\x9d\x9d\x9b\x8d\x8d\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x9e\x91\x8b\x92\x9c\x8d").c_str()
        luaL_error(L, STR_0);
        #undef STR_0
    }

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

    /*size*/ scrypt_def(STR_0, "\x8d\x97\x86\x9b");
    luaL_argcheck(L, size >= 0, 3, STR_0->c_str());

    if (isoutofbounds(offset, len, unsigned(size))) {
        #define STR_1 /*buffer access out of bounds*/ scrypt("\x9e\x8b\x9a\x9a\x9b\x8e\xe0\x9f\x9d\x9d\x9b\x8d\x8d\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x9e\x91\x8b\x92\x9c\x8d").c_str()
        luaL_error(L, STR_1);
        #undef STR_1
    }

    lua_pushlstring(L, (char*)buf + offset, size);
    return 1;
}

static int buffer_writestring(lua_State* L)
{
    /*count*/ scrypt_def(STR_0, "\x9d\x91\x8b\x92\x8c");

    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int offset = luaL_checkinteger(L, 2);
    size_t size = 0;
    const char* val = luaL_checklstring(L, 3, &size);
    int count = luaL_optinteger(L, 4, int(size));

    luaL_argcheck(L, count >= 0, 4, STR_0->c_str());

    if (size_t(count) > size) {
        #define STR_1 /*string length overflow*/ scrypt("\x8d\x8c\x8e\x97\x92\x99\xe0\x94\x9b\x92\x99\x8c\x98\xe0\x91\x8a\x9b\x8e\x9a\x94\x91\x89").c_str()
        luaL_error(L, STR_1);
        #undef STR_1
    }

    // string size can't exceed INT_MAX at this point
    if (isoutofbounds(offset, len, unsigned(count))) {
        #define STR_2 /*buffer access out of bounds*/ scrypt("\x9e\x8b\x9a\x9a\x9b\x8e\xe0\x9f\x9d\x9d\x9b\x8d\x8d\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x9e\x91\x8b\x92\x9c\x8d").c_str()
        luaL_error(L, STR_2);
        #undef STR_2
    }

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
#define STR_0 /*buffer access out of bounds*/ scrypt("\x9e\x8b\x9a\x9a\x9b\x8e\xe0\x9f\x9d\x9d\x9b\x8d\x8d\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x9e\x91\x8b\x92\x9c\x8d").c_str()

    size_t tlen = 0;
    void* tbuf = luaL_checkbuffer(L, 1, &tlen);
    int toffset = luaL_checkinteger(L, 2);

    size_t slen = 0;
    void* sbuf = luaL_checkbuffer(L, 3, &slen);
    int soffset = luaL_optinteger(L, 4, 0);

    int size = luaL_optinteger(L, 5, int(slen) - soffset);

    if (size < 0)
        luaL_error(L, STR_0);

    if (isoutofbounds(soffset, slen, unsigned(size)))
        luaL_error(L, STR_0);

    if (isoutofbounds(toffset, tlen, unsigned(size)))
        luaL_error(L, STR_0);

    memmove((char*)tbuf + toffset, (char*)sbuf + soffset, size);
    return 0;

#undef STR_0
}

static int buffer_fill(lua_State* L)
{
#define STR_0 /*buffer access out of bounds*/ scrypt("\x9e\x8b\x9a\x9a\x9b\x8e\xe0\x9f\x9d\x9d\x9b\x8d\x8d\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x9e\x91\x8b\x92\x9c\x8d").c_str()

    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int offset = luaL_checkinteger(L, 2);
    unsigned value = luaL_checkunsigned(L, 3);
    int size = luaL_optinteger(L, 4, int(len) - offset);

    if (size < 0)
        luaL_error(L, STR_0);

    if (isoutofbounds(offset, len, unsigned(size)))
        luaL_error(L, STR_0);

    memset((char*)buf + offset, value & 0xff, size);
    return 0;

#undef STR_0
}

#define STR_0 /*buffer access out of bounds*/ scrypt("\x9e\x8b\x9a\x9a\x9b\x8e\xe0\x9f\x9d\x9d\x9b\x8d\x8d\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x9e\x91\x8b\x92\x9c\x8d").c_str()
#define STR_1 /*bit count is out of range of [0; 32]*/ scrypt("\x9e\x97\x8c\xe0\x9d\x91\x8b\x92\x8c\xe0\x97\x8d\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x8e\x9f\x92\x99\x9b\xe0\x91\x9a\xe0\xa5\xd0\xc5\xe0\xcd\xce\xa3").c_str() 
#define STR_2 /*buffer access out of bounds*/ scrypt("\x9e\x8b\x9a\x9a\x9b\x8e\xe0\x9f\x9d\x9d\x9b\x8d\x8d\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x9e\x91\x8b\x92\x9c\x8d").c_str() 

static int buffer_readbits(lua_State* L)
{
    size_t len = 0;
    void* buf = luaL_checkbuffer(L, 1, &len);
    int64_t bitoffset = (int64_t)luaL_checknumber(L, 2);
    int bitcount = luaL_checkinteger(L, 3);

    if (bitoffset < 0)
        luaL_error(L, STR_0);

    if (unsigned(bitcount) > 32)
        luaL_error(L, STR_1);

    if (uint64_t(bitoffset + bitcount) > uint64_t(len) * 8)
        luaL_error(L, STR_2);

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
        luaL_error(L, STR_0);

    if (unsigned(bitcount) > 32)
        luaL_error(L, STR_1);

    if (uint64_t(bitoffset + bitcount) > uint64_t(len) * 8)
        luaL_error(L, STR_2);

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

#undef STR_2
#undef STR_1
#undef STR_0

int luaopen_buffer(lua_State* L)
{
    std::string STR_0  = /*create*/ scrypt("\x9d\x8e\x9b\x9f\x8c\x9b");
    std::string STR_1  = /*fromstring*/ scrypt("\x9a\x8e\x91\x93\x8d\x8c\x8e\x97\x92\x99");
    std::string STR_2  = /*tostring*/ scrypt("\x8c\x91\x8d\x8c\x8e\x97\x92\x99");
    std::string STR_3  = /*readi8*/ scrypt("\x8e\x9b\x9f\x9c\x97\xc8");
    std::string STR_4  = /*readu8*/ scrypt("\x8e\x9b\x9f\x9c\x8b\xc8");
    std::string STR_5  = /*readi16*/ scrypt("\x8e\x9b\x9f\x9c\x97\xcf\xca");
    std::string STR_6  = /*readu16*/ scrypt("\x8e\x9b\x9f\x9c\x8b\xcf\xca");
    std::string STR_7  = /*readi32*/ scrypt("\x8e\x9b\x9f\x9c\x97\xcd\xce");
    std::string STR_8  = /*readu32*/ scrypt("\x8e\x9b\x9f\x9c\x8b\xcd\xce");
    std::string STR_9  = /*readf32*/ scrypt("\x8e\x9b\x9f\x9c\x9a\xcd\xce");
    std::string STR_10 = /*readf64*/ scrypt("\x8e\x9b\x9f\x9c\x9a\xca\xcc");
    std::string STR_11 = /*writei8*/ scrypt("\x89\x8e\x97\x8c\x9b\x97\xc8");
    std::string STR_12 = /*writeu8*/ scrypt("\x89\x8e\x97\x8c\x9b\x8b\xc8");
    std::string STR_13 = /*writei16*/ scrypt("\x89\x8e\x97\x8c\x9b\x97\xcf\xca");
    std::string STR_14 = /*writeu16*/ scrypt("\x89\x8e\x97\x8c\x9b\x8b\xcf\xca");
    std::string STR_15 = /*writei32*/ scrypt("\x89\x8e\x97\x8c\x9b\x97\xcd\xce");
    std::string STR_16 = /*writeu32*/ scrypt("\x89\x8e\x97\x8c\x9b\x8b\xcd\xce");
    std::string STR_17 = /*writef32*/ scrypt("\x89\x8e\x97\x8c\x9b\x9a\xcd\xce");
    std::string STR_18 = /*writef64*/ scrypt("\x89\x8e\x97\x8c\x9b\x9a\xca\xcc");
    std::string STR_19 = /*readstring*/ scrypt("\x8e\x9b\x9f\x9c\x8d\x8c\x8e\x97\x92\x99");
    std::string STR_20 = /*writestring*/ scrypt("\x89\x8e\x97\x8c\x9b\x8d\x8c\x8e\x97\x92\x99");
    std::string STR_21 = /*len*/ scrypt("\x94\x9b\x92");
    std::string STR_22 = /*copy*/ scrypt("\x9d\x91\x90\x87");
    std::string STR_23 = /*fill*/ scrypt("\x9a\x97\x94\x94");
    std::string STR_24 = /*buffer*/ scrypt("\x9e\x8b\x9a\x9a\x9b\x8e");
    std::string STR_25 = /*readbits*/ scrypt("\x8e\x9b\x9f\x9c\x9e\x97\x8c\x8d");
    std::string STR_26 = /*writebits*/ scrypt("\x89\x8e\x97\x8c\x9b\x9e\x97\x8c\x8d");

    const luaL_Reg bufferlib[] = {
        {STR_0.c_str(), buffer_create},
        {STR_1.c_str(), buffer_fromstring},
        {STR_2.c_str(), buffer_tostring},
        {STR_3.c_str(), buffer_readinteger<int8_t>},
        {STR_4.c_str(), buffer_readinteger<uint8_t>},
        {STR_5.c_str(), buffer_readinteger<int16_t>},
        {STR_6.c_str(), buffer_readinteger<uint16_t>},
        {STR_7.c_str(), buffer_readinteger<int32_t>},
        {STR_8.c_str(), buffer_readinteger<uint32_t>},
        {STR_9.c_str(), buffer_readfp<float, uint32_t>},
        {STR_10.c_str(), buffer_readfp<double, uint64_t>},
        {STR_11.c_str(), buffer_writeinteger<int8_t>},
        {STR_12.c_str(), buffer_writeinteger<uint8_t>},
        {STR_13.c_str(), buffer_writeinteger<int16_t>},
        {STR_14.c_str(), buffer_writeinteger<uint16_t>},
        {STR_15.c_str(), buffer_writeinteger<int32_t>},
        {STR_16.c_str(), buffer_writeinteger<uint32_t>},
        {STR_17.c_str(), buffer_writefp<float, uint32_t>},
        {STR_18.c_str(), buffer_writefp<double, uint64_t>},
        {STR_19.c_str(), buffer_readstring},
        {STR_20.c_str(), buffer_writestring},
        {STR_21.c_str(), buffer_len},
        {STR_22.c_str(), buffer_copy},
        {STR_23.c_str(), buffer_fill},
        {STR_25.c_str(), buffer_readbits},
        {STR_26.c_str(), buffer_writebits},
        {NULL, NULL},
    };

    luaL_register(L, STR_24.c_str(), bufferlib);

    return 1;
}
