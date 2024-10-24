// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lualib.h"

#include "ldebug.h"
#include "lstate.h"
#include "lvm.h"

#include <vector>

LUAU_DYNAMIC_FASTFLAGVARIABLE(LuauCoroCheckStack, false)
LUAU_DYNAMIC_FASTFLAG(LuauStackLimit)

#define CO_STATUS_ERROR -1
#define CO_STATUS_BREAK -2

// static const char* const statnames[] = {"running", "suspended", "normal", "dead", "dead"}; // dead appears twice for LUA_COERR and LUA_COFIN

static std::string& statnames(int i) {
    static std::shared_ptr<std::vector<std::string>> v;
    if (!v) {
        std::string STR_0 = /*running*/ scrypt("\x8e\x8b\x92\x92\x97\x92\x99");
        std::string STR_1 = /*suspended*/ scrypt("\x8d\x8b\x8d\x90\x9b\x92\x9c\x9b\x9c");
        std::string STR_2 = /*normal*/ scrypt("\x92\x91\x8e\x93\x9f\x94");
        std::string STR_3 = /*dead*/ scrypt("\x9c\x9b\x9f\x9c");
        v = std::make_shared<std::vector<std::string>>();
        v->push_back(STR_0);
        v->push_back(STR_1);
        v->push_back(STR_2);
        v->push_back(STR_3);
        v->push_back(STR_3);
    }
    return (*v)[i];
}

static int costatus(lua_State* L)
{
    /*thread*/ scrypt_def(STR_0, "\x8c\x98\x8e\x9b\x9f\x9c")

    lua_State* co = lua_tothread(L, 1);
    luaL_argexpected(L, co, 1, STR_0->c_str());
    lua_pushstring(L, statnames(lua_costatus(L, co)).c_str());
    return 1;
}

static int auxresume(lua_State* L, lua_State* co, int narg)
{
    // error handling for edge cases
    if (co->status != LUA_YIELD)
    {
        int status = lua_costatus(L, co);
        if (status != LUA_COSUS)
        {
            #define STR_0 /*cannot resume %s coroutine*/ scrypt("\x9d\x9f\x92\x92\x91\x8c\xe0\x8e\x9b\x8d\x8b\x93\x9b\xe0\xdb\x8d\xe0\x9d\x91\x8e\x91\x8b\x8c\x97\x92\x9b").c_str()
            lua_pushfstring(L, STR_0, statnames(status).c_str());
            #undef STR_0
            return CO_STATUS_ERROR;
        }
    }

    if (narg)
    {
        if (!lua_checkstack(co, narg)) {
            #define STR_1 /*too many arguments to resume*/ scrypt("\x8c\x91\x91\xe0\x93\x9f\x92\x87\xe0\x9f\x8e\x99\x8b\x93\x9b\x92\x8c\x8d\xe0\x8c\x91\xe0\x8e\x9b\x8d\x8b\x93\x9b").c_str()
            luaL_error(L, STR_1);
            #undef STR_1
        }
        lua_xmove(L, co, narg);
    }
    else if (DFFlag::LuauCoroCheckStack)
    {
        // coroutine might be completely full already
        if ((co->top - co->base) > LUAI_MAXCSTACK)
            luaL_error(L, "too many arguments to resume");
    }

    co->singlestep = L->singlestep;

    int status = lua_resume(co, L, narg);
    if (status == 0 || status == LUA_YIELD)
    {
        int nres = cast_int(co->top - co->base);
        if (nres)
        {
            // +1 accounts for true/false status in resumefinish
            if (nres + 1 > LUA_MINSTACK && !lua_checkstack(L, nres + 1)) {
                #define STR_2 /*too many results to resume*/ scrypt("\x8c\x91\x91\xe0\x93\x9f\x92\x87\xe0\x8e\x9b\x8d\x8b\x94\x8c\x8d\xe0\x8c\x91\xe0\x8e\x9b\x8d\x8b\x93\x9b").c_str()
                luaL_error(L, STR_2);
                #undef STR_2
            }
            lua_xmove(co, L, nres); // move yielded values
        }
        return nres;
    }
    else if (status == LUA_BREAK)
    {
        return CO_STATUS_BREAK;
    }
    else
    {
        lua_xmove(co, L, 1); // move error message
        return CO_STATUS_ERROR;
    }
}

