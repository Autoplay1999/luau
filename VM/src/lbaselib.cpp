// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lualib.h"

#include "lstate.h"
#include "lapi.h"
#include "ldo.h"
#include "ludata.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static void writestring(const char* s, size_t l)
{
    fwrite(s, 1, l, stdout);
}

static int luaB_print(lua_State* L)
{
    int n = lua_gettop(L); // number of arguments
    for (int i = 1; i <= n; i++)
    {
        size_t l;
        const char* s = luaL_tolstring(L, i, &l); // convert to string using __tostring et al
        if (i > 1)
            writestring("\t", 1);
        writestring(s, l);
        lua_pop(L, 1); // pop result
    }
    writestring("\n", 1);
    return 0;
}

static int luaB_tonumber(lua_State* L)
{
    int base = luaL_optinteger(L, 2, 10);
    if (base == 10)
    { // standard conversion
        int isnum = 0;
        double n = lua_tonumberx(L, 1, &isnum);
        if (isnum)
        {
            lua_pushnumber(L, n);
            return 1;
        }
        luaL_checkany(L, 1); // error if we don't have any argument
    }
    else
    {
        /*base out of range*/ scrypt_def(STR_0, "\x9e\x9f\x8d\x9b\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x8e\x9f\x92\x99\x9b");
        const char* s1 = luaL_checkstring(L, 1);
        luaL_argcheck(L, 2 <= base && base <= 36, 2, STR_0->c_str());
        char* s2;
        unsigned long long n;
        n = strtoull(s1, &s2, base);
        if (s1 != s2)
        { // at least one valid digit?
            while (isspace((unsigned char)(*s2)))
                s2++; // skip trailing spaces
            if (*s2 == '\0')
            { // no invalid trailing characters?
                lua_pushnumber(L, (double)n);
                return 1;
            }
        }
    }
    lua_pushnil(L); // else not a number
    return 1;
}

static int luaB_error(lua_State* L)
{
    int level = luaL_optinteger(L, 2, 1);
    lua_settop(L, 1);
    if (lua_isstring(L, 1) && level > 0)
    { // add extra information?
        luaL_where(L, level);
        lua_pushvalue(L, 1);
        lua_concat(L, 2);
    }
    lua_error(L);
}

static int luaB_getmetatable(lua_State* L)
{
    luaL_checkany(L, 1);
    if (!lua_getmetatable(L, 1))
    {
        lua_pushnil(L);
        return 1; // no metatable
    }
    /*__metatable*/ scrypt_def(STR_0, "\xa1\xa1\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b");
    luaL_getmetafield(L, 1, STR_0->c_str());
    return 1; // returns either __metatable field (if present) or metatable
}

static int luaB_setmetatable(lua_State* L)
{
    /*nil or table*/ scrypt_def(STR_0, "\x92\x97\x94\xe0\x91\x8e\xe0\x8c\x9f\x9e\x94\x9b");
    /*__metatable*/ scrypt_def(STR_1, "\xa1\xa1\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b");

    int t = lua_type(L, 2);
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_argexpected(L, t == LUA_TNIL || t == LUA_TTABLE, 2, STR_0->c_str());
    if (luaL_getmetafield(L, 1, STR_1->c_str())) {
        #define STR_2 /*cannot change a protected metatable*/ scrypt("\x9d\x9f\x92\x92\x91\x8c\xe0\x9d\x98\x9f\x92\x99\x9b\xe0\x9f\xe0\x90\x8e\x91\x8c\x9b\x9d\x8c\x9b\x9c\xe0\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b").c_str()
        luaL_error(L, STR_2);
        #undef STR_2
    }
    lua_settop(L, 2);
    lua_setmetatable(L, 1);
    return 1;
}

