// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lualib.h"
#include "lstate.h"
#include "MinCrypt.hpp"

#include <stdlib.h>

LUAU_FASTFLAG(LuauIntegerLibrary)
LUAU_FASTFLAG(DebugLuauUserDefinedClassesRuntime)

void luaL_openlibs(lua_State* L)
{
    auto n_co       = MINCRYPT_STACK_CODE(LUA_COLIBNAME);
    auto n_tab      = MINCRYPT_STACK_CODE(LUA_TABLIBNAME);
    auto n_os       = MINCRYPT_STACK_CODE(LUA_OSLIBNAME);
    auto n_str      = MINCRYPT_STACK_CODE(LUA_STRLIBNAME);
    auto n_math     = MINCRYPT_STACK_CODE(LUA_MATHLIBNAME);
    auto n_db       = MINCRYPT_STACK_CODE(LUA_DBLIBNAME);
    auto n_utf8     = MINCRYPT_STACK_CODE(LUA_UTF8LIBNAME);
    auto n_bit      = MINCRYPT_STACK_CODE(LUA_BITLIBNAME);
    auto n_buf      = MINCRYPT_STACK_CODE(LUA_BUFFERLIBNAME);
    auto n_vec      = MINCRYPT_STACK_CODE(LUA_VECLIBNAME);
    auto n_int      = MINCRYPT_STACK_CODE(LUA_INTLIBNAME);

    luaL_Reg lualibs[] = {
        {"", luaopen_base},
        {n_co.get_data(), luaopen_coroutine},
        {n_tab.get_data(), luaopen_table},
        {n_os.get_data(), luaopen_os},
        {n_str.get_data(), luaopen_string},
        {n_math.get_data(), luaopen_math},
        {n_db.get_data(), luaopen_debug},
        {n_utf8.get_data(), luaopen_utf8},
        {n_bit.get_data(), luaopen_bit32},
        {n_buf.get_data(), luaopen_buffer},
        {n_vec.get_data(), luaopen_vector},
        {n_int.get_data(), luaopen_integer},
        {NULL, NULL},
    };

    luaL_Reg lualibs_NOINTEGER[] = {
        {"", luaopen_base},
        {n_co.get_data(), luaopen_coroutine},
        {n_tab.get_data(), luaopen_table},
        {n_os.get_data(), luaopen_os},
        {n_str.get_data(), luaopen_string},
        {n_math.get_data(), luaopen_math},
        {n_db.get_data(), luaopen_debug},
        {n_utf8.get_data(), luaopen_utf8},
        {n_bit.get_data(), luaopen_bit32},
        {n_buf.get_data(), luaopen_buffer},
        {n_vec.get_data(), luaopen_vector},
        {NULL, NULL},
    };

    const luaL_Reg* lib;
    if (FFlag::LuauIntegerLibrary)
        lib = lualibs;
    else
        lib = lualibs_NOINTEGER;

    for (; lib->func; lib++)
    {
        lua_pushcfunction(L, lib->func, NULL);
        lua_pushstring(L, lib->name);
        lua_call(L, 1, 0);
    }

    if (FFlag::DebugLuauUserDefinedClassesRuntime)
    {
        lua_pushcfunction(L, luaopen_class, NULL);
        lua_pushstring(L, MINCRYPT(LUA_CLASSLIBNAME));
        lua_call(L, 1, 0);
    }
}

void luaL_sandbox(lua_State* L)
{
    // set all libraries to read-only
    lua_pushnil(L);
    while (lua_next(L, LUA_GLOBALSINDEX) != 0)
    {
        if (lua_istable(L, -1))
            lua_setreadonly(L, -1, true);

        lua_pop(L, 1);
    }

    // set all builtin metatables to read-only
    lua_pushliteral(L, "");
    if (lua_getmetatable(L, -1))
    {
        lua_setreadonly(L, -1, true);
        lua_pop(L, 2);
    }
    else
    {
        lua_pop(L, 1);
    }

    // set globals to readonly and activate safeenv since the env is immutable
    lua_setreadonly(L, LUA_GLOBALSINDEX, true);
    lua_setsafeenv(L, LUA_GLOBALSINDEX, true);
}

void luaL_sandboxthread(lua_State* L)
{
    // create new global table that proxies reads to original table
    lua_newtable(L);

    lua_newtable(L);
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setfield(L, -2, MINCRYPT_LAZY("__index")());
    lua_setreadonly(L, -1, true);

    lua_setmetatable(L, -2);

    // we can set safeenv now although it's important to set it to false if code is loaded twice into the thread
    lua_replace(L, LUA_GLOBALSINDEX);
    lua_setsafeenv(L, LUA_GLOBALSINDEX, true);
}

static void* l_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
    (void)ud;
    (void)osize;
    if (nsize == 0)
    {
        free(ptr);
        return NULL;
    }
    else
        return realloc(ptr, nsize);
}

lua_State* luaL_newstate(void)
{
    return lua_newstate(l_alloc, NULL);
}