static int interruptThread(lua_State* L, lua_State* co)
{
    // notify the debugger that the thread was suspended
    if (L->global->cb.debuginterrupt)
        luau_callhook(L, L->global->cb.debuginterrupt, co);

    return lua_break(L);
}

static int auxresumecont(lua_State* L, lua_State* co)
{
    if (co->status == 0 || co->status == LUA_YIELD)
    {
        int nres = cast_int(co->top - co->base);
        if (!lua_checkstack(L, nres + 1)) {
            #define STR_0 /*too many results to resume*/ scrypt("\x8c\x91\x91\xe0\x93\x9f\x92\x87\xe0\x8e\x9b\x8d\x8b\x94\x8c\x8d\xe0\x8c\x91\xe0\x8e\x9b\x8d\x8b\x93\x9b").c_str()
            luaL_error(L, STR_0);
            #undef STR_0
        }
        lua_xmove(co, L, nres); // move yielded values
        return nres;
    }
    else
    {
        lua_rawcheckstack(L, 2);
        lua_xmove(co, L, 1); // move error message
        return CO_STATUS_ERROR;
    }
}

static int coresumefinish(lua_State* L, int r)
{
    if (r < 0)
    {
        lua_pushboolean(L, 0);
        lua_insert(L, -2);
        return 2; // return false + error message
    }
    else
    {
        lua_pushboolean(L, 1);
        lua_insert(L, -(r + 1));
        return r + 1; // return true + `resume' returns
    }
}

static int coresumey(lua_State* L)
{
    /*thread*/ scrypt_def(STR_0, "\x8c\x98\x8e\x9b\x9f\x9c");

    lua_State* co = lua_tothread(L, 1);
    luaL_argexpected(L, co, 1, STR_0->c_str());
    int narg = cast_int(L->top - L->base) - 1;
    int r = auxresume(L, co, narg);

    if (r == CO_STATUS_BREAK)
        return interruptThread(L, co);

    return coresumefinish(L, r);
}

static int coresumecont(lua_State* L, int status)
{
    /*thread*/ scrypt_def(STR_0, "\x8c\x98\x8e\x9b\x9f\x9c");

    lua_State* co = lua_tothread(L, 1);
    luaL_argexpected(L, co, 1, STR_0->c_str());

    // if coroutine still hasn't yielded after the break, break current thread again
    if (co->status == LUA_BREAK)
        return interruptThread(L, co);

    int r = auxresumecont(L, co);

    return coresumefinish(L, r);
}

static int auxwrapfinish(lua_State* L, int r)
{
    if (r < 0)
    {
        if (lua_isstring(L, -1))
        {                     // error object is a string?
            luaL_where(L, 1); // add extra info
            lua_insert(L, -2);
            lua_concat(L, 2);
        }
        lua_error(L); // propagate error
    }
    return r;
}

static int auxwrapy(lua_State* L)
{
    lua_State* co = lua_tothread(L, lua_upvalueindex(1));
    int narg = cast_int(L->top - L->base);
    int r = auxresume(L, co, narg);

    if (r == CO_STATUS_BREAK)
        return interruptThread(L, co);

    return auxwrapfinish(L, r);
}

static int auxwrapcont(lua_State* L, int status)
{
    lua_State* co = lua_tothread(L, lua_upvalueindex(1));

    // if coroutine still hasn't yielded after the break, break current thread again
    if (co->status == LUA_BREAK)
        return interruptThread(L, co);

    int r = auxresumecont(L, co);

    return auxwrapfinish(L, r);
}

static int cocreate(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_State* NL = lua_newthread(L);
    lua_xpush(L, NL, 1); // push function on top of NL
    return 1;
}

