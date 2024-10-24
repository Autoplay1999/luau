// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lualib.h"

#include "lobject.h"
#include "lstate.h"
#include "lstring.h"
#include "lapi.h"
#include "lgc.h"
#include "lnumutils.h"

#include <string.h>

// convert a stack index to positive
#define abs_index(L, i) ((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : lua_gettop(L) + (i) + 1)

/*
** {======================================================
** Error-report functions
** =======================================================
*/

static const char* currfuncname(lua_State* L)
{
    /*__namecall*/ scrypt_def(STR_0, "\xa1\xa1\x92\x9f\x93\x9b\x9d\x9f\x94\x94");

    Closure* cl = L->ci > L->base_ci ? curr_func(L) : NULL;
    const char* debugname = cl && cl->isC ? cl->c.debugname + 0 : NULL;

    if (debugname && strcmp(debugname, STR_0->c_str()) == 0)
        return L->namecall ? getstr(L->namecall) : NULL;
    else
        return debugname;
}

l_noret luaL_argerrorL(lua_State* L, int narg, const char* extramsg)
{
    const char* fname = currfuncname(L);

    if (fname) {
        #define STR_0 /*invalid argument #%d to '%s' (%s)*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x9f\x8e\x99\x8b\x93\x9b\x92\x8c\xe0\xdd\xdb\x9c\xe0\x8c\x91\xe0\xd9\xdb\x8d\xd9\xe0\xd8\xdb\x8d\xd7").c_str()
        luaL_error(L, STR_0, narg, fname, extramsg);
        #undef STR_0
    }
    else {
        #define STR_1 /*invalid argument #%d (%s)*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x9f\x8e\x99\x8b\x93\x9b\x92\x8c\xe0\xdd\xdb\x9c\xe0\xd8\xdb\x8d\xd7").c_str()
        luaL_error(L, STR_1, narg, extramsg);
        #undef STR_1
    }
}

l_noret luaL_typeerrorL(lua_State* L, int narg, const char* tname)
{
    const char* fname = currfuncname(L);
    const TValue* obj = luaA_toobject(L, narg);

    if (obj)
    {
        if (fname) {
            #define STR_0 /*invalid argument #%d to '%s' (%s expected, got %s)*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x9f\x8e\x99\x8b\x93\x9b\x92\x8c\xe0\xdd\xdb\x9c\xe0\x8c\x91\xe0\xd9\xdb\x8d\xd9\xe0\xd8\xdb\x8d\xe0\x9b\x88\x90\x9b\x9d\x8c\x9b\x9c\xd4\xe0\x99\x91\x8c\xe0\xdb\x8d\xd7").c_str()
            luaL_error(L, STR_0, narg, fname, tname, luaT_objtypename(L, obj));
            #undef STR_0
        }
        else {
            #define STR_1 /*invalid argument #%d (%s expected, got %s)*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x9f\x8e\x99\x8b\x93\x9b\x92\x8c\xe0\xdd\xdb\x9c\xe0\xd8\xdb\x8d\xe0\x9b\x88\x90\x9b\x9d\x8c\x9b\x9c\xd4\xe0\x99\x91\x8c\xe0\xdb\x8d\xd7").c_str()
            luaL_error(L, STR_1, narg, tname, luaT_objtypename(L, obj));
            #undef STR_1
        }
    }
    else
    {
        if (fname) {
            #define STR_2 /*missing argument #%d to '%s' (%s expected)*/ scrypt("\x93\x97\x8d\x8d\x97\x92\x99\xe0\x9f\x8e\x99\x8b\x93\x9b\x92\x8c\xe0\xdd\xdb\x9c\xe0\x8c\x91\xe0\xd9\xdb\x8d\xd9\xe0\xd8\xdb\x8d\xe0\x9b\x88\x90\x9b\x9d\x8c\x9b\x9c\xd7").c_str()
            luaL_error(L, STR_2, narg, fname, tname);
            #undef STR_2
        }
        else {
            #define STR_3 /*missing argument #%d (%s expected)*/ scrypt("\x93\x97\x8d\x8d\x97\x92\x99\xe0\x9f\x8e\x99\x8b\x93\x9b\x92\x8c\xe0\xdd\xdb\x9c\xe0\xd8\xdb\x8d\xe0\x9b\x88\x90\x9b\x9d\x8c\x9b\x9c\xd7").c_str()
            luaL_error(L, STR_3, narg, tname);
            #undef STR_3
        }
    }
}

static l_noret tag_error(lua_State* L, int narg, int tag)
{
    luaL_typeerrorL(L, narg, lua_typename(L, tag));
}

