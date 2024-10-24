// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lgc.h"

#include "lfunc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ludata.h"
#include "lbuffer.h"

#include <string.h>
#include <stdio.h>

static void validateobjref(global_State* g, GCObject* f, GCObject* t)
{
    LUAU_ASSERT(!isdead(g, t));

    if (keepinvariant(g))
    {
        // basic incremental invariant: black can't point to white
        LUAU_ASSERT(!(isblack(f) && iswhite(t)));
    }
}

static void validateref(global_State* g, GCObject* f, TValue* v)
{
    if (iscollectable(v))
    {
        LUAU_ASSERT(ttype(v) == gcvalue(v)->gch.tt);
        validateobjref(g, f, gcvalue(v));
    }
}

static void validatetable(global_State* g, Table* h)
{
    int sizenode = 1 << h->lsizenode;

    LUAU_ASSERT(h->lastfree <= sizenode);

    if (h->metatable)
        validateobjref(g, obj2gco(h), obj2gco(h->metatable));

    for (int i = 0; i < h->sizearray; ++i)
        validateref(g, obj2gco(h), &h->array[i]);

    for (int i = 0; i < sizenode; ++i)
    {
        LuaNode* n = &h->node[i];

        LUAU_ASSERT(ttype(gkey(n)) != LUA_TDEADKEY || ttisnil(gval(n)));
        LUAU_ASSERT(i + gnext(n) >= 0 && i + gnext(n) < sizenode);

        if (!ttisnil(gval(n)))
        {
            TValue k = {};
            k.tt = gkey(n)->tt;
            k.value = gkey(n)->value;

            validateref(g, obj2gco(h), &k);
            validateref(g, obj2gco(h), gval(n));
        }
    }
}

static void validateclosure(global_State* g, Closure* cl)
{
    validateobjref(g, obj2gco(cl), obj2gco(cl->env));

    if (cl->isC)
    {
        for (int i = 0; i < cl->nupvalues; ++i)
            validateref(g, obj2gco(cl), &cl->c.upvals[i]);
    }
    else
    {
        LUAU_ASSERT(cl->nupvalues == cl->l.p->nups);

        validateobjref(g, obj2gco(cl), obj2gco(cl->l.p));

        for (int i = 0; i < cl->nupvalues; ++i)
            validateref(g, obj2gco(cl), &cl->l.uprefs[i]);
    }
}

static void validatestack(global_State* g, lua_State* l)
{
    validateobjref(g, obj2gco(l), obj2gco(l->gt));

    for (CallInfo* ci = l->base_ci; ci <= l->ci; ++ci)
    {
        LUAU_ASSERT(l->stack <= ci->base);
        LUAU_ASSERT(ci->func <= ci->base && ci->base <= ci->top);
        LUAU_ASSERT(ci->top <= l->stack_last);
    }

    // note: stack refs can violate gc invariant so we only check for liveness
    for (StkId o = l->stack; o < l->top; ++o)
        checkliveness(g, o);

    if (l->namecall)
        validateobjref(g, obj2gco(l), obj2gco(l->namecall));

    for (UpVal* uv = l->openupval; uv; uv = uv->u.open.threadnext)
    {
        LUAU_ASSERT(uv->tt == LUA_TUPVAL);
        LUAU_ASSERT(upisopen(uv));
        LUAU_ASSERT(uv->u.open.next->u.open.prev == uv && uv->u.open.prev->u.open.next == uv);
        LUAU_ASSERT(!isblack(obj2gco(uv))); // open upvalues are never black
    }
}

static void validateproto(global_State* g, Proto* f)
{
    if (f->source)
        validateobjref(g, obj2gco(f), obj2gco(f->source));

    if (f->debugname)
        validateobjref(g, obj2gco(f), obj2gco(f->debugname));

    for (int i = 0; i < f->sizek; ++i)
        validateref(g, obj2gco(f), &f->k[i]);

    for (int i = 0; i < f->sizeupvalues; ++i)
        if (f->upvalues[i])
            validateobjref(g, obj2gco(f), obj2gco(f->upvalues[i]));

    for (int i = 0; i < f->sizep; ++i)
        if (f->p[i])
            validateobjref(g, obj2gco(f), obj2gco(f->p[i]));

    for (int i = 0; i < f->sizelocvars; i++)
        if (f->locvars[i].varname)
            validateobjref(g, obj2gco(f), obj2gco(f->locvars[i].varname));
}

static void validateobj(global_State* g, GCObject* o)
{
    // dead objects can only occur during sweep
    if (isdead(g, o))
    {
        LUAU_ASSERT(g->gcstate == GCSsweep);
        return;
    }

    switch (o->gch.tt)
    {
    case LUA_TSTRING:
        break;

    case LUA_TTABLE:
        validatetable(g, gco2h(o));
        break;

    case LUA_TFUNCTION:
        validateclosure(g, gco2cl(o));
        break;

    case LUA_TUSERDATA:
        if (gco2u(o)->metatable)
            validateobjref(g, o, obj2gco(gco2u(o)->metatable));
        break;

    case LUA_TTHREAD:
        validatestack(g, gco2th(o));
        break;

    case LUA_TBUFFER:
        break;

    case LUA_TPROTO:
        validateproto(g, gco2p(o));
        break;

    case LUA_TUPVAL:
        validateref(g, o, gco2uv(o)->v);
        break;

    default:
        LUAU_ASSERT(!"unexpected object type");
    }
}

