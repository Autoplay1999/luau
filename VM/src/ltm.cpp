// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "ltm.h"

#include "lstate.h"
#include "lstring.h"
#include "ludata.h"
#include "ltable.h"
#include "lgc.h"

#include <string.h>

// clang-format off
/*const char* const luaT_typenames[] = {
    // ORDER TYPE
    "nil",
    "boolean",

    
    "userdata",
    "number",
    "vector",

    "string",

    
    "table",
    "function",
    "userdata",
    "thread",
    "buffer",
};*/

const char* const luaT_typenames(int i) {
#define STR_0 /*nil*/ scrypt("\x92\x97\x94")
#define STR_1 /*boolean*/ scrypt("\x9e\x91\x91\x94\x9b\x9f\x92")
#define STR_2 /*userdata*/ scrypt("\x8b\x8d\x9b\x8e\x9c\x9f\x8c\x9f")
#define STR_3 /*number*/ scrypt("\x92\x8b\x93\x9e\x9b\x8e")
#define STR_4 /*vector*/ scrypt("\x8a\x9b\x9d\x8c\x91\x8e")
#define STR_5 /*string*/ scrypt("\x8d\x8c\x8e\x97\x92\x99")
#define STR_6 /*table*/ scrypt("\x8c\x9f\x9e\x94\x9b")
#define STR_7 /*function*/ scrypt("\x9a\x8b\x92\x9d\x8c\x97\x91\x92")
#define STR_8 /*userdata*/ scrypt("\x8b\x8d\x9b\x8e\x9c\x9f\x8c\x9f")
#define STR_9 /*thread*/ scrypt("\x8c\x98\x8e\x9b\x9f\x9c")
#define STR_10 /*buffer*/ scrypt("\x9e\x8b\x9a\x9a\x9b\x8e")

    static std::shared_ptr<std::vector<std::string>> _luaT_typenames;

    if (!_luaT_typenames) {
        _luaT_typenames = std::make_shared<std::vector<std::string>>();
        _luaT_typenames->push_back(STR_0);
        _luaT_typenames->push_back(STR_1);
        _luaT_typenames->push_back(STR_2);
        _luaT_typenames->push_back(STR_3);
        _luaT_typenames->push_back(STR_4);
        _luaT_typenames->push_back(STR_5);
        _luaT_typenames->push_back(STR_6);
        _luaT_typenames->push_back(STR_7);
        _luaT_typenames->push_back(STR_8);
        _luaT_typenames->push_back(STR_9);
        _luaT_typenames->push_back(STR_10);
    }

    return (*_luaT_typenames)[i].c_str();

#undef STR_0
#undef STR_1
#undef STR_2
#undef STR_3
#undef STR_4
#undef STR_5
#undef STR_6
#undef STR_7
#undef STR_8
#undef STR_9
#undef STR_10
}

/*const char* const luaT_eventname[] = {
    // ORDER TM
    
    "__index",
    "__newindex",
    "__mode",
    "__namecall",
    "__call",
    "__iter",
    "__len",

    "__eq",

    
    "__add",
    "__sub",
    "__mul",
    "__div",
    "__idiv",
    "__mod",
    "__pow",
    "__unm",

    
    "__lt",
    "__le",
    "__concat",
    "__type",
    "__metatable",
};*/

const char* const luaT_eventname(int i) {
#define STR_0 /*__index*/ scrypt("\xa1\xa1\x97\x92\x9c\x9b\x88")
#define STR_1 /*__newindex*/ scrypt("\xa1\xa1\x92\x9b\x89\x97\x92\x9c\x9b\x88")
#define STR_2 /*__mode*/ scrypt("\xa1\xa1\x93\x91\x9c\x9b")
#define STR_3 /*__namecall*/ scrypt("\xa1\xa1\x92\x9f\x93\x9b\x9d\x9f\x94\x94")
#define STR_4 /*__call*/ scrypt("\xa1\xa1\x9d\x9f\x94\x94")
#define STR_5 /*__iter*/ scrypt("\xa1\xa1\x97\x8c\x9b\x8e")
#define STR_6 /*__len*/ scrypt("\xa1\xa1\x94\x9b\x92")
#define STR_7 /*__eq*/ scrypt("\xa1\xa1\x9b\x8f")
#define STR_8 /*__add*/ scrypt("\xa1\xa1\x9f\x9c\x9c")
#define STR_9 /*__sub*/ scrypt("\xa1\xa1\x8d\x8b\x9e")
#define STR_10 /*__mul*/ scrypt("\xa1\xa1\x93\x8b\x94")
#define STR_11 /*__div*/ scrypt("\xa1\xa1\x9c\x97\x8a")
#define STR_12 /*__idiv*/ scrypt("\xa1\xa1\x97\x9c\x97\x8a")
#define STR_13 /*__mod*/ scrypt("\xa1\xa1\x93\x91\x9c")
#define STR_14 /*__pow*/ scrypt("\xa1\xa1\x90\x91\x89")
#define STR_15 /*__unm*/ scrypt("\xa1\xa1\x8b\x92\x93")
#define STR_16 /*__lt*/ scrypt("\xa1\xa1\x94\x8c")
#define STR_17 /*__le*/ scrypt("\xa1\xa1\x94\x9b")
#define STR_18 /*__concat*/ scrypt("\xa1\xa1\x9d\x91\x92\x9d\x9f\x8c")
#define STR_19 /*__type*/ scrypt("\xa1\xa1\x8c\x87\x90\x9b")
#define STR_20 /*__metatable*/ scrypt("\xa1\xa1\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b")

    static std::shared_ptr<std::vector<std::string>> _luaT_eventname;

    if (!_luaT_eventname) {
        _luaT_eventname = std::make_shared<std::vector<std::string>>();
        _luaT_eventname->push_back(STR_0);
        _luaT_eventname->push_back(STR_1);
        _luaT_eventname->push_back(STR_2);
        _luaT_eventname->push_back(STR_3);
        _luaT_eventname->push_back(STR_4);
        _luaT_eventname->push_back(STR_5);
        _luaT_eventname->push_back(STR_6);
        _luaT_eventname->push_back(STR_7);
        _luaT_eventname->push_back(STR_8);
        _luaT_eventname->push_back(STR_9);
        _luaT_eventname->push_back(STR_10);
        _luaT_eventname->push_back(STR_11);
        _luaT_eventname->push_back(STR_12);
        _luaT_eventname->push_back(STR_13);
        _luaT_eventname->push_back(STR_14);
        _luaT_eventname->push_back(STR_15);
        _luaT_eventname->push_back(STR_16);
        _luaT_eventname->push_back(STR_17);
        _luaT_eventname->push_back(STR_18);
        _luaT_eventname->push_back(STR_19);
        _luaT_eventname->push_back(STR_20);
    }

    return (*_luaT_eventname)[i].c_str();

#undef STR_0
#undef STR_1
#undef STR_2
#undef STR_3
#undef STR_4
#undef STR_5
#undef STR_6
#undef STR_7
#undef STR_8
#undef STR_9
#undef STR_10
#undef STR_11
#undef STR_12
#undef STR_13
#undef STR_14
#undef STR_15
#undef STR_16
#undef STR_17
#undef STR_18
#undef STR_19
#undef STR_20
}
// clang-format on

