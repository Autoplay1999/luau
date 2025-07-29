// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lualib.h"

#include "lapi.h"
#include "lnumutils.h"
#include "lstate.h"
#include "ltable.h"
#include "lstring.h"
#include "lgc.h"
#include "ldebug.h"
#include "lvm.h"

static int foreachi(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    int i;
    int n = lua_objlen(L, 1);
    for (i = 1; i <= n; i++)
    {
        lua_pushvalue(L, 2);   // function
        lua_pushinteger(L, i); // 1st argument
        lua_rawgeti(L, 1, i);  // 2nd argument
        lua_call(L, 2, 1);
        if (!lua_isnil(L, -1))
            return 1;
        lua_pop(L, 1); // remove nil result
    }
    return 0;
}

static int foreach (lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_pushnil(L); // first key
    while (lua_next(L, 1))
    {
        lua_pushvalue(L, 2);  // function
        lua_pushvalue(L, -3); // key
        lua_pushvalue(L, -3); // value
        lua_call(L, 2, 1);
        if (!lua_isnil(L, -1))
            return 1;
        lua_pop(L, 2); // remove value and result
    }
    return 0;
}

static int maxn(lua_State* L)
{
    double max = 0;
    luaL_checktype(L, 1, LUA_TTABLE);

    LuaTable* t = hvalue(L->base);

    for (int i = 0; i < t->sizearray; i++)
    {
        if (!ttisnil(&t->array[i]))
            max = i + 1;
    }

    for (int i = 0; i < sizenode(t); i++)
    {
        LuaNode* n = gnode(t, i);

        if (!ttisnil(gval(n)) && ttisnumber(gkey(n)))
        {
            double v = nvalue(gkey(n));

            if (v > max)
                max = v;
        }
    }

    lua_pushnumber(L, max);
    return 1;
}

static int getn(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_pushinteger(L, lua_objlen(L, 1));
    return 1;
}

static void moveelements(lua_State* L, int srct, int dstt, int f, int e, int t)
{
    LuaTable* src = hvalue(L->base + (srct - 1));
    LuaTable* dst = hvalue(L->base + (dstt - 1));

    if (dst->readonly)
        luaG_readonlyerror(L);

    int n = e - f + 1; // number of elements to move

    if (unsigned(f) - 1 < unsigned(src->sizearray) && unsigned(t) - 1 < unsigned(dst->sizearray) &&
        unsigned(f) - 1 + unsigned(n) <= unsigned(src->sizearray) && unsigned(t) - 1 + unsigned(n) <= unsigned(dst->sizearray))
    {
        TValue* srcarray = src->array;
        TValue* dstarray = dst->array;

        if (t > e || t <= f || (dstt != srct && dst != src))
        {
            for (int i = 0; i < n; ++i)
            {
                TValue* s = &srcarray[f + i - 1];
                TValue* d = &dstarray[t + i - 1];
                setobj2t(L, d, s);
            }
        }
        else
        {
            for (int i = n - 1; i >= 0; i--)
            {
                TValue* s = &srcarray[(f + i) - 1];
                TValue* d = &dstarray[(t + i) - 1];
                setobj2t(L, d, s);
            }
        }

        luaC_barrierfast(L, dst);
    }
    else
    {
        if (t > e || t <= f || dst != src)
        {
            for (int i = 0; i < n; ++i)
            {
                lua_rawgeti(L, srct, f + i);
                lua_rawseti(L, dstt, t + i);
            }
        }
        else
        {
            for (int i = n - 1; i >= 0; i--)
            {
                lua_rawgeti(L, srct, f + i);
                lua_rawseti(L, dstt, t + i);
            }
        }
    }
}