static void validategraylist(global_State* g, GCObject* o)
{
    if (!keepinvariant(g))
        return;

    while (o)
    {
        LUAU_ASSERT(isgray(o));

        switch (o->gch.tt)
        {
        case LUA_TTABLE:
            o = gco2h(o)->gclist;
            break;
        case LUA_TFUNCTION:
            o = gco2cl(o)->gclist;
            break;
        case LUA_TTHREAD:
            o = gco2th(o)->gclist;
            break;
        case LUA_TPROTO:
            o = gco2p(o)->gclist;
            break;
        default:
            LUAU_ASSERT(!"unknown object in gray list");
            return;
        }
    }
}

static bool validategco(void* context, lua_Page* page, GCObject* gco)
{
    lua_State* L = (lua_State*)context;
    global_State* g = L->global;

    validateobj(g, gco);
    return false;
}

void luaC_validate(lua_State* L)
{
    global_State* g = L->global;

    LUAU_ASSERT(!isdead(g, obj2gco(g->mainthread)));
    checkliveness(g, &g->registry);

    for (int i = 0; i < LUA_T_COUNT; ++i)
        if (g->mt[i])
            LUAU_ASSERT(!isdead(g, obj2gco(g->mt[i])));

    validategraylist(g, g->weak);
    validategraylist(g, g->gray);
    validategraylist(g, g->grayagain);

    validategco(L, NULL, obj2gco(g->mainthread));

    luaM_visitgco(L, L, validategco);

    for (UpVal* uv = g->uvhead.u.open.next; uv != &g->uvhead; uv = uv->u.open.next)
    {
        LUAU_ASSERT(uv->tt == LUA_TUPVAL);
        LUAU_ASSERT(upisopen(uv));
        LUAU_ASSERT(uv->u.open.next->u.open.prev == uv && uv->u.open.prev->u.open.next == uv);
        LUAU_ASSERT(!isblack(obj2gco(uv))); // open upvalues are never black
    }
}

inline bool safejson(char ch)
{
    return unsigned(ch) < 128 && ch >= 32 && ch != '\\' && ch != '\"';
}

static void dumpref(FILE* f, GCObject* o)
{
    #define STR_0 /*"%p"*/ scrypt("\xde\xdb\x90\xde").c_str()
    fprintf(f, STR_0, o);
    #undef STR_0
}

static void dumprefs(FILE* f, TValue* data, size_t size)
{
    bool first = true;

    for (size_t i = 0; i < size; ++i)
    {
        if (iscollectable(&data[i]))
        {
            if (!first)
                fputc(',', f);
            first = false;

            dumpref(f, gcvalue(&data[i]));
        }
    }
}

static void dumpstringdata(FILE* f, const char* data, size_t len)
{
    for (size_t i = 0; i < len; ++i)
        fputc(safejson(data[i]) ? data[i] : '?', f);
}

static void dumpstring(FILE* f, TString* ts)
{
    #define STR_0 /*{"type":"string","cat":%d,"size":%d,"data":"*/ scrypt("\x85\xde\x8c\x87\x90\x9b\xde\xc6\xde\x8d\x8c\x8e\x97\x92\x99\xde\xd4\xde\x9d\x9f\x8c\xde\xc6\xdb\x9c\xd4\xde\x8d\x97\x86\x9b\xde\xc6\xdb\x9c\xd4\xde\x9c\x9f\x8c\x9f\xde\xc6\xde").c_str()
    #define STR_1 /*"}*/ scrypt("\xde\x83").c_str()
    fprintf(f, STR_0, ts->memcat, int(sizestring(ts->len)));
    dumpstringdata(f, ts->data, ts->len);
    fprintf(f, STR_1);
    #undef STR_0
    #undef STR_1
}

static void dumptable(FILE* f, Table* h)
{   
    size_t size = sizeof(Table) + (h->node == &luaH_dummynode ? 0 : sizenode(h) * sizeof(LuaNode)) + h->sizearray * sizeof(TValue);

    #define STR_0 /*{"type":"table","cat":%d,"size":%d*/ scrypt("\x85\xde\x8c\x87\x90\x9b\xde\xc6\xde\x8c\x9f\x9e\x94\x9b\xde\xd4\xde\x9d\x9f\x8c\xde\xc6\xdb\x9c\xd4\xde\x8d\x97\x86\x9b\xde\xc6\xdb\x9c").c_str()
    fprintf(f, STR_0, h->memcat, int(size));
    #undef STR_0

    if (h->node != &luaH_dummynode)
    {
        #define STR_1 /*,"pairs":[*/ scrypt("\xd4\xde\x90\x9f\x97\x8e\x8d\xde\xc6\xa5").c_str()
        fprintf(f, STR_1);
        #undef STR_1

        bool first = true;

        for (int i = 0; i < sizenode(h); ++i)
        {
            const LuaNode& n = h->node[i];

            if (!ttisnil(&n.val) && (iscollectable(&n.key) || iscollectable(&n.val)))
            {
                #define STR_2 /*null*/ scrypt("\x92\x8b\x94\x94").c_str()

                if (!first)
                    fputc(',', f);
                first = false;

                if (iscollectable(&n.key))
                    dumpref(f, gcvalue(&n.key));
                else
                    fprintf(f, STR_2);

                fputc(',', f);

                if (iscollectable(&n.val))
                    dumpref(f, gcvalue(&n.val));
                else
                    fprintf(f, STR_2);
                
                #undef STR_2
            }
        }

        fprintf(f, "]");
    }
    if (h->sizearray)
    {
        #define STR_3 /*,"array":[*/ scrypt("\xd4\xde\x9f\x8e\x8e\x9f\x87\xde\xc6\xa5").c_str()
        fprintf(f, STR_3);
        #undef STR_3
        dumprefs(f, h->array, h->sizearray);
        fprintf(f, "]");
    }
    if (h->metatable)
    {
        #define STR_4 /*,"metatable":*/ scrypt("\xd4\xde\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b\xde\xc6").c_str()
        fprintf(f, STR_4);
        #undef STR_4
        dumpref(f, obj2gco(h->metatable));
    }
    fprintf(f, "}");
}