static void getfunc(lua_State* L, int opt)
{
    if (lua_isfunction(L, 1))
        lua_pushvalue(L, 1);
    else
    {
        /*level must be non-negative*/ scrypt_def(STR_0, "\x94\x9b\x8a\x9b\x94\xe0\x93\x8b\x8d\x8c\xe0\x9e\x9b\xe0\x92\x91\x92\xd3\x92\x9b\x99\x9f\x8c\x97\x8a\x9b");
        lua_Debug ar;
        int level = opt ? luaL_optinteger(L, 1, 1) : luaL_checkinteger(L, 1);
        luaL_argcheck(L, level >= 0, 1, STR_0->c_str());
        if (lua_getinfo(L, level, "f", &ar) == 0) {
            #define STR_1 /*invalid level*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x94\x9b\x8a\x9b\x94").c_str()
            luaL_argerror(L, 1, STR_1);
            #undef STR_1
        }
        if (lua_isnil(L, -1)) {
            #define STR_2 /*no function environment for tail call at level %d*/ scrypt("\x92\x91\xe0\x9a\x8b\x92\x9d\x8c\x97\x91\x92\xe0\x9b\x92\x8a\x97\x8e\x91\x92\x93\x9b\x92\x8c\xe0\x9a\x91\x8e\xe0\x8c\x9f\x97\x94\xe0\x9d\x9f\x94\x94\xe0\x9f\x8c\xe0\x94\x9b\x8a\x9b\x94\xe0\xdb\x9c").c_str()
            luaL_error(L, STR_2, level);
            #undef STR_2
        }
    }
}

static int luaB_getfenv(lua_State* L)
{
    getfunc(L, 1);
    if (lua_iscfunction(L, -1))             // is a C function?
        lua_pushvalue(L, LUA_GLOBALSINDEX); // return the thread's global env.
    else
        lua_getfenv(L, -1);
    lua_setsafeenv(L, -1, false);
    return 1;
}

static int luaB_setfenv(lua_State* L)
{
    luaL_checktype(L, 2, LUA_TTABLE);
    getfunc(L, 0);
    lua_pushvalue(L, 2);
    lua_setsafeenv(L, -1, false);
    if (lua_isnumber(L, 1) && lua_tonumber(L, 1) == 0)
    {
        // change environment of current thread
        lua_pushthread(L);
        lua_insert(L, -2);
        lua_setfenv(L, -2);
        return 0;
    }
    else if (lua_iscfunction(L, -2) || lua_setfenv(L, -2) == 0) {
        #define STR_0 /*'setfenv' cannot change environment of given object*/ scrypt("\xd9\x8d\x9b\x8c\x9a\x9b\x92\x8a\xd9\xe0\x9d\x9f\x92\x92\x91\x8c\xe0\x9d\x98\x9f\x92\x99\x9b\xe0\x9b\x92\x8a\x97\x8e\x91\x92\x93\x9b\x92\x8c\xe0\x91\x9a\xe0\x99\x97\x8a\x9b\x92\xe0\x91\x9e\x96\x9b\x9d\x8c").c_str()
        luaL_error(L, STR_0);
        #undef STR_0
    }
    return 1;
}

static int luaB_rawequal(lua_State* L)
{
    luaL_checkany(L, 1);
    luaL_checkany(L, 2);
    lua_pushboolean(L, lua_rawequal(L, 1, 2));
    return 1;
}

static int luaB_rawget(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checkany(L, 2);
    lua_settop(L, 2);
    lua_rawget(L, 1);
    return 1;
}

static int luaB_rawset(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checkany(L, 2);
    luaL_checkany(L, 3);
    lua_settop(L, 3);
    lua_rawset(L, 1);
    return 1;
}

static int luaB_rawlen(lua_State* L)
{
    /*table or string expected*/ scrypt_def(STR_0, "\x8c\x9f\x9e\x94\x9b\xe0\x91\x8e\xe0\x8d\x8c\x8e\x97\x92\x99\xe0\x9b\x88\x90\x9b\x9d\x8c\x9b\x9c");

    int tt = lua_type(L, 1);
    luaL_argcheck(L, tt == LUA_TTABLE || tt == LUA_TSTRING, 1, STR_0->c_str());
    int len = lua_objlen(L, 1);
    lua_pushinteger(L, len);
    return 1;
}

static int luaB_gcinfo(lua_State* L)
{
    lua_pushinteger(L, lua_gc(L, LUA_GCCOUNT, 0));
    return 1;
}