void luaL_where(lua_State* L, int level)
{
    /*sl*/ scrypt_def(STR_0, "\x8d\x94");

    lua_Debug ar;
    if (lua_getinfo(L, level, STR_0->c_str(), &ar) && ar.currentline > 0)
    {
        /*%s:%d: */ scrypt_def(STR_1, "\xdb\x8d\xc6\xdb\x9c\xc6\xe0");
        lua_pushfstring(L, STR_1->c_str(), ar.short_src, ar.currentline);
        return;
    }
    lua_pushliteral(L, ""); // else, no information available...
}

l_noret luaL_errorL(lua_State* L, const char* fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    luaL_where(L, 1);
    lua_pushvfstring(L, fmt, argp);
    va_end(argp);
    lua_concat(L, 2);
    lua_error(L);
}

// }======================================================

int luaL_checkoption(lua_State* L, int narg, const char* def, const char* const lst[])
{
    const char* name = (def) ? luaL_optstring(L, narg, def) : luaL_checkstring(L, narg);
    int i;
    for (i = 0; lst[i]; i++)
        if (strcmp(lst[i], name) == 0)
            return i;
    /*invalid option '%s'*/ scrypt_def(STR_0, "\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x91\x90\x8c\x97\x91\x92\xe0\xd9\xdb\x8d\xd9");
    const char* msg = lua_pushfstring(L, STR_0->c_str(), name);
    luaL_argerrorL(L, narg, msg);
}

int luaL_newmetatable(lua_State* L, const char* tname)
{
    lua_getfield(L, LUA_REGISTRYINDEX, tname); // get registry.name
    if (!lua_isnil(L, -1))                     // name already in use?
        return 0;                              // leave previous value on top, but return 0
    lua_pop(L, 1);
    lua_newtable(L); // create metatable
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, tname); // registry.name = metatable
    return 1;
}

void* luaL_checkudata(lua_State* L, int ud, const char* tname)
{
    void* p = lua_touserdata(L, ud);
    if (p != NULL)
    { // value is a userdata?
        if (lua_getmetatable(L, ud))
        {                                              // does it have a metatable?
            lua_getfield(L, LUA_REGISTRYINDEX, tname); // get correct metatable
            if (lua_rawequal(L, -1, -2))
            {                  // does it have the correct mt?
                lua_pop(L, 2); // remove both metatables
                return p;
            }
        }
    }
    luaL_typeerrorL(L, ud, tname); // else error
}

void* luaL_checkbuffer(lua_State* L, int narg, size_t* len)
{
    void* b = lua_tobuffer(L, narg, len);
    if (!b)
        tag_error(L, narg, LUA_TBUFFER);
    return b;
}

void luaL_checkstack(lua_State* L, int space, const char* mes)
{
    if (!lua_checkstack(L, space)) {
        #define STR_0 /*stack overflow (%s)*/ scrypt("\x8d\x8c\x9f\x9d\x95\xe0\x91\x8a\x9b\x8e\x9a\x94\x91\x89\xe0\xd8\xdb\x8d\xd7").c_str()
        luaL_error(L, STR_0, mes);
        #undef STR_0
    }
}

void luaL_checktype(lua_State* L, int narg, int t)
{
    if (lua_type(L, narg) != t)
        tag_error(L, narg, t);
}

void luaL_checkany(lua_State* L, int narg)
{
    if (lua_type(L, narg) == LUA_TNONE) {
        #define STR_0 /*missing argument #%d*/ scrypt("\x93\x97\x8d\x8d\x97\x92\x99\xe0\x9f\x8e\x99\x8b\x93\x9b\x92\x8c\xe0\xdd\xdb\x9c").c_str()
        luaL_error(L, STR_0, narg);
        #undef STR_0
    }
}

const char* luaL_checklstring(lua_State* L, int narg, size_t* len)
{
    const char* s = lua_tolstring(L, narg, len);
    if (!s)
        tag_error(L, narg, LUA_TSTRING);
    return s;
}

const char* luaL_optlstring(lua_State* L, int narg, const char* def, size_t* len)
{
    if (lua_isnoneornil(L, narg))
    {
        if (len)
            *len = (def ? strlen(def) : 0);
        return def;
    }
    else
        return luaL_checklstring(L, narg, len);
}

double luaL_checknumber(lua_State* L, int narg)
{
    int isnum;
    double d = lua_tonumberx(L, narg, &isnum);
    if (!isnum)
        tag_error(L, narg, LUA_TNUMBER);
    return d;
}