static int tinsert(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    int n = lua_objlen(L, 1);
    int pos; // where to insert new element
    switch (lua_gettop(L))
    {
    case 2:
    {                // called with only 2 arguments
        pos = n + 1; // insert new element at the end
        break;
    }
    case 3:
    {
        pos = luaL_checkinteger(L, 2); // 2nd argument is the position

        // move up elements if necessary
        if (1 <= pos && pos <= n)
            moveelements(L, 1, 1, pos, n, pos + 1);
        break;
    }
    default:
    {
        #define STR_0 /*wrong number of arguments to 'insert'*/ scrypt("\x89\x8e\x91\x92\x99\xe0\x92\x8b\x93\x9e\x9b\x8e\xe0\x91\x9a\xe0\x9f\x8e\x99\x8b\x93\x9b\x92\x8c\x8d\xe0\x8c\x91\xe0\xd9\x97\x92\x8d\x9b\x8e\x8c\xd9").c_str()
        luaL_error(L, STR_0);
        #undef STR_0
    }
    }
    lua_rawseti(L, 1, pos); // t[pos] = v
    return 0;
}

static int tremove(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    int n = lua_objlen(L, 1);
    int pos = luaL_optinteger(L, 2, n);

    if (!(1 <= pos && pos <= n)) // position is outside bounds?
        return 0;                // nothing to remove
    lua_rawgeti(L, 1, pos);      // result = t[pos]

    moveelements(L, 1, 1, pos + 1, n, pos);

    lua_pushnil(L);
    lua_rawseti(L, 1, n); // t[n] = nil
    return 1;
}

/*
** Copy elements (1[f], ..., 1[e]) into (tt[t], tt[t+1], ...). Whenever
** possible, copy in increasing order, which is better for rehashing.
** "possible" means destination after original range, or smaller
** than origin, or copying to another table.
*/
static int tmove(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    int f = luaL_checkinteger(L, 2);
    int e = luaL_checkinteger(L, 3);
    int t = luaL_checkinteger(L, 4);
    int tt = !lua_isnoneornil(L, 5) ? 5 : 1; // destination table
    luaL_checktype(L, tt, LUA_TTABLE);

    if (e >= f)
    { // otherwise, nothing to move
        /*too many elements to move*/ scrypt_def(STR_0, "\x8c\x91\x91\xe0\x93\x9f\x92\x87\xe0\x9b\x94\x9b\x93\x9b\x92\x8c\x8d\xe0\x8c\x91\xe0\x93\x91\x8a\x9b");
        /*destination wrap around*/ scrypt_def(STR_1, "\x9c\x9b\x8d\x8c\x97\x92\x9f\x8c\x97\x91\x92\xe0\x89\x8e\x9f\x90\xe0\x9f\x8e\x91\x8b\x92\x9c");

        luaL_argcheck(L, f > 0 || e < INT_MAX + f, 3, STR_0->c_str());
        int n = e - f + 1; // number of elements to move
        luaL_argcheck(L, t <= INT_MAX - n + 1, 4, STR_1->c_str());

        LuaTable* dst = hvalue(L->base + (tt - 1));

        if (dst->readonly) // also checked in moveelements, but this blocks resizes of r/o tables
            luaG_readonlyerror(L);

        if (t > 0 && (t - 1) <= dst->sizearray && (t - 1 + n) > dst->sizearray)
        { // grow the destination table array
            luaH_resizearray(L, dst, t - 1 + n);
        }

        moveelements(L, 1, tt, f, e, t);
    }
    lua_pushvalue(L, tt); // return destination table
    return 1;
}

static void addfield(lua_State* L, luaL_Strbuf* b, int i, LuaTable* t)
{
    if (t && unsigned(i - 1) < unsigned(t->sizearray) && ttisstring(&t->array[i - 1]))
    {
        TString* ts = tsvalue(&t->array[i - 1]);
        luaL_addlstring(b, getstr(ts), ts->len);
    }
    else
    {
        int tt = lua_rawgeti(L, 1, i);
        if (tt != LUA_TSTRING && tt != LUA_TNUMBER) {
            #define STR_0 /*invalid value (%s) at index %d in table for 'concat'*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x8a\x9f\x94\x8b\x9b\xe0\xd8\xdb\x8d\xd7\xe0\x9f\x8c\xe0\x97\x92\x9c\x9b\x88\xe0\xdb\x9c\xe0\x97\x92\xe0\x8c\x9f\x9e\x94\x9b\xe0\x9a\x91\x8e\xe0\xd9\x9d\x91\x92\x9d\x9f\x8c\xd9").c_str()
            luaL_error(L, STR_0, luaL_typename(L, -1), i);
            #undef STR_0
        }
        luaL_addvalue(b);
    }
}