static void dumpclosure(FILE* f, Closure* cl)
{
    #define STR_0 /*{"type":"function","cat":%d,"size":%d*/ scrypt("\x85\xde\x8c\x87\x90\x9b\xde\xc6\xde\x9a\x8b\x92\x9d\x8c\x97\x91\x92\xde\xd4\xde\x9d\x9f\x8c\xde\xc6\xdb\x9c\xd4\xde\x8d\x97\x86\x9b\xde\xc6\xdb\x9c").c_str()
    fprintf(
        f, STR_0, cl->memcat, cl->isC ? int(sizeCclosure(cl->nupvalues)) : int(sizeLclosure(cl->nupvalues))
    );
    #undef STR_0

    #define STR_1 /*,"env":*/ scrypt("\xd4\xde\x9b\x92\x8a\xde\xc6").c_str()
    fprintf(f, STR_1);
    #undef STR_1
    dumpref(f, obj2gco(cl->env));

    if (cl->isC)
    {
        #define STR_2 /*,"name":"%s"*/ scrypt("\xd4\xde\x92\x9f\x93\x9b\xde\xc6\xde\xdb\x8d\xde").c_str()

        if (cl->c.debugname)
            fprintf(f, STR_2, cl->c.debugname + 0);

        #undef STR_2

        if (cl->nupvalues)
        {
            #define STR_3 /*,"upvalues":[*/ scrypt("\xd4\xde\x8b\x90\x8a\x9f\x94\x8b\x9b\x8d\xde\xc6\xa5").c_str()
            fprintf(f, STR_3);
            #undef STR_3
            dumprefs(f, cl->c.upvals, cl->nupvalues);
            fprintf(f, "]");
        }
    }
    else
    {
        if (cl->l.p->debugname) {
            #define STR_4 /*,"name":"%s"*/ scrypt("\xd4\xde\x92\x9f\x93\x9b\xde\xc6\xde\xdb\x8d\xde").c_str()
            fprintf(f, STR_4, getstr(cl->l.p->debugname));
            #undef STR_4
        }

        #define STR_5 /*,"proto":*/ scrypt("\xd4\xde\x90\x8e\x91\x8c\x91\xde\xc6").c_str()
        fprintf(f, STR_5);
        #undef STR_5
        dumpref(f, obj2gco(cl->l.p));
        if (cl->nupvalues)
        {
            #define STR_6 /*,"upvalues":[*/ scrypt("\xd4\xde\x8b\x90\x8a\x9f\x94\x8b\x9b\x8d\xde\xc6\xa5").c_str()
            fprintf(f, STR_6);
            #undef STR_6
            dumprefs(f, cl->l.uprefs, cl->nupvalues);
            fprintf(f, "]");
        }
    }
    fprintf(f, "}");
}


static void dumpudata(FILE* f, Udata* u)
{
    #define STR_0 /*{"type":"userdata","cat":%d,"size":%d,"tag":%d*/ scrypt("\x85\xde\x8c\x87\x90\x9b\xde\xc6\xde\x8b\x8d\x9b\x8e\x9c\x9f\x8c\x9f\xde\xd4\xde\x9d\x9f\x8c\xde\xc6\xdb\x9c\xd4\xde\x8d\x97\x86\x9b\xde\xc6\xdb\x9c\xd4\xde\x8c\x9f\x99\xde\xc6\xdb\x9c").c_str()
    fprintf(f, STR_0, u->memcat, int(sizeudata(u->len)), u->tag);
    #undef STR_0

    if (u->metatable)
    {
        #define STR_1 /*,"metatable":*/ scrypt("\xd4\xde\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b\xde\xc6").c_str()
        fprintf(f, STR_1);
        #undef STR_1
        dumpref(f, obj2gco(u->metatable));
    }
    fprintf(f, "}");
}