static int luaB_type(lua_State* L)
{
    luaL_checkany(L, 1);
    // resulting name doesn't differentiate between userdata types
    lua_pushstring(L, lua_typename(L, lua_type(L, 1)));
    return 1;
}

static int luaB_typeof(lua_State* L)
{
    luaL_checkany(L, 1);
    // resulting name returns __type if specified unless the input is a newproxy-created userdata
    lua_pushstring(L, luaL_typename(L, 1));
    return 1;
}

int luaB_next(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 2); // create a 2nd argument if there isn't one
    if (lua_next(L, 1))
        return 2;
    else
    {
        lua_pushnil(L);
        return 1;
    }
}

static int luaB_pairs(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_pushvalue(L, lua_upvalueindex(1)); // return generator,
    lua_pushvalue(L, 1);                   // state,
    lua_pushnil(L);                        // and initial value
    return 3;
}

int luaB_inext(lua_State* L)
{
    int i = luaL_checkinteger(L, 2);
    luaL_checktype(L, 1, LUA_TTABLE);
    i++; // next value
    lua_pushinteger(L, i);
    lua_rawgeti(L, 1, i);
    return (lua_isnil(L, -1)) ? 0 : 2;
}

static int luaB_ipairs(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_pushvalue(L, lua_upvalueindex(1)); // return generator,
    lua_pushvalue(L, 1);                   // state,
    lua_pushinteger(L, 0);                 // and initial value
    return 3;
}

static int luaB_assert(lua_State* L)
{
    luaL_checkany(L, 1);
    if (!lua_toboolean(L, 1)) {
        #define STR_0 /*assertion failed!*/ scrypt("\x9f\x8d\x8d\x9b\x8e\x8c\x97\x91\x92\xe0\x9a\x9f\x97\x94\x9b\x9c\xdf").c_str()
        luaL_error(L, "%s", luaL_optstring(L, 2, STR_0));
        #undef STR_0
    }
    return lua_gettop(L);
}

static int luaB_select(lua_State* L)
{
    int n = lua_gettop(L);
    if (lua_type(L, 1) == LUA_TSTRING && *lua_tostring(L, 1) == '#')
    {
        lua_pushinteger(L, n - 1);
        return 1;
    }
    else
    {
        /*index out of range*/ scrypt_def(STR_0, "\x97\x92\x9c\x9b\x88\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x8e\x9f\x92\x99\x9b");
        int i = luaL_checkinteger(L, 1);
        if (i < 0)
            i = n + i;
        else if (i > n)
            i = n;
        luaL_argcheck(L, 1 <= i, 1, STR_0->c_str());
        return n - i;
    }
}

static void luaB_pcallrun(lua_State* L, void* ud)
{
    StkId func = (StkId)ud;

    luaD_call(L, func, LUA_MULTRET);
}

static int luaB_pcally(lua_State* L)
{
    luaL_checkany(L, 1);

    StkId func = L->base;

    // any errors from this point on are handled by continuation
    L->ci->flags |= LUA_CALLINFO_HANDLE;

    // maintain yieldable invariant (baseCcalls <= nCcalls)
    L->baseCcalls++;
    int status = luaD_pcall(L, luaB_pcallrun, func, savestack(L, func), 0);
    L->baseCcalls--;

    // necessary to accomodate functions that return lots of values
    expandstacklimit(L, L->top);

    // yielding means we need to propagate yield; resume will call continuation function later
    if (status == 0 && (L->status == LUA_YIELD || L->status == LUA_BREAK))
        return -1; // -1 is a marker for yielding from C

    // immediate return (error or success)
    lua_rawcheckstack(L, 1);
    lua_pushboolean(L, status == 0);
    lua_insert(L, 1);
    return lua_gettop(L); // return status + all results
}

static int luaB_pcallcont(lua_State* L, int status)
{
    if (status == 0)
    {
        lua_rawcheckstack(L, 1);
        lua_pushboolean(L, true);
        lua_insert(L, 1); // insert status before all results
        return lua_gettop(L);
    }
    else
    {
        lua_rawcheckstack(L, 1);
        lua_pushboolean(L, false);
        lua_insert(L, -2); // insert status before error object
        return 2;
    }
}