static int tconcat(lua_State* L)
{
    size_t lsep;
    const char* sep = luaL_optlstring(L, 2, "", &lsep);
    luaL_checktype(L, 1, LUA_TTABLE);
    int i = luaL_optinteger(L, 3, 1);
    int last = luaL_opt(L, luaL_checkinteger, 4, lua_objlen(L, 1));

    LuaTable* t = hvalue(L->base);

    luaL_Strbuf b;
    luaL_buffinit(L, &b);
    for (; i < last; i++)
    {
        addfield(L, &b, i, t);
        if (lsep != 0)
            luaL_addlstring(&b, sep, lsep);
    }
    if (i == last) // add last value (if interval was not empty)
        addfield(L, &b, i, t);
    luaL_pushresult(&b);
    return 1;
}

static int tpack(lua_State* L)
{
    int n = lua_gettop(L);    // number of elements to pack
    lua_createtable(L, n, 1); // create result table

    LuaTable* t = hvalue(L->top - 1);

    for (int i = 0; i < n; ++i)
    {
        TValue* e = &t->array[i];
        setobj2t(L, e, L->base + i);
    }

    // t.n = number of elements
    TValue* nv = luaH_setstr(L, t, luaS_newliteral(L, "n"));
    setnvalue(nv, n);

    return 1; // return table
}

static int tunpack(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    LuaTable* t = hvalue(L->base);

    int i = luaL_optinteger(L, 2, 1);
    int e = luaL_opt(L, luaL_checkinteger, 3, lua_objlen(L, 1));
    if (i > e)
        return 0;                 // empty range
    unsigned n = (unsigned)e - i; // number of elements minus 1 (avoid overflows)
    if (n >= (unsigned int)INT_MAX || !lua_checkstack(L, (int)(++n))) {
        #define STR_0 /*too many results to unpack*/ scrypt("\x8c\x91\x91\xe0\x93\x9f\x92\x87\xe0\x8e\x9b\x8d\x8b\x94\x8c\x8d\xe0\x8c\x91\xe0\x8b\x92\x90\x9f\x9d\x95").c_str()
        luaL_error(L, STR_0);
        #undef STR_0
    }

    // fast-path: direct array-to-stack copy
    if (i == 1 && int(n) <= t->sizearray)
    {
        for (i = 0; i < int(n); i++)
            setobj2s(L, L->top + i, &t->array[i]);
        L->top += n;
    }
    else
    {
        // push arg[i..e - 1] (to avoid overflows)
        for (; i < e; i++)
            lua_rawgeti(L, 1, i);
        lua_rawgeti(L, 1, e); // push last element
    }
    return (int)n;
}

typedef int (*SortPredicate)(lua_State* L, const TValue* l, const TValue* r);

static int sort_func(lua_State* L, const TValue* l, const TValue* r)
{
    LUAU_ASSERT(L->top == L->base + 2); // table, function

    setobj2s(L, L->top, &L->base[1]);
    setobj2s(L, L->top + 1, l);
    setobj2s(L, L->top + 2, r);
    L->top += 3; // safe because of LUA_MINSTACK guarantee
    luaD_call(L, L->top - 3, 1);
    L->top -= 1; // maintain stack depth

    return !l_isfalse(L->top);
}

inline void sort_swap(lua_State* L, LuaTable* t, int i, int j)
{
    TValue* arr = t->array;
    int n = t->sizearray;
    LUAU_ASSERT(unsigned(i) < unsigned(n) && unsigned(j) < unsigned(n)); // contract maintained in sort_less after predicate call

    // no barrier required because both elements are in the array before and after the swap
    TValue temp;
    setobj2s(L, &temp, &arr[i]);
    setobj2t(L, &arr[i], &arr[j]);
    setobj2t(L, &arr[j], &temp);
}