double luaL_optnumber(lua_State* L, int narg, double def)
{
    return luaL_opt(L, luaL_checknumber, narg, def);
}

int luaL_checkboolean(lua_State* L, int narg)
{
    // This checks specifically for boolean values, ignoring
    // all other truthy/falsy values. If the desired result
    // is true if value is present then lua_toboolean should
    // directly be used instead.
    if (!lua_isboolean(L, narg))
        tag_error(L, narg, LUA_TBOOLEAN);
    return lua_toboolean(L, narg);
}

int luaL_optboolean(lua_State* L, int narg, int def)
{
    return luaL_opt(L, luaL_checkboolean, narg, def);
}

int luaL_checkinteger(lua_State* L, int narg)
{
    int isnum;
    int d = lua_tointegerx(L, narg, &isnum);
    if (!isnum)
        tag_error(L, narg, LUA_TNUMBER);
    return d;
}

int luaL_optinteger(lua_State* L, int narg, int def)
{
    return luaL_opt(L, luaL_checkinteger, narg, def);
}

unsigned luaL_checkunsigned(lua_State* L, int narg)
{
    int isnum;
    unsigned d = lua_tounsignedx(L, narg, &isnum);
    if (!isnum)
        tag_error(L, narg, LUA_TNUMBER);
    return d;
}

unsigned luaL_optunsigned(lua_State* L, int narg, unsigned def)
{
    return luaL_opt(L, luaL_checkunsigned, narg, def);
}

const float* luaL_checkvector(lua_State* L, int narg)
{
    const float* v = lua_tovector(L, narg);
    if (!v)
        tag_error(L, narg, LUA_TVECTOR);
    return v;
}

const float* luaL_optvector(lua_State* L, int narg, const float* def)
{
    return luaL_opt(L, luaL_checkvector, narg, def);
}

int luaL_getmetafield(lua_State* L, int obj, const char* event)
{
    if (!lua_getmetatable(L, obj)) // no metatable?
        return 0;
    lua_pushstring(L, event);
    lua_rawget(L, -2);
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 2); // remove metatable and metafield
        return 0;
    }
    else
    {
        lua_remove(L, -2); // remove only metatable
        return 1;
    }
}

int luaL_callmeta(lua_State* L, int obj, const char* event)
{
    obj = abs_index(L, obj);
    if (!luaL_getmetafield(L, obj, event)) // no metafield?
        return 0;
    lua_pushvalue(L, obj);
    lua_call(L, 1, 1);
    return 1;
}

static int libsize(const luaL_Reg* l)
{
    int size = 0;
    for (; l->name; l++)
        size++;
    return size;
}

void luaL_register(lua_State* L, const char* libname, const luaL_Reg* l)
{
    if (libname)
    {
        /*_LOADED*/ scrypt_def(STR_0, "\xa1\xb4\xb1\xbf\xbc\xbb\xbc");
        int size = libsize(l);
        // check whether lib already exists
        luaL_findtable(L, LUA_REGISTRYINDEX, STR_0->c_str(), 1);
        lua_getfield(L, -1, libname); // get _LOADED[libname]
        if (!lua_istable(L, -1))
        {                  // not found?
            lua_pop(L, 1); // remove previous result
            // try global variable (and create one if it does not exist)
            if (luaL_findtable(L, LUA_GLOBALSINDEX, libname, size) != NULL) {
                #define STR_1 /*name conflict for module '%s'*/ scrypt("\x92\x9f\x93\x9b\xe0\x9d\x91\x92\x9a\x94\x97\x9d\x8c\xe0\x9a\x91\x8e\xe0\x93\x91\x9c\x8b\x94\x9b\xe0\xd9\xdb\x8d\xd9").c_str()
                luaL_error(L, STR_1, libname);
                #undef STR_1
            }
            lua_pushvalue(L, -1);
            lua_setfield(L, -3, libname); // _LOADED[libname] = new table
        }
        lua_remove(L, -2); // remove _LOADED table
    }
    for (; l->name; l++)
    {
        lua_pushcfunction(L, l->func, l->name);
        lua_setfield(L, -2, l->name);
    }
}