static int luaB_xpcally(lua_State* L)
{
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // swap function & error function
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_replace(L, 1);
    lua_replace(L, 2);
    // at this point the stack looks like err, f, args

    // any errors from this point on are handled by continuation
    L->ci->flags |= LUA_CALLINFO_HANDLE;

    StkId errf = L->base;
    StkId func = L->base + 1;

    // maintain yieldable invariant (baseCcalls <= nCcalls)
    L->baseCcalls++;
    int status = luaD_pcall(L, luaB_pcallrun, func, savestack(L, func), savestack(L, errf));
    L->baseCcalls--;

    // necessary to accomodate functions that return lots of values
    expandstacklimit(L, L->top);

    // yielding means we need to propagate yield; resume will call continuation function later
    if (status == 0 && (L->status == LUA_YIELD || L->status == LUA_BREAK))
        return -1; // -1 is a marker for yielding from C

    // immediate return (error or success)
    lua_rawcheckstack(L, 1);
    lua_pushboolean(L, status == 0);
    lua_replace(L, 1);    // replace error function with status
    return lua_gettop(L); // return status + all results
}

static void luaB_xpcallerr(lua_State* L, void* ud)
{
    StkId func = (StkId)ud;

    luaD_call(L, func, 1);
}

static int luaB_xpcallcont(lua_State* L, int status)
{
    if (status == 0)
    {
        lua_rawcheckstack(L, 1);
        lua_pushboolean(L, true);
        lua_replace(L, 1);    // replace error function with status
        return lua_gettop(L); // return status + all results
    }
    else
    {
        lua_rawcheckstack(L, 3);
        lua_pushboolean(L, false);
        lua_pushvalue(L, 1);  // push error function on top of the stack
        lua_pushvalue(L, -3); // push error object (that was on top of the stack before)

        StkId res = L->top - 3;
        StkId errf = L->top - 2;

        // note: we pass res as errfunc as a short cut; if errf generates an error, we'll try to execute res (boolean) and fail
        luaD_pcall(L, luaB_xpcallerr, errf, savestack(L, errf), savestack(L, res));

        return 2;
    }
}

static int luaB_tostring(lua_State* L)
{
    luaL_checkany(L, 1);
    luaL_tolstring(L, 1, NULL);
    return 1;
}

static int luaB_newproxy(lua_State* L)
{
    /*nil or boolean*/ scrypt_def(STR_0, "\x92\x97\x94\xe0\x91\x8e\xe0\x9e\x91\x91\x94\x9b\x9f\x92");

    int t = lua_type(L, 1);
    luaL_argexpected(L, t == LUA_TNONE || t == LUA_TNIL || t == LUA_TBOOLEAN, 1, STR_0->c_str());

    bool needsmt = lua_toboolean(L, 1);

    lua_newuserdatatagged(L, 0, UTAG_PROXY);

    if (needsmt)
    {
        lua_newtable(L);
        lua_setmetatable(L, -2);
    }

    return 1;
}

static void auxopen(lua_State* L, const char* name, lua_CFunction f, lua_CFunction u)
{
    lua_pushcfunction(L, u, NULL);
    lua_pushcclosure(L, f, name, 1);
    lua_setfield(L, -2, name);
}