inline int sort_less(lua_State* L, LuaTable* t, int i, int j, SortPredicate pred)
{
    TValue* arr = t->array;
    int n = t->sizearray;
    LUAU_ASSERT(unsigned(i) < unsigned(n) && unsigned(j) < unsigned(n)); // contract maintained in sort_less after predicate call

    int res = pred(L, &arr[i], &arr[j]);

    // predicate call may resize the table, which is invalid
    if (t->sizearray != n) {
        #define STR_0 /*table modified during sorting*/ scrypt("\x8c\x9f\x9e\x94\x9b\xe0\x93\x91\x9c\x97\x9a\x97\x9b\x9c\xe0\x9c\x8b\x8e\x97\x92\x99\xe0\x8d\x91\x8e\x8c\x97\x92\x99").c_str()
        luaL_error(L, STR_0);
        #undef STR_0
    }

    return res;
}

static void sort_siftheap(lua_State* L, LuaTable* t, int l, int u, SortPredicate pred, int root)
{
    LUAU_ASSERT(l <= u);
    int count = u - l + 1;

    // process all elements with two children
    while (root * 2 + 2 < count)
    {
        int left = root * 2 + 1, right = root * 2 + 2;
        int next = root;
        next = sort_less(L, t, l + next, l + left, pred) ? left : next;
        next = sort_less(L, t, l + next, l + right, pred) ? right : next;

        if (next == root)
            break;

        sort_swap(L, t, l + root, l + next);
        root = next;
    }

    // process last element if it has just one child
    int lastleft = root * 2 + 1;
    if (lastleft == count - 1 && sort_less(L, t, l + root, l + lastleft, pred))
        sort_swap(L, t, l + root, l + lastleft);
}

static void sort_heap(lua_State* L, LuaTable* t, int l, int u, SortPredicate pred)
{
    LUAU_ASSERT(l <= u);
    int count = u - l + 1;

    for (int i = count / 2 - 1; i >= 0; --i)
        sort_siftheap(L, t, l, u, pred, i);

    for (int i = count - 1; i > 0; --i)
    {
        sort_swap(L, t, l, l + i);
        sort_siftheap(L, t, l, l + i - 1, pred, 0);
    }
}

static void sort_rec(lua_State* L, LuaTable* t, int l, int u, int limit, SortPredicate pred)
{
    // sort range [l..u] (inclusive, 0-based)
    while (l < u)
    {
        // if the limit has been reached, quick sort is going over the permitted nlogn complexity, so we fall back to heap sort
        if (limit == 0)
            return sort_heap(L, t, l, u, pred);

        // sort elements a[l], a[(l+u)/2] and a[u]
        // note: this simultaneously acts as a small sort and a median selector
        if (sort_less(L, t, u, l, pred)) // a[u] < a[l]?
            sort_swap(L, t, u, l);       // swap a[l] - a[u]
        if (u - l == 1)
            break;                       // only 2 elements
        int m = l + ((u - l) >> 1);      // midpoint
        if (sort_less(L, t, m, l, pred)) // a[m]<a[l]?
            sort_swap(L, t, m, l);
        else if (sort_less(L, t, u, m, pred)) // a[u]<a[m]?
            sort_swap(L, t, m, u);
        if (u - l == 2)
            break; // only 3 elements

        // here l, m, u are ordered; m will become the new pivot
        int p = u - 1;
        sort_swap(L, t, m, u - 1); // pivot is now (and always) at u-1

        // a[l] <= P == a[u-1] <= a[u], only need to sort from l+1 to u-2
        int i = l;
        int j = u - 1;
        for (;;)
        { // invariant: a[l..i] <= P <= a[j..u]
            // repeat ++i until a[i] >= P
            #define STR_0 /*invalid order function for sorting*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x91\x8e\x9c\x9b\x8e\xe0\x9a\x8b\x92\x9d\x8c\x97\x91\x92\xe0\x9a\x91\x8e\xe0\x8d\x91\x8e\x8c\x97\x92\x99").c_str()

            while (sort_less(L, t, ++i, p, pred))
            {
                if (i >= u)
                    luaL_error(L, STR_0);
            }
            // repeat --j until a[j] <= P
            while (sort_less(L, t, p, --j, pred))
            {
                if (j <= l)
                    luaL_error(L, STR_0);
            }
            if (j < i)
                break;
            sort_swap(L, t, i, j);
            #undef STR_0
        }

        // swap pivot a[p] with a[i], which is the new midpoint
        sort_swap(L, t, p, i);

        // adjust limit to allow 1.5 log2N recursive steps
        limit = (limit >> 1) + (limit >> 2);

        // a[l..i-1] <= a[i] == P <= a[i+1..u]
        // sort smaller half recursively; the larger half is sorted in the next loop iteration
        if (i - l < u - i)
        {
            sort_rec(L, t, l, i - 1, limit, pred);
            l = i + 1;
        }
        else
        {
            sort_rec(L, t, i + 1, u, limit, pred);
            u = i - 1;
        }
    }
}