const char* luaL_findtable(lua_State* L, int idx, const char* fname, int szhint)
{
    const char* e;
    lua_pushvalue(L, idx);
    do
    {
        e = strchr(fname, '.');
        if (e == NULL)
            e = fname + strlen(fname);
        lua_pushlstring(L, fname, e - fname);
        lua_rawget(L, -2);
        if (lua_isnil(L, -1))
        {                                                    // no such field?
            lua_pop(L, 1);                                   // remove this nil
            lua_createtable(L, 0, (*e == '.' ? 1 : szhint)); // new table for field
            lua_pushlstring(L, fname, e - fname);
            lua_pushvalue(L, -2);
            lua_settable(L, -4); // set new table into field
        }
        else if (!lua_istable(L, -1))
        {                  // field has a non-table value?
            lua_pop(L, 2); // remove table and value
            return fname;  // return problematic part of the name
        }
        lua_remove(L, -2); // remove previous table
        fname = e + 1;
    } while (*e == '.');
    return NULL;
}

const char* luaL_typename(lua_State* L, int idx)
{
    /*no value*/ scrypt_def(STR_0, "\x92\x91\xe0\x8a\x9f\x94\x8b\x9b");
    const TValue* obj = luaA_toobject(L, idx);
    return obj ? luaT_objtypename(L, obj) : STR_0->c_str();
}

/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/

static size_t getnextbuffersize(lua_State* L, size_t currentsize, size_t desiredsize)
{
    size_t newsize = currentsize + currentsize / 2;

    // check for size overflow
    if (SIZE_MAX - desiredsize < currentsize) {
        #define STR_0 /*buffer too large*/ scrypt("\x9e\x8b\x9a\x9a\x9b\x8e\xe0\x8c\x91\x91\xe0\x94\x9f\x8e\x99\x9b").c_str()
        luaL_error(L, STR_0);
        #undef STR_0
    }

    // growth factor might not be enough to satisfy the desired size
    if (newsize < desiredsize)
        newsize = desiredsize;

    return newsize;
}

static char* extendstrbuf(luaL_Strbuf* B, size_t additionalsize, int boxloc)
{
    lua_State* L = B->L;

    if (B->storage)
        LUAU_ASSERT(B->storage == tsvalue(L->top + boxloc));

    char* base = B->storage ? B->storage->data : B->buffer;

    size_t capacity = B->end - base;
    size_t nextsize = getnextbuffersize(B->L, capacity, capacity + additionalsize);

    TString* newStorage = luaS_bufstart(L, nextsize);

    memcpy(newStorage->data, base, B->p - base);

    // place the string storage at the expected position in the stack
    if (base == B->buffer)
    {
        lua_pushnil(L);
        lua_insert(L, boxloc);
    }

    setsvalue(L, L->top + boxloc, newStorage);
    B->p = newStorage->data + (B->p - base);
    B->end = newStorage->data + nextsize;
    B->storage = newStorage;

    return B->p;
}

void luaL_buffinit(lua_State* L, luaL_Strbuf* B)
{
    // start with an internal buffer
    B->p = B->buffer;
    B->end = B->p + LUA_BUFFERSIZE;

    B->L = L;
    B->storage = nullptr;
}

char* luaL_buffinitsize(lua_State* L, luaL_Strbuf* B, size_t size)
{
    luaL_buffinit(L, B);
    return luaL_prepbuffsize(B, size);
}

char* luaL_prepbuffsize(luaL_Strbuf* B, size_t size)
{
    if (size_t(B->end - B->p) < size)
        return extendstrbuf(B, size - (B->end - B->p), -1);
    return B->p;
}

void luaL_addlstring(luaL_Strbuf* B, const char* s, size_t len)
{
    if (size_t(B->end - B->p) < len)
        extendstrbuf(B, len - (B->end - B->p), -1);

    memcpy(B->p, s, len);
    B->p += len;
}

void luaL_addvalue(luaL_Strbuf* B)
{
    lua_State* L = B->L;

    size_t vl;
    if (const char* s = lua_tolstring(L, -1, &vl))
    {
        if (size_t(B->end - B->p) < vl)
            extendstrbuf(B, vl - (B->end - B->p), -2);

        memcpy(B->p, s, vl);
        B->p += vl;

        lua_pop(L, 1);
    }
}