int luaopen_base(lua_State* L)
{
    std::string STR_0  = /*assert*/ scrypt("\x9f\x8d\x8d\x9b\x8e\x8c");
    std::string STR_1  = /*error*/ scrypt("\x9b\x8e\x8e\x91\x8e");
    std::string STR_2  = /*gcinfo*/ scrypt("\x99\x9d\x97\x92\x9a\x91");
    std::string STR_3  = /*getfenv*/ scrypt("\x99\x9b\x8c\x9a\x9b\x92\x8a");
    std::string STR_4  = /*getmetatable*/ scrypt("\x99\x9b\x8c\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b");
    std::string STR_5  = /*next*/ scrypt("\x92\x9b\x88\x8c");
    std::string STR_6  = /*newproxy*/ scrypt("\x92\x9b\x89\x90\x8e\x91\x88\x87");
    std::string STR_7  = /*print*/ scrypt("\x90\x8e\x97\x92\x8c");
    std::string STR_8  = /*rawequal*/ scrypt("\x8e\x9f\x89\x9b\x8f\x8b\x9f\x94");
    std::string STR_9  = /*rawget*/ scrypt("\x8e\x9f\x89\x99\x9b\x8c");
    std::string STR_10 = /*rawset*/ scrypt("\x8e\x9f\x89\x8d\x9b\x8c");
    std::string STR_11 = /*rawlen*/ scrypt("\x8e\x9f\x89\x94\x9b\x92");
    std::string STR_12 = /*select*/ scrypt("\x8d\x9b\x94\x9b\x9d\x8c");
    std::string STR_13 = /*setfenv*/ scrypt("\x8d\x9b\x8c\x9a\x9b\x92\x8a");
    std::string STR_14 = /*setmetatable*/ scrypt("\x8d\x9b\x8c\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b");
    std::string STR_15 = /*tonumber*/ scrypt("\x8c\x91\x92\x8b\x93\x9e\x9b\x8e");
    std::string STR_16 = /*tostring*/ scrypt("\x8c\x91\x8d\x8c\x8e\x97\x92\x99");
    std::string STR_17 = /*type*/ scrypt("\x8c\x87\x90\x9b");
    std::string STR_18 = /*typeof*/ scrypt("\x8c\x87\x90\x9b\x91\x9a");
    std::string STR_19 = /*_G*/ scrypt("\xa1\xb9");
    std::string STR_20 = /*Luau*/ scrypt("\xb4\x8b\x9f\x8b");
    std::string STR_21 = /*_VERSION*/ scrypt("\xa1\xaa\xbb\xae\xad\xb7\xb1\xb2");
    std::string STR_22 = /*ipairs*/ scrypt("\x97\x90\x9f\x97\x8e\x8d");
    std::string STR_23 = /*pairs*/ scrypt("\x90\x9f\x97\x8e\x8d");
    std::string STR_24 = /*pcall*/ scrypt("\x90\x9d\x9f\x94\x94");
    std::string STR_25 = /*xpcall*/ scrypt("\x88\x90\x9d\x9f\x94\x94");

    const luaL_Reg base_funcs[] = {
        {STR_0.c_str(), luaB_assert},
        {STR_1.c_str(), luaB_error},
        {STR_2.c_str(), luaB_gcinfo},
        {STR_3.c_str(), luaB_getfenv},
        {STR_4.c_str(), luaB_getmetatable},
        {STR_5.c_str(), luaB_next},
        {STR_6.c_str(), luaB_newproxy},
        {STR_7.c_str(), luaB_print},
        {STR_8.c_str(), luaB_rawequal},
        {STR_9.c_str(), luaB_rawget},
        {STR_10.c_str(), luaB_rawset},
        {STR_11.c_str(), luaB_rawlen},
        {STR_12.c_str(), luaB_select},
        {STR_13.c_str(), luaB_setfenv},
        {STR_14.c_str(), luaB_setmetatable},
        {STR_15.c_str(), luaB_tonumber},
        {STR_16.c_str(), luaB_tostring},
        {STR_17.c_str(), luaB_type},
        {STR_18.c_str(), luaB_typeof},
        {NULL, NULL},
    };

    // set global _G
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setglobal(L, STR_19.c_str());

    // open lib into global table
    luaL_register(L, STR_19.c_str(), base_funcs);
    lua_pushstring(L, STR_20.c_str());
    lua_setglobal(L, STR_21.c_str()); // set global _VERSION

    // `ipairs' and `pairs' need auxiliary functions as upvalues
    auxopen(L, STR_22.c_str(), luaB_ipairs, luaB_inext);
    auxopen(L, STR_23.c_str(), luaB_pairs, luaB_next);

    lua_pushcclosurek(L, luaB_pcally, STR_24.c_str(), 0, luaB_pcallcont);
    lua_setfield(L, -2, STR_24.c_str());

    lua_pushcclosurek(L, luaB_xpcally, STR_25.c_str(), 0, luaB_xpcallcont);
    lua_setfield(L, -2, STR_25.c_str());

    return 1;
}