static int tsort(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    LuaTable* t = hvalue(L->base);
    int n = luaH_getn(t);
    if (t->readonly)
        luaG_readonlyerror(L);

    SortPredicate pred = luaV_lessthan;
    if (!lua_isnoneornil(L, 2)) // is there a 2nd argument?
    {
        luaL_checktype(L, 2, LUA_TFUNCTION);
        pred = sort_func;
    }
    lua_settop(L, 2); // make sure there are two arguments

    if (n > 0)
        sort_rec(L, t, 0, n - 1, n, pred);
    return 0;
}

static int tcreate(lua_State* L)
{
    int size = luaL_checkinteger(L, 1);
    if (size < 0) {
        #define STR_0 /*size out of range*/ scrypt("\x8d\x97\x86\x9b\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x8e\x9f\x92\x99\x9b").c_str()
        luaL_argerror(L, 1, STR_0);
        #undef STR_0
    }

    if (!lua_isnoneornil(L, 2))
    {
        lua_createtable(L, size, 0);
        LuaTable* t = hvalue(L->top - 1);

        StkId v = L->base + 1;

        for (int i = 0; i < size; ++i)
        {
            TValue* e = &t->array[i];
            setobj2t(L, e, v);
        }
    }
    else
    {
        lua_createtable(L, size, 0);
    }

    return 1;
}

