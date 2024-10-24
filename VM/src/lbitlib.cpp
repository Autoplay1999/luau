// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lualib.h"

#include "lcommon.h"
#include "lnumutils.h"

#define ALLONES ~0u
#define NBITS int(8 * sizeof(unsigned))

// macro to trim extra bits
#define trim(x) ((x) & ALLONES)

// builds a number with 'n' ones (1 <= n <= NBITS)
#define mask(n) (~((ALLONES << 1) << ((n)-1)))

typedef unsigned b_uint;

static b_uint andaux(lua_State* L)
{
    int i, n = lua_gettop(L);
    b_uint r = ~(b_uint)0;
    for (i = 1; i <= n; i++)
        r &= luaL_checkunsigned(L, i);
    return trim(r);
}

static int b_and(lua_State* L)
{
    b_uint r = andaux(L);
    lua_pushunsigned(L, r);
    return 1;
}

static int b_test(lua_State* L)
{
    b_uint r = andaux(L);
    lua_pushboolean(L, r != 0);
    return 1;
}

static int b_or(lua_State* L)
{
    int i, n = lua_gettop(L);
    b_uint r = 0;
    for (i = 1; i <= n; i++)
        r |= luaL_checkunsigned(L, i);
    lua_pushunsigned(L, trim(r));
    return 1;
}

static int b_xor(lua_State* L)
{
    int i, n = lua_gettop(L);
    b_uint r = 0;
    for (i = 1; i <= n; i++)
        r ^= luaL_checkunsigned(L, i);
    lua_pushunsigned(L, trim(r));
    return 1;
}

static int b_not(lua_State* L)
{
    b_uint r = ~luaL_checkunsigned(L, 1);
    lua_pushunsigned(L, trim(r));
    return 1;
}

static int b_shift(lua_State* L, b_uint r, int i)
{
    if (i < 0)
    { // shift right?
        i = -i;
        r = trim(r);
        if (i >= NBITS)
            r = 0;
        else
            r >>= i;
    }
    else
    { // shift left
        if (i >= NBITS)
            r = 0;
        else
            r <<= i;
        r = trim(r);
    }
    lua_pushunsigned(L, r);
    return 1;
}

static int b_lshift(lua_State* L)
{
    return b_shift(L, luaL_checkunsigned(L, 1), luaL_checkinteger(L, 2));
}

static int b_rshift(lua_State* L)
{
    return b_shift(L, luaL_checkunsigned(L, 1), -luaL_checkinteger(L, 2));
}

static int b_arshift(lua_State* L)
{
    b_uint r = luaL_checkunsigned(L, 1);
    int i = luaL_checkinteger(L, 2);
    if (i < 0 || !(r & ((b_uint)1 << (NBITS - 1))))
        return b_shift(L, r, -i);
    else
    { // arithmetic shift for 'negative' number
        if (i >= NBITS)
            r = ALLONES;
        else
            r = trim((r >> i) | ~(~(b_uint)0 >> i)); // add signal bit
        lua_pushunsigned(L, r);
        return 1;
    }
}

static int b_rot(lua_State* L, int i)
{
    b_uint r = luaL_checkunsigned(L, 1);
    i &= (NBITS - 1); // i = i % NBITS
    r = trim(r);
    if (i != 0) // avoid undefined shift of NBITS when i == 0
        r = (r << i) | (r >> (NBITS - i));
    lua_pushunsigned(L, trim(r));
    return 1;
}

static int b_lrot(lua_State* L)
{
    return b_rot(L, luaL_checkinteger(L, 2));
}

static int b_rrot(lua_State* L)
{
    return b_rot(L, -luaL_checkinteger(L, 2));
}

/*
** get field and width arguments for field-manipulation functions,
** checking whether they are valid.
** ('luaL_error' called without 'return' to avoid later warnings about
** 'width' being used uninitialized.)
*/
static int fieldargs(lua_State* L, int farg, int* width)
{
    /*field cannot be negative*/ scrypt_def(STR_0, "\x9a\x97\x9b\x94\x9c\xe0\x9d\x9f\x92\x92\x91\x8c\xe0\x9e\x9b\xe0\x92\x9b\x99\x9f\x8c\x97\x8a\x9b");
    /*width must be positive*/ scrypt_def(STR_1, "\x89\x97\x9c\x8c\x98\xe0\x93\x8b\x8d\x8c\xe0\x9e\x9b\xe0\x90\x91\x8d\x97\x8c\x97\x8a\x9b");

    int f = luaL_checkinteger(L, farg);
    int w = luaL_optinteger(L, farg + 1, 1);
    luaL_argcheck(L, 0 <= f, farg, STR_0->c_str());
    luaL_argcheck(L, 0 < w, farg + 1, STR_1->c_str());
    if (f + w > NBITS) {
        #define STR_2 /*trying to access non-existent bits*/ scrypt("\x8c\x8e\x87\x97\x92\x99\xe0\x8c\x91\xe0\x9f\x9d\x9d\x9b\x8d\x8d\xe0\x92\x91\x92\xd3\x9b\x88\x97\x8d\x8c\x9b\x92\x8c\xe0\x9e\x97\x8c\x8d").c_str()
        luaL_error(L, STR_2);
        #undef STR_2
    }
    *width = w;
    return f;
}

