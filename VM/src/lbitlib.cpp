// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lualib.h"

#include "lcommon.h"
#include "lnumutils.h"
#include "MinCrypt.hpp"

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
    int f = luaL_checkinteger(L, farg);
    int w = luaL_optinteger(L, farg + 1, 1);
    luaL_argcheck(L, 0 <= f, farg, MINCRYPT_LAZY("field cannot be negative")());
    luaL_argcheck(L, 0 < w, farg + 1, MINCRYPT_LAZY("width must be positive")());
    if (f + w > NBITS)
        luaL_error(L, MINCRYPT("trying to access non-existent bits"));
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
    auto n_arshift = MINCRYPT_LAZY("arshift")();
    auto n_band = MINCRYPT_LAZY("band")();
    auto n_bnot = MINCRYPT_LAZY("bnot")();
    auto n_bor = MINCRYPT_LAZY("bor")();
    auto n_bxor = MINCRYPT_LAZY("bxor")();
    auto n_btest = MINCRYPT_LAZY("btest")();
    auto n_extract = MINCRYPT_LAZY("extract")();
    auto n_lrotate = MINCRYPT_LAZY("lrotate")();
    auto n_lshift = MINCRYPT_LAZY("lshift")();
    auto n_replace = MINCRYPT_LAZY("replace")();
    auto n_rrotate = MINCRYPT_LAZY("rrotate")();
    auto n_rshift = MINCRYPT_LAZY("rshift")();
    auto n_countlz = MINCRYPT_LAZY("countlz")();
    auto n_countrz = MINCRYPT_LAZY("countrz")();
    auto n_byteswap = MINCRYPT_LAZY("byteswap")();

    luaL_Reg bitlib[] = {
        {n_arshift,  b_arshift},
        {n_band,     b_and},
        {n_bnot,     b_not},
        {n_bor,      b_or},
        {n_bxor,     b_xor},
        {n_btest,    b_test},
        {n_extract,  b_extract},
        {n_lrotate,  b_lrot},
        {n_lshift,   b_lshift},
        {n_replace,  b_replace},
        {n_rrotate,  b_rrot},
        {n_rshift,   b_rshift},
        {n_countlz,  b_countlz},
        {n_countrz,  b_countrz},
        {n_byteswap, b_swap},
        {NULL, NULL},
    };

    luaL_register(L, MINCRYPT(LUA_BITLIBNAME), bitlib);

    return 1;
}