static int tfind(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checkany(L, 2);
    int init = luaL_optinteger(L, 3, 1);
    if (init < 1) {
        #define STR_0 /*index out of range*/ scrypt("\x97\x92\x9c\x9b\x88\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x8e\x9f\x92\x99\x9b").c_str()
        luaL_argerror(L, 3, STR_0);
        #undef STR_0
    }

    LuaTable* t = hvalue(L->base);
    StkId v = L->base + 1;

    for (int i = init;; ++i)
    {
        const TValue* e = luaH_getnum(t, i);
        if (ttisnil(e))
            break;

        if (equalobj(L, v, e))
        {
            lua_pushinteger(L, i);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

static int tclear(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);

    LuaTable* tt = hvalue(L->base);
    if (tt->readonly)
        luaG_readonlyerror(L);

    luaH_clear(tt);
    return 0;
}

static int tfreeze(lua_State* L)
{
    /*table is already frozen*/ scrypt_def(STR_0, "\x8c\x9f\x9e\x94\x9b\xe0\x97\x8d\xe0\x9f\x94\x8e\x9b\x9f\x9c\x87\xe0\x9a\x8e\x91\x86\x9b\x92");
    /*__metatable*/ scrypt_def(STR_1, "\xa1\xa1\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b");
    /*table has a protected metatable*/ scrypt_def(STR_2, "\x8c\x9f\x9e\x94\x9b\xe0\x98\x9f\x8d\xe0\x9f\xe0\x90\x8e\x91\x8c\x9b\x9d\x8c\x9b\x9c\xe0\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b");

    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_argcheck(L, !lua_getreadonly(L, 1), 1, STR_0->c_str());
    luaL_argcheck(L, !luaL_getmetafield(L, 1, STR_1->c_str()), 1, STR_2->c_str());

    lua_setreadonly(L, 1, true);

    lua_pushvalue(L, 1);
    return 1;
}

static int tisfrozen(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_pushboolean(L, lua_getreadonly(L, 1));
    return 1;
}

static int tclone(lua_State* L)
{
    /*__metatable*/ scrypt_def(STR_0, "\xa1\xa1\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b");
    /*table has a protected metatable*/ scrypt_def(STR_1, "\x8c\x9f\x9e\x94\x9b\xe0\x98\x9f\x8d\xe0\x9f\xe0\x90\x8e\x91\x8c\x9b\x9d\x8c\x9b\x9c\xe0\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b");

    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_argcheck(L, !luaL_getmetafield(L, 1, STR_0->c_str()), 1, STR_1->c_str());

    LuaTable* tt = luaH_clone(L, hvalue(L->base));

    TValue v;
    sethvalue(L, &v, tt);
    luaA_pushobject(L, &v);

    return 1;
}

int luaopen_table(lua_State* L)
{
    std::string STR_0 = /*concat*/ scrypt("\x9d\x91\x92\x9d\x9f\x8c");
    std::string STR_1 = /*foreach*/ scrypt("\x9a\x91\x8e\x9b\x9f\x9d\x98");
    std::string STR_2 = /*foreachi*/ scrypt("\x9a\x91\x8e\x9b\x9f\x9d\x98\x97");
    std::string STR_3 = /*getn*/ scrypt("\x99\x9b\x8c\x92");
    std::string STR_4 = /*maxn*/ scrypt("\x93\x9f\x88\x92");
    std::string STR_5 = /*insert*/ scrypt("\x97\x92\x8d\x9b\x8e\x8c");
    std::string STR_6 = /*remove*/ scrypt("\x8e\x9b\x93\x91\x8a\x9b");
    std::string STR_7 = /*sort*/ scrypt("\x8d\x91\x8e\x8c");
    std::string STR_8 = /*pack*/ scrypt("\x90\x9f\x9d\x95");
    std::string STR_9 = /*unpack*/ scrypt("\x8b\x92\x90\x9f\x9d\x95");
    std::string STR_10 = /*move*/ scrypt("\x93\x91\x8a\x9b");
    std::string STR_11 = /*create*/ scrypt("\x9d\x8e\x9b\x9f\x8c\x9b");
    std::string STR_12 = /*find*/ scrypt("\x9a\x97\x92\x9c");
    std::string STR_13 = /*clear*/ scrypt("\x9d\x94\x9b\x9f\x8e");
    std::string STR_14 = /*freeze*/ scrypt("\x9a\x8e\x9b\x9b\x86\x9b");
    std::string STR_15 = /*isfrozen*/ scrypt("\x97\x8d\x9a\x8e\x91\x86\x9b\x92");
    std::string STR_16 = /*clone*/ scrypt("\x9d\x94\x91\x92\x9b");
    std::string STR_17 = /*unpack*/ scrypt("\x8b\x92\x90\x9f\x9d\x95");
    std::string STR_18 = /*table*/ scrypt("\x8c\x9f\x9e\x94\x9b");

    static const luaL_Reg tab_funcs[] = {
        {STR_0.c_str(), tconcat},
        {STR_1.c_str(), foreach},
        {STR_2.c_str(), foreachi},
        {STR_3.c_str(), getn}, 
        {STR_4.c_str(), maxn},
        {STR_5.c_str(), tinsert},
        {STR_6.c_str(), tremove},
        {STR_7.c_str(), tsort},
        {STR_8.c_str(), tpack},
        {STR_9.c_str(), tunpack},
        {STR_10.c_str(), tmove},
        {STR_11.c_str(), tcreate},
        {STR_12.c_str(), tfind},
        {STR_13.c_str(), tclear},
        {STR_14.c_str(), tfreeze},
        {STR_15.c_str(), tisfrozen},
        {STR_16.c_str(), tclone},
        {NULL, NULL},
    };

    luaL_register(L, STR_18.c_str(), tab_funcs);

    // Lua 5.1 compat
    lua_pushcfunction(L, tunpack, STR_17.c_str());
    lua_setglobal(L, STR_17.c_str());

    return 1;
}
