// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lualib.h"

#include <stdlib.h>

#include "luau\SCrypt.h"

void luaL_openlibs(lua_State* L)
{
    std::string STR_0 = /*coroutine*/ scrypt("\x9d\x91\x8e\x91\x8b\x8c\x97\x92\x9b");
    std::string STR_1 = /*table*/ scrypt("\x8c\x9f\x9e\x94\x9b");
    std::string STR_2 = /*os*/ scrypt("\x91\x8d");
    std::string STR_3 = /*string*/ scrypt("\x8d\x8c\x8e\x97\x92\x99");
    std::string STR_4 = /*math*/ scrypt("\x93\x9f\x8c\x98");
    std::string STR_5 = /*debug*/ scrypt("\x9c\x9b\x9e\x8b\x99");
    std::string STR_6 = /*utf8*/ scrypt("\x8b\x8c\x9a\xc8");
    std::string STR_7 = /*bit32*/ scrypt("\x9e\x97\x8c\xcd\xce");
    std::string STR_8 = /*buffer*/ scrypt("\x9e\x8b\x9a\x9a\x9b\x8e");
    std::string STR_9 = /*vector*/ scrypt("\x8a\x9b\x9d\x8c\x91\x8e"); 

    const luaL_Reg lualibs[] = {
        {"", luaopen_base},
        {STR_0.c_str(), luaopen_coroutine},
        {STR_1.c_str(), luaopen_table},
        {STR_2.c_str(), luaopen_os},
        {STR_3.c_str(), luaopen_string},
        {STR_4.c_str(), luaopen_math},
        {STR_5.c_str(), luaopen_debug},
        {STR_6.c_str(), luaopen_utf8},
        {STR_7.c_str(), luaopen_bit32},
        {STR_8.c_str(), luaopen_buffer},
	{STR_9.c_str(), luaopen_vector},
        {NULL, NULL},
    };

    const luaL_Reg* lib = lualibs;
    for (; lib->func; lib++)
    {
        lua_pushcfunction(L, lib->func, NULL);
        lua_pushstring(L, lib->name);
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
    /*__index*/ scrypt_def(STR_0, "\xa1\xa1\x97\x92\x9c\x9b\x88");

    // create new global table that proxies reads to original table
    lua_newtable(L);

    lua_newtable(L);
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setfield(L, -2, STR_0->c_str());
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