static void dumpthread(FILE* f, lua_State* th)
{
    size_t size = sizeof(lua_State) + sizeof(TValue) * th->stacksize + sizeof(CallInfo) * th->size_ci;

    #define STR_0 /*{"type":"thread","cat":%d,"size":%d*/ scrypt("\x85\xde\x8c\x87\x90\x9b\xde\xc6\xde\x8c\x98\x8e\x9b\x9f\x9c\xde\xd4\xde\x9d\x9f\x8c\xde\xc6\xdb\x9c\xd4\xde\x8d\x97\x86\x9b\xde\xc6\xdb\x9c").c_str()
    fprintf(f, STR_0, th->memcat, int(size));
    #undef STR_0

    #define STR_1 /*,"env":*/ scrypt("\xd4\xde\x9b\x92\x8a\xde\xc6").c_str()
    fprintf(f, STR_1);
    #undef STR_1
    dumpref(f, obj2gco(th->gt));

    Closure* tcl = 0;
    for (CallInfo* ci = th->base_ci; ci <= th->ci; ++ci)
    {
        if (ttisfunction(ci->func))
        {
            tcl = clvalue(ci->func);
            break;
        }
    }

    if (tcl && !tcl->isC && tcl->l.p->source)
    {
        Proto* p = tcl->l.p;

        #define STR_2 /*,"source":"*/ scrypt("\xd4\xde\x8d\x91\x8b\x8e\x9d\x9b\xde\xc6\xde").c_str()
        #define STR_3 /*","line":%d*/ scrypt("\xde\xd4\xde\x94\x97\x92\x9b\xde\xc6\xdb\x9c").c_str()
        fprintf(f, STR_2);
        dumpstringdata(f, p->source->data, p->source->len);
        fprintf(f, STR_3, p->linedefined);
        #undef STR_2
        #undef STR_3
    }

    if (th->top > th->stack)
    {
        #define STR_4 /*,"stack":[*/ scrypt("\xd4\xde\x8d\x8c\x9f\x9d\x95\xde\xc6\xa5").c_str()
        fprintf(f, STR_4);
        #undef STR_4
        dumprefs(f, th->stack, th->top - th->stack);
        fprintf(f, "]");

        CallInfo* ci = th->base_ci;
        bool first = true;

        #define STR_5 /*,"stacknames":[*/ scrypt("\xd4\xde\x8d\x8c\x9f\x9d\x95\x92\x9f\x93\x9b\x8d\xde\xc6\xa5").c_str()
        fprintf(f, STR_5);
        #undef STR_5
        for (StkId v = th->stack; v < th->top; ++v)
        {
            if (!iscollectable(v))
                continue;

            while (ci < th->ci && v >= (ci + 1)->func)
                ci++;

            if (!first)
                fputc(',', f);
            first = false;
            #define STR_10 /*null*/ scrypt("\x92\x8b\x94\x94").c_str()

            if (v == ci->func)
            {
                Closure* cl = ci_func(ci);

                if (cl->isC)
                {
                    #define STR_6 /*"frame:%s"*/ scrypt("\xde\x9a\x8e\x9f\x93\x9b\xc6\xdb\x8d\xde").c_str()
                    fprintf(f, STR_6, cl->c.debugname ? cl->c.debugname : "[C]");
                    #undef STR_6
                }
                else
                {
                    #define STR_7 /*"frame:*/ scrypt("\xde\x9a\x8e\x9f\x93\x9b\xc6").c_str()
                    Proto* p = cl->l.p;
                    fprintf(f, STR_7);
                    #undef STR_7
                    if (p->source)
                        dumpstringdata(f, p->source->data, p->source->len);
                    #define STR_8 /*:%d:%s"*/ scrypt("\xc6\xdb\x9c\xc6\xdb\x8d\xde").c_str()
                    fprintf(f, STR_8, p->linedefined, p->debugname ? getstr(p->debugname) : "");
                    #undef STR_8
                }
            }
            else if (isLua(ci))
            {
                Proto* p = ci_func(ci)->l.p;
                int pc = pcRel(ci->savedpc, p);
                const LocVar* var = luaF_findlocal(p, int(v - ci->base), pc);

                if (var && var->varname) {
                    #define STR_9 /*"%s"*/ scrypt("\xde\xdb\x8d\xde").c_str()
                    fprintf(f, STR_9, getstr(var->varname));
                    #undef STR_9
                }
                else
                    fprintf(f, STR_10);
            }
            else
                fprintf(f, STR_10);
        }
        #undef STR_10
        fprintf(f, "]");
    }
    fprintf(f, "}");
}

static void dumpbuffer(FILE* f, Buffer* b)
{
    #define STR_0 /*{"type":"buffer","cat":%d,"size":%d}*/ scrypt("\x85\xde\x8c\x87\x90\x9b\xde\xc6\xde\x9e\x8b\x9a\x9a\x9b\x8e\xde\xd4\xde\x9d\x9f\x8c\xde\xc6\xdb\x9c\xd4\xde\x8d\x97\x86\x9b\xde\xc6\xdb\x9c\x83").c_str()
    fprintf(f, STR_0, b->memcat, int(sizebuffer(b->len)));
    #undef STR_0
}