static int cowrap(lua_State* L)
{
    cocreate(L);

    lua_pushcclosurek(L, auxwrapy, NULL, 1, auxwrapcont);
    return 1;
}

static int coyield(lua_State* L)
{
    int nres = cast_int(L->top - L->base);
    return lua_yield(L, nres);
}

static int corunning(lua_State* L)
{
    if (lua_pushthread(L))
        lua_pushnil(L); // main thread is not a coroutine
    return 1;
}

static int coyieldable(lua_State* L)
{
    lua_pushboolean(L, lua_isyieldable(L));
    return 1;
}

static int coclose(lua_State* L)
{
    /*thread*/ scrypt_def(STR_0, "\x8c\x98\x8e\x9b\x9f\x9c");

    lua_State* co = lua_tothread(L, 1);
    luaL_argexpected(L, co, 1, STR_0->c_str());

    int status = lua_costatus(L, co);
    if (status != LUA_COFIN && status != LUA_COERR && status != LUA_COSUS) {
        #define STR_1 /*cannot close %s coroutine*/ scrypt("\x9d\x9f\x92\x92\x91\x8c\xe0\x9d\x94\x91\x8d\x9b\xe0\xdb\x8d\xe0\x9d\x91\x8e\x91\x8b\x8c\x97\x92\x9b").c_str()
        luaL_error(L, STR_1, statnames(status).c_str());
        #undef STR_1
    }

    if (co->status == LUA_OK || co->status == LUA_YIELD)
    {
        lua_pushboolean(L, true);
        lua_resetthread(co);
        return 1;
    }
    else
    {
        lua_pushboolean(L, false);

        if (DFFlag::LuauStackLimit)
        {
            if (co->status == LUA_ERRMEM)
                lua_pushstring(L, LUA_MEMERRMSG);
            else if (co->status == LUA_ERRERR)
                lua_pushstring(L, LUA_ERRERRMSG);
            else if (lua_gettop(co))
                lua_xmove(co, L, 1); // move error message
        }
        else
        {
            if (lua_gettop(co))
                lua_xmove(co, L, 1); // move error message
        }

        lua_resetthread(co);
        return 2;
    }
}

int luaopen_coroutine(lua_State* L)
{
    std::string STR_0 = /*create*/ scrypt("\x9d\x8e\x9b\x9f\x8c\x9b");
    std::string STR_1 = /*running*/ scrypt("\x8e\x8b\x92\x92\x97\x92\x99");
    std::string STR_2 = /*status*/ scrypt("\x8d\x8c\x9f\x8c\x8b\x8d");
    std::string STR_3 = /*wrap*/ scrypt("\x89\x8e\x9f\x90");
    std::string STR_4 = /*yield*/ scrypt("\x87\x97\x9b\x94\x9c");
    std::string STR_5 = /*isyieldable*/ scrypt("\x97\x8d\x87\x97\x9b\x94\x9c\x9f\x9e\x94\x9b");
    std::string STR_6 = /*close*/ scrypt("\x9d\x94\x91\x8d\x9b");
    std::string STR_7 = /*resume*/ scrypt("\x8e\x9b\x8d\x8b\x93\x9b");
    std::string STR_8 = /*coroutine*/ scrypt("\x9d\x91\x8e\x91\x8b\x8c\x97\x92\x9b");

    const luaL_Reg co_funcs[] = {
        {STR_0.c_str(), cocreate},
        {STR_1.c_str(), corunning},
        {STR_2.c_str(), costatus},
        {STR_3.c_str(), cowrap},
        {STR_4.c_str(), coyield},
        {STR_5.c_str(), coyieldable},
        {STR_6.c_str(), coclose},
        {NULL, NULL},
    };

    luaL_register(L, STR_8.c_str(), co_funcs);

    lua_pushcclosurek(L, coresumey, STR_7.c_str(), 0, coresumecont);
    lua_setfield(L, -2, STR_7.c_str());

    return 1;
}