//static_assert(sizeof(luaT_typenames) / sizeof(luaT_typenames[0]) == LUA_T_COUNT, "luaT_typenames size mismatch");
//static_assert(sizeof(luaT_eventname) / sizeof(luaT_eventname[0]) == TM_N, "luaT_eventname size mismatch");
static_assert(TM_EQ < 8, "fasttm optimization stores a bitfield with metamethods in a byte");

void luaT_init(lua_State* L)
{
    int i;
    for (i = 0; i < LUA_T_COUNT; i++)
    {
        L->global->ttname[i] = luaS_new(L, luaT_typenames(i));
        luaS_fix(L->global->ttname[i]); // never collect these names
    }
    for (i = 0; i < TM_N; i++)
    {
        L->global->tmname[i] = luaS_new(L, luaT_eventname(i));
        luaS_fix(L->global->tmname[i]); // never collect these names
    }
}

/*
** function to be used with macro "fasttm": optimized for absence of
** tag methods.
*/
const TValue* luaT_gettm(Table* events, TMS event, TString* ename)
{
    const TValue* tm = luaH_getstr(events, ename);
    LUAU_ASSERT(event <= TM_EQ);
    if (ttisnil(tm))
    {                                              // no tag method?
        events->tmcache |= cast_byte(1u << event); // cache this fact
        return NULL;
    }
    else
        return tm;
}

const TValue* luaT_gettmbyobj(lua_State* L, const TValue* o, TMS event)
{
    /*
      NB: Tag-methods were replaced by meta-methods in Lua 5.0, but the
      old names are still around (this function, for example).
    */
    Table* mt;
    switch (ttype(o))
    {
    case LUA_TTABLE:
        mt = hvalue(o)->metatable;
        break;
    case LUA_TUSERDATA:
        mt = uvalue(o)->metatable;
        break;
    default:
        mt = L->global->mt[ttype(o)];
    }
    return (mt ? luaH_getstr(mt, L->global->tmname[event]) : luaO_nilobject);
}

const TString* luaT_objtypenamestr(lua_State* L, const TValue* o)
{
    // Userdata created by the environment can have a custom type name set in the individual metatable
    // If there is no custom name, 'userdata' is returned
    if (ttisuserdata(o) && uvalue(o)->tag != UTAG_PROXY && uvalue(o)->metatable)
    {
        const TValue* type = luaH_getstr(uvalue(o)->metatable, L->global->tmname[TM_TYPE]);

        if (ttisstring(type))
            return tsvalue(type);

        return L->global->ttname[ttype(o)];
    }

    // Tagged lightuserdata can be named using lua_setlightuserdataname
    if (ttislightuserdata(o))
    {
        int tag = lightuserdatatag(o);

        if (unsigned(tag) < LUA_LUTAG_LIMIT)
        {
            if (const TString* name = L->global->lightuserdataname[tag])
                return name;
        }
    }

    // For all types except userdata and table, a global metatable can be set with a global name override
    if (Table* mt = L->global->mt[ttype(o)])
    {
        const TValue* type = luaH_getstr(mt, L->global->tmname[TM_TYPE]);

        if (ttisstring(type))
            return tsvalue(type);
    }

    return L->global->ttname[ttype(o)];
}

const char* luaT_objtypename(lua_State* L, const TValue* o)
{
    return getstr(luaT_objtypenamestr(L, o));
}