static void dumpproto(FILE* f, Proto* p)
{
    size_t size = sizeof(Proto) + sizeof(Instruction) * p->sizecode + sizeof(Proto*) * p->sizep + sizeof(TValue) * p->sizek + p->sizelineinfo +
                  sizeof(LocVar) * p->sizelocvars + sizeof(TString*) * p->sizeupvalues;

    #define STR_0 /*{"type":"proto","cat":%d,"size":%d*/ scrypt("\x85\xde\x8c\x87\x90\x9b\xde\xc6\xde\x90\x8e\x91\x8c\x91\xde\xd4\xde\x9d\x9f\x8c\xde\xc6\xdb\x9c\xd4\xde\x8d\x97\x86\x9b\xde\xc6\xdb\x9c").c_str()
    fprintf(f, STR_0, p->memcat, int(size));
    #undef STR_0

    if (p->source)
    {
        #define STR_1 /*,"source":"*/ scrypt("\xd4\xde\x8d\x91\x8b\x8e\x9d\x9b\xde\xc6\xde").c_str()
        #define STR_2 /*","line":%d*/ scrypt("\xde\xd4\xde\x94\x97\x92\x9b\xde\xc6\xdb\x9c").c_str()
        fprintf(f, STR_1);
        dumpstringdata(f, p->source->data, p->source->len);
        fprintf(f, STR_2, p->abslineinfo ? p->abslineinfo[0] : 0);
        #undef STR_1
        #undef STR_2
    }

    if (p->sizek)
    {
        #define STR_3 /*,"constants":[*/ scrypt("\xd4\xde\x9d\x91\x92\x8d\x8c\x9f\x92\x8c\x8d\xde\xc6\xa5").c_str()
        fprintf(f, STR_3);
        #undef STR_3
        dumprefs(f, p->k, p->sizek);
        fprintf(f, "]");
    }

    if (p->sizep)
    {
        #define STR_4 /*,"protos":[*/ scrypt("\xd4\xde\x90\x8e\x91\x8c\x91\x8d\xde\xc6\xa5").c_str()
        fprintf(f, STR_4);
        #undef STR_4
        for (int i = 0; i < p->sizep; ++i)
        {
            if (i != 0)
                fputc(',', f);
            dumpref(f, obj2gco(p->p[i]));
        }
        fprintf(f, "]");
    }

    fprintf(f, "}");
}

static void dumpupval(FILE* f, UpVal* uv)
{
    #define STR_0 /*{"type":"upvalue","cat":%d,"size":%d,"open":%s*/ scrypt("\x85\xde\x8c\x87\x90\x9b\xde\xc6\xde\x8b\x90\x8a\x9f\x94\x8b\x9b\xde\xd4\xde\x9d\x9f\x8c\xde\xc6\xdb\x9c\xd4\xde\x8d\x97\x86\x9b\xde\xc6\xdb\x9c\xd4\xde\x91\x90\x9b\x92\xde\xc6\xdb\x8d").c_str()
    #define STR_1 /*true*/ scrypt("\x8c\x8e\x8b\x9b").c_str()
    #define STR_2 /*false*/ scrypt("\x9a\x9f\x94\x8d\x9b").c_str()
    fprintf(f, STR_0, uv->memcat, int(sizeof(UpVal)), upisopen(uv) ? STR_1 : STR_2);
    #undef STR_0
    #undef STR_1
    #undef STR_2

    if (iscollectable(uv->v))
    {
        #define STR_3 /*,"object":*/ scrypt("\xd4\xde\x91\x9e\x96\x9b\x9d\x8c\xde\xc6").c_str()
        fprintf(f, STR_3);
        #undef STR_3
        dumpref(f, gcvalue(uv->v));
    }

    fprintf(f, "}");
}

static void dumpobj(FILE* f, GCObject* o)
{
    switch (o->gch.tt)
    {
    case LUA_TSTRING:
        return dumpstring(f, gco2ts(o));

    case LUA_TTABLE:
        return dumptable(f, gco2h(o));

    case LUA_TFUNCTION:
        return dumpclosure(f, gco2cl(o));

    case LUA_TUSERDATA:
        return dumpudata(f, gco2u(o));

    case LUA_TTHREAD:
        return dumpthread(f, gco2th(o));

    case LUA_TBUFFER:
        return dumpbuffer(f, gco2buf(o));

    case LUA_TPROTO:
        return dumpproto(f, gco2p(o));

    case LUA_TUPVAL:
        return dumpupval(f, gco2uv(o));

    default:
        LUAU_ASSERT(0);
    }
}

static bool dumpgco(void* context, lua_Page* page, GCObject* gco)
{
    FILE* f = (FILE*)context;

    dumpref(f, gco);
    fputc(':', f);
    dumpobj(f, gco);
    fputc(',', f);
    fputc('\n', f);

    return false;
}