void luaL_addvalueany(luaL_Strbuf* B, int idx)
{
    lua_State* L = B->L;

    switch (lua_type(L, idx))
    {
    case LUA_TNONE:
    {
        LUAU_ASSERT(!"expected value");
        break;
    }
    case LUA_TNIL: {
        /*nil*/ scrypt_def(STR_0, "\x92\x97\x94");
        luaL_addstring(B, STR_0->c_str());
        break;
    }
    case LUA_TBOOLEAN:
        if (lua_toboolean(L, idx)) {
            /*true*/ scrypt_def(STR_1, "\x8c\x8e\x8b\x9b");
            luaL_addstring(B, STR_1->c_str());
        }
        else {
            /*false*/ scrypt_def(STR_2, "\x9a\x9f\x94\x8d\x9b");
            luaL_addstring(B, STR_2->c_str());
        }
        break;
    case LUA_TNUMBER:
    {
        double n = lua_tonumber(L, idx);
        char s[LUAI_MAXNUM2STR];
        char* e = luai_num2str(s, n);
        luaL_addlstring(B, s, e - s);
        break;
    }
    case LUA_TSTRING:
    {
        size_t len;
        const char* s = lua_tolstring(L, idx, &len);
        luaL_addlstring(B, s, len);
        break;
    }
    default:
    {
        size_t len;
        luaL_tolstring(L, idx, &len);

        // note: luaL_addlstring assumes box is stored at top of stack, so we can't call it here
        // instead we use luaL_addvalue which will take the string from the top of the stack and add that
        luaL_addvalue(B);
    }
    }
}

void luaL_pushresult(luaL_Strbuf* B)
{
    lua_State* L = B->L;

    if (TString* storage = B->storage)
    {
        luaC_checkGC(L);

        // if we finished just at the end of the string buffer, we can convert it to a mutable stirng without a copy
        if (B->p == B->end)
        {
            setsvalue(L, L->top - 1, luaS_buffinish(L, storage));
        }
        else
        {
            setsvalue(L, L->top - 1, luaS_newlstr(L, storage->data, B->p - storage->data));
        }
    }
    else
    {
        lua_pushlstring(L, B->buffer, B->p - B->buffer);
    }
}

void luaL_pushresultsize(luaL_Strbuf* B, size_t size)
{
    B->p += size;
    luaL_pushresult(B);
}

// }======================================================

const char* luaL_tolstring(lua_State* L, int idx, size_t* len)
{
    /*__tostring*/ scrypt_def(STR_0, "\xa1\xa1\x8c\x91\x8d\x8c\x8e\x97\x92\x99");

    if (luaL_callmeta(L, idx, STR_0->c_str())) // is there a metafield?
    {
        const char* s = lua_tolstring(L, -1, len);
        if (!s) {
            #define STR_1 /*'__tostring' must return a string*/ scrypt("\xd9\xa1\xa1\x8c\x91\x8d\x8c\x8e\x97\x92\x99\xd9\xe0\x93\x8b\x8d\x8c\xe0\x8e\x9b\x8c\x8b\x8e\x92\xe0\x9f\xe0\x8d\x8c\x8e\x97\x92\x99").c_str()
            luaL_error(L, STR_1);
            #undef STR_1
        }
        return s;
    }

    switch (lua_type(L, idx))
    {
    case LUA_TNIL: {
        /*nil*/ scrypt_def(STR_2, "\x92\x97\x94");
        lua_pushlstring(L, STR_2->c_str(), 3);
        break;
    }
    case LUA_TBOOLEAN: {
        /*true*/ scrypt_def(STR_3, "\x8c\x8e\x8b\x9b");
        /*false*/ scrypt_def(STR_4, "\x9a\x9f\x94\x8d\x9b");
        lua_pushstring(L, (lua_toboolean(L, idx) ? STR_3->c_str() : STR_4->c_str()));
        break;
    }
    case LUA_TNUMBER:
    {
        double n = lua_tonumber(L, idx);
        char s[LUAI_MAXNUM2STR];
        char* e = luai_num2str(s, n);
        lua_pushlstring(L, s, e - s);
        break;
    }
    case LUA_TVECTOR:
    {
        const float* v = lua_tovector(L, idx);

        char s[LUAI_MAXNUM2STR * LUA_VECTOR_SIZE];
        char* e = s;
        for (int i = 0; i < LUA_VECTOR_SIZE; ++i)
        {
            if (i != 0)
            {
                *e++ = ',';
                *e++ = ' ';
            }
            e = luai_num2str(e, v[i]);
        }
        lua_pushlstring(L, s, e - s);
        break;
    }
    case LUA_TSTRING:
        lua_pushvalue(L, idx);
        break;
    default:
    {
        /*%s: 0x%016llx*/ scrypt_def(STR_5, "\xdb\x8d\xc6\xe0\xd0\x88\xdb\xd0\xcf\xca\x94\x94\x88");
        const void* ptr = lua_topointer(L, idx);
        unsigned long long enc = lua_encodepointer(L, uintptr_t(ptr));
        lua_pushfstring(L, STR_5->c_str(), luaL_typename(L, idx), enc);
        break;
    }
    }
    return lua_tolstring(L, -1, len);
}