static int b_extract(lua_State* L)
{
    int w;
    b_uint r = luaL_checkunsigned(L, 1);
    int f = fieldargs(L, 2, &w);
    r = (r >> f) & mask(w);
    lua_pushunsigned(L, r);
    return 1;
}

static int b_replace(lua_State* L)
{
    int w;
    b_uint r = luaL_checkunsigned(L, 1);
    b_uint v = luaL_checkunsigned(L, 2);
    int f = fieldargs(L, 3, &w);
    int m = mask(w);
    v &= m; // erase bits outside given width
    r = (r & ~(m << f)) | (v << f);
    lua_pushunsigned(L, r);
    return 1;
}

static int b_countlz(lua_State* L)
{
    b_uint v = luaL_checkunsigned(L, 1);

    b_uint r = NBITS;
    for (int i = 0; i < NBITS; ++i)
        if (v & (1u << (NBITS - 1 - i)))
        {
            r = i;
            break;
        }

    lua_pushunsigned(L, r);
    return 1;
}

static int b_countrz(lua_State* L)
{
    b_uint v = luaL_checkunsigned(L, 1);

    b_uint r = NBITS;
    for (int i = 0; i < NBITS; ++i)
        if (v & (1u << i))
        {
            r = i;
            break;
        }

    lua_pushunsigned(L, r);
    return 1;
}

static int b_swap(lua_State* L)
{
    b_uint n = luaL_checkunsigned(L, 1);
    n = (n << 24) | ((n << 8) & 0xff0000) | ((n >> 8) & 0xff00) | (n >> 24);

    lua_pushunsigned(L, n);
    return 1;
}

int luaopen_bit32(lua_State* L)
{
    std::string STR_0  = /*arshift*/ scrypt("\x9f\x8e\x8d\x98\x97\x9a\x8c");
    std::string STR_1  = /*band*/ scrypt("\x9e\x9f\x92\x9c");
    std::string STR_2  = /*bnot*/ scrypt("\x9e\x92\x91\x8c");
    std::string STR_3  = /*bor*/ scrypt("\x9e\x91\x8e");
    std::string STR_4  = /*bxor*/ scrypt("\x9e\x88\x91\x8e");
    std::string STR_5  = /*btest*/ scrypt("\x9e\x8c\x9b\x8d\x8c");
    std::string STR_6  = /*extract*/ scrypt("\x9b\x88\x8c\x8e\x9f\x9d\x8c");
    std::string STR_7  = /*lrotate*/ scrypt("\x94\x8e\x91\x8c\x9f\x8c\x9b");
    std::string STR_8  = /*lshift*/ scrypt("\x94\x8d\x98\x97\x9a\x8c");
    std::string STR_9  = /*replace*/ scrypt("\x8e\x9b\x90\x94\x9f\x9d\x9b");
    std::string STR_10 = /*rrotate*/ scrypt("\x8e\x8e\x91\x8c\x9f\x8c\x9b");
    std::string STR_11 = /*rshift*/ scrypt("\x8e\x8d\x98\x97\x9a\x8c");
    std::string STR_12 = /*countlz*/ scrypt("\x9d\x91\x8b\x92\x8c\x94\x86");
    std::string STR_13 = /*countrz*/ scrypt("\x9d\x91\x8b\x92\x8c\x8e\x86");
    std::string STR_14 = /*byteswap*/ scrypt("\x9e\x87\x8c\x9b\x8d\x89\x9f\x90");
    std::string STR_15 = /*bit32*/ scrypt("\x9e\x97\x8c\xcd\xce");

    const luaL_Reg bitlib[] = {
        {STR_0.c_str(), b_arshift},
        {STR_1.c_str(), b_and},
        {STR_2.c_str(), b_not},
        {STR_3.c_str(), b_or},
        {STR_4.c_str(), b_xor},
        {STR_5.c_str(), b_test},
        {STR_6.c_str(), b_extract},
        {STR_7.c_str(), b_lrot},
        {STR_8.c_str(), b_lshift},
        {STR_9.c_str(), b_replace},
        {STR_10.c_str(), b_rrot},
        {STR_11.c_str(), b_rshift},
        {STR_12.c_str(), b_countlz},
        {STR_13.c_str(), b_countrz},
        {STR_14.c_str(), b_swap},
        {NULL, NULL},
    };

    luaL_register(L, STR_15.c_str(), bitlib);

    return 1;
}