void luaC_dump(lua_State* L, void* file, const char* (*categoryName)(lua_State* L, uint8_t memcat))
{
#define STR_0 /*{"objects":{\n*/ scrypt("\x85\xde\x91\x9e\x96\x9b\x9d\x8c\x8d\xde\xc6\x85\xf6").c_str()
#define STR_1 /*"0":{"type":"userdata","cat":0,"size":0}\n*/ scrypt("\xde\xd0\xde\xc6\x85\xde\x8c\x87\x90\x9b\xde\xc6\xde\x8b\x8d\x9b\x8e\x9c\x9f\x8c\x9f\xde\xd4\xde\x9d\x9f\x8c\xde\xc6\xd0\xd4\xde\x8d\x97\x86\x9b\xde\xc6\xd0\x83\xf6").c_str()
#define STR_2 /*},"roots":{\n*/ scrypt("\x83\xd4\xde\x8e\x91\x91\x8c\x8d\xde\xc6\x85\xf6").c_str()
#define STR_3 /*"mainthread":*/ scrypt("\xde\x93\x9f\x97\x92\x8c\x98\x8e\x9b\x9f\x9c\xde\xc6").c_str()
#define STR_4 /*,"registry":*/ scrypt("\xd4\xde\x8e\x9b\x99\x97\x8d\x8c\x8e\x87\xde\xc6").c_str()
#define STR_5 /*},"stats":{\n*/ scrypt("\x83\xd4\xde\x8d\x8c\x9f\x8c\x8d\xde\xc6\x85\xf6").c_str()
#define STR_6 /*"size":%d,\n*/ scrypt("\xde\x8d\x97\x86\x9b\xde\xc6\xdb\x9c\xd4\xf6").c_str()
#define STR_7 /*"categories":{\n*/ scrypt("\xde\x9d\x9f\x8c\x9b\x99\x91\x8e\x97\x9b\x8d\xde\xc6\x85\xf6").c_str()
#define STR_8 /*"%d":{"name":"%s", "size":%d},\n*/ scrypt("\xde\xdb\x9c\xde\xc6\x85\xde\x92\x9f\x93\x9b\xde\xc6\xde\xdb\x8d\xde\xd4\xe0\xde\x8d\x97\x86\x9b\xde\xc6\xdb\x9c\x83\xd4\xf6").c_str()
#define STR_9 /*"%d":{"size":%d},\n*/ scrypt("\xde\xdb\x9c\xde\xc6\x85\xde\x8d\x97\x86\x9b\xde\xc6\xdb\x9c\x83\xd4\xf6").c_str()
#define STR_10 /*"none":{}\n*/ scrypt("\xde\x92\x91\x92\x9b\xde\xc6\x85\x83\xf6").c_str()

    global_State* g = L->global;
    FILE* f = static_cast<FILE*>(file);

    fprintf(f, STR_0);

    dumpgco(f, NULL, obj2gco(g->mainthread));

    luaM_visitgco(L, f, dumpgco);

    fprintf(f, STR_1); // to avoid issues with trailing ,
    fprintf(f, STR_2);
    fprintf(f, STR_3);
    dumpref(f, obj2gco(g->mainthread));
    fprintf(f, STR_4);
    dumpref(f, gcvalue(&g->registry));

    fprintf(f, STR_5);

    fprintf(f, STR_6, int(g->totalbytes));

    fprintf(f, STR_7);
    for (int i = 0; i < LUA_MEMORY_CATEGORIES; i++)
    {
        if (size_t bytes = g->memcatbytes[i])
        {
            if (categoryName)
                fprintf(f, STR_8, i, categoryName(L, i), int(bytes));
            else
                fprintf(f, STR_9, i, int(bytes));
        }
    }
    fprintf(f, STR_10); // to avoid issues with trailing ,
    fprintf(f, "}\n");
    fprintf(f, "}}\n");

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

struct EnumContext
{
    lua_State* L;
    void* context;
    void (*node)(void* context, void* ptr, uint8_t tt, uint8_t memcat, size_t size, const char* name);
    void (*edge)(void* context, void* from, void* to, const char* name);
};

static void* enumtopointer(GCObject* gco)
{
    // To match lua_topointer, userdata pointer is represented as a pointer to internal data
    return gco->gch.tt == LUA_TUSERDATA ? (void*)gco2u(gco)->data : (void*)gco;
}

static void enumnode(EnumContext* ctx, GCObject* gco, size_t size, const char* objname)
{
    ctx->node(ctx->context, enumtopointer(gco), gco->gch.tt, gco->gch.memcat, size, objname);
}

static void enumedge(EnumContext* ctx, GCObject* from, GCObject* to, const char* edgename)
{
    ctx->edge(ctx->context, enumtopointer(from), enumtopointer(to), edgename);
}

static void enumedges(EnumContext* ctx, GCObject* from, TValue* data, size_t size, const char* edgename)
{
    for (size_t i = 0; i < size; ++i)
    {
        if (iscollectable(&data[i]))
            enumedge(ctx, from, gcvalue(&data[i]), edgename);
    }
}

static void enumstring(EnumContext* ctx, TString* ts)
{
    enumnode(ctx, obj2gco(ts), ts->len, NULL);
}

static void enumtable(EnumContext* ctx, Table* h)
{
    /*registry*/ scrypt_def(STR_0, "\x8e\x9b\x99\x97\x8d\x8c\x8e\x87");

    size_t size = sizeof(Table) + (h->node == &luaH_dummynode ? 0 : sizenode(h) * sizeof(LuaNode)) + h->sizearray * sizeof(TValue);

    // Provide a name for a special registry table
    enumnode(ctx, obj2gco(h), size, h == hvalue(registry(ctx->L)) ? STR_0->c_str() : NULL);

    if (h->node != &luaH_dummynode)
    {
        bool weakkey = false;
        bool weakvalue = false;

        if (const TValue* mode = gfasttm(ctx->L->global, h->metatable, TM_MODE))
        {
            if (ttisstring(mode))
            {
                weakkey = strchr(svalue(mode), 'k') != NULL;
                weakvalue = strchr(svalue(mode), 'v') != NULL;
            }
        }

        for (int i = 0; i < sizenode(h); ++i)
        {
            const LuaNode& n = h->node[i];

            if (!ttisnil(&n.val) && (iscollectable(&n.key) || iscollectable(&n.val)))
            {
                if (!weakkey && iscollectable(&n.key)) {
                    /*[key]*/ scrypt_def(STR_1, "\xa5\x95\x9b\x87\xa3");
                    enumedge(ctx, obj2gco(h), gcvalue(&n.key), STR_1->c_str());
                }

                if (!weakvalue && iscollectable(&n.val))
                {
                    if (ttisstring(&n.key))
                    {
                        enumedge(ctx, obj2gco(h), gcvalue(&n.val), svalue(&n.key));
                    }
                    else if (ttisnumber(&n.key))
                    {
                        /*%.14g*/ scrypt_def(STR_2, "\xdb\xd2\xcf\xcc\x99");
                        char buf[32];
                        snprintf(buf, sizeof(buf), STR_2->c_str(), nvalue(&n.key));
                        enumedge(ctx, obj2gco(h), gcvalue(&n.val), buf);
                    }
                    else
                    {
                        /*[%s]*/ scrypt_def(STR_3, "\xa5\xdb\x8d\xa3");
                        char buf[32];
                        snprintf(buf, sizeof(buf), STR_3->c_str(), getstr(ctx->L->global->ttname[n.key.tt]));
                        enumedge(ctx, obj2gco(h), gcvalue(&n.val), buf);
                    }
                }
            }
        }
    }

    if (h->sizearray) {
        /*array*/ scrypt_def(STR_4, "\x9f\x8e\x8e\x9f\x87");
        enumedges(ctx, obj2gco(h), h->array, h->sizearray, STR_4->c_str());
    }

    if (h->metatable) {
        /*metatable*/ scrypt_def(STR_5, "\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b");
        enumedge(ctx, obj2gco(h), obj2gco(h->metatable), STR_5->c_str());
    }
}

static void enumclosure(EnumContext* ctx, Closure* cl)
{
    if (cl->isC)
    {
        enumnode(ctx, obj2gco(cl), sizeCclosure(cl->nupvalues), cl->c.debugname);
    }
    else
    {
        Proto* p = cl->l.p;

        char buf[LUA_IDSIZE];

        if (p->source) {
            /*%s:%d %s*/ scrypt_def(STR_0, "\xdb\x8d\xc6\xdb\x9c\xe0\xdb\x8d");
            snprintf(buf, sizeof(buf), STR_0->c_str(), p->debugname ? getstr(p->debugname) : "", p->linedefined, getstr(p->source));
        }
        else {
            /*%s:%d*/ scrypt_def(STR_1, "\xdb\x8d\xc6\xdb\x9c");
            snprintf(buf, sizeof(buf), STR_1->c_str(), p->debugname ? getstr(p->debugname) : "", p->linedefined);
        }

        enumnode(ctx, obj2gco(cl), sizeLclosure(cl->nupvalues), buf);
    }

    /*env*/ scrypt_def(STR_2, "\x9b\x92\x8a");
    enumedge(ctx, obj2gco(cl), obj2gco(cl->env), STR_2->c_str());

    if (cl->isC)
    {
        if (cl->nupvalues) {
            /*upvalue*/ scrypt_def(STR_3, "\x8b\x90\x8a\x9f\x94\x8b\x9b");
            enumedges(ctx, obj2gco(cl), cl->c.upvals, cl->nupvalues, STR_3->c_str());
        }
    }
    else
    {
        /*proto*/ scrypt_def(STR_4, "\x90\x8e\x91\x8c\x91");
        enumedge(ctx, obj2gco(cl), obj2gco(cl->l.p), STR_4->c_str());

        if (cl->nupvalues) {
            /*upvalue*/ scrypt_def(STR_5, "\x8b\x90\x8a\x9f\x94\x8b\x9b");
            enumedges(ctx, obj2gco(cl), cl->l.uprefs, cl->nupvalues, STR_5->c_str());
        }
    }
}

static void enumudata(EnumContext* ctx, Udata* u)
{
    const char* name = NULL;

    if (Table* h = u->metatable)
    {
        if (h->node != &luaH_dummynode)
        {
            for (int i = 0; i < sizenode(h); ++i)
            {
                const LuaNode& n = h->node[i];
                /*__type*/ scrypt_def(STR_0, "\xa1\xa1\x8c\x87\x90\x9b");

                if (ttisstring(&n.key) && ttisstring(&n.val) && strcmp(svalue(&n.key), STR_0->c_str()) == 0)
                {
                    name = svalue(&n.val);
                    break;
                }
            }
        }
    }

    enumnode(ctx, obj2gco(u), sizeudata(u->len), name);

    if (u->metatable) {
        /*metatable*/ scrypt_def(STR_1, "\x93\x9b\x8c\x9f\x8c\x9f\x9e\x94\x9b");
        enumedge(ctx, obj2gco(u), obj2gco(u->metatable), STR_1->c_str());
    }
}

static void enumthread(EnumContext* ctx, lua_State* th)
{
    size_t size = sizeof(lua_State) + sizeof(TValue) * th->stacksize + sizeof(CallInfo) * th->size_ci;

    Closure* tcl = NULL;
    for (CallInfo* ci = th->base_ci; ci <= th->ci; ++ci)
    {
        if (ttisfunction(ci->func))
        {
            tcl = clvalue(ci->func);
            break;
        }
    }

    if (tcl && !tcl->isC && tcl->l.p->source)
    {
        Proto* p = tcl->l.p;

        char buf[LUA_IDSIZE];

        if (p->source) {
            /*%s:%d %s*/ scrypt_def(STR_0, "\xdb\x8d\xc6\xdb\x9c\xe0\xdb\x8d");
            snprintf(buf, sizeof(buf), STR_0->c_str(), p->debugname ? getstr(p->debugname) : "", p->linedefined, getstr(p->source));
        }
        else {
            /*%s:%d*/ scrypt_def(STR_1, "\xdb\x8d\xc6\xdb\x9c");
            snprintf(buf, sizeof(buf), STR_1->c_str(), p->debugname ? getstr(p->debugname) : "", p->linedefined);
        }

        enumnode(ctx, obj2gco(th), size, buf);
    }
    else
    {
        enumnode(ctx, obj2gco(th), size, NULL);
    }

    /*globals*/ scrypt_def(STR_2, "\x99\x94\x91\x9e\x9f\x94\x8d");
    enumedge(ctx, obj2gco(th), obj2gco(th->gt), STR_2->c_str());

    if (th->top > th->stack) {
        /*stack*/ scrypt_def(STR_3, "\x8d\x8c\x9f\x9d\x95");
        enumedges(ctx, obj2gco(th), th->stack, th->top - th->stack, STR_3->c_str());
    }
}

static void enumbuffer(EnumContext* ctx, Buffer* b)
{
    enumnode(ctx, obj2gco(b), sizebuffer(b->len), NULL);
}

static void enumproto(EnumContext* ctx, Proto* p)
{
    size_t size = sizeof(Proto) + sizeof(Instruction) * p->sizecode + sizeof(Proto*) * p->sizep + sizeof(TValue) * p->sizek + p->sizelineinfo +
                  sizeof(LocVar) * p->sizelocvars + sizeof(TString*) * p->sizeupvalues;

    if (p->execdata && ctx->L->global->ecb.getmemorysize)
    {
        size_t nativesize = ctx->L->global->ecb.getmemorysize(ctx->L, p);
        /*[native]*/ scrypt_def(STR_0, "\xa5\x92\x9f\x8c\x97\x8a\x9b\xa3");

        ctx->node(ctx->context, p->execdata, uint8_t(LUA_TNONE), p->memcat, nativesize, NULL);
        ctx->edge(ctx->context, enumtopointer(obj2gco(p)), p->execdata, STR_0->c_str());
    }

    enumnode(ctx, obj2gco(p), size, p->source ? getstr(p->source) : NULL);

    if (p->sizek) {
        /*constants*/ scrypt_def(STR_1, "\x9d\x91\x92\x8d\x8c\x9f\x92\x8c\x8d");
        enumedges(ctx, obj2gco(p), p->k, p->sizek, STR_1->c_str());
    }

    for (int i = 0; i < p->sizep; ++i) {
        /*protos*/ scrypt_def(STR_2, "\x90\x8e\x91\x8c\x91\x8d");
        enumedge(ctx, obj2gco(p), obj2gco(p->p[i]), STR_2->c_str());
    }
}

static void enumupval(EnumContext* ctx, UpVal* uv)
{
    enumnode(ctx, obj2gco(uv), sizeof(UpVal), NULL);

    if (iscollectable(uv->v)) {
        /*value*/ scrypt_def(STR_0, "\x8a\x9f\x94\x8b\x9b");
        enumedge(ctx, obj2gco(uv), gcvalue(uv->v), STR_0->c_str());
    }
}

static void enumobj(EnumContext* ctx, GCObject* o)
{
    switch (o->gch.tt)
    {
    case LUA_TSTRING:
        return enumstring(ctx, gco2ts(o));

    case LUA_TTABLE:
        return enumtable(ctx, gco2h(o));

    case LUA_TFUNCTION:
        return enumclosure(ctx, gco2cl(o));

    case LUA_TUSERDATA:
        return enumudata(ctx, gco2u(o));

    case LUA_TTHREAD:
        return enumthread(ctx, gco2th(o));

    case LUA_TBUFFER:
        return enumbuffer(ctx, gco2buf(o));

    case LUA_TPROTO:
        return enumproto(ctx, gco2p(o));

    case LUA_TUPVAL:
        return enumupval(ctx, gco2uv(o));

    default:
        LUAU_ASSERT(!"Unknown object tag");
    }
}

static bool enumgco(void* context, lua_Page* page, GCObject* gco)
{
    enumobj((EnumContext*)context, gco);
    return false;
}

void luaC_enumheap(
    lua_State* L,
    void* context,
    void (*node)(void* context, void* ptr, uint8_t tt, uint8_t memcat, size_t size, const char* name),
    void (*edge)(void* context, void* from, void* to, const char* name)
)
{
    global_State* g = L->global;

    EnumContext ctx;
    ctx.L = L;
    ctx.context = context;
    ctx.node = node;
    ctx.edge = edge;

    enumgco(&ctx, NULL, obj2gco(g->mainthread));

    luaM_visitgco(L, &ctx, enumgco);
}
