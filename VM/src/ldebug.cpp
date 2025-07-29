// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "ldebug.h"

#include "lapi.h"
#include "lfunc.h"
#include "lmem.h"
#include "lgc.h"
#include "ldo.h"
#include "lbytecode.h"

#include <string.h>
#include <stdio.h>

static const char* getfuncname(Closure* f);

static int currentpc(lua_State* L, CallInfo* ci)
{
    return pcRel(ci->savedpc, ci_func(ci)->l.p);
}

static int currentline(lua_State* L, CallInfo* ci)
{
    return luaG_getline(ci_func(ci)->l.p, currentpc(L, ci));
}

static Proto* getluaproto(CallInfo* ci)
{
    return (isLua(ci) ? cast_to(Proto*, ci_func(ci)->l.p) : NULL);
}

int lua_getargument(lua_State* L, int level, int n)
{
    if (unsigned(level) >= unsigned(L->ci - L->base_ci))
        return 0;

    CallInfo* ci = L->ci - level;
    // changing tables in native functions externally may invalidate safety contracts wrt table state (metatable/size/readonly)
    if (ci->flags & LUA_CALLINFO_NATIVE)
        return 0;

    Proto* fp = getluaproto(ci);
    int res = 0;

    if (fp && n > 0)
    {
        if (n <= fp->numparams)
        {
            luaC_threadbarrier(L);
            luaA_pushobject(L, ci->base + (n - 1));
            res = 1;
        }
        else if (fp->is_vararg && n < ci->base - ci->func)
        {
            luaC_threadbarrier(L);
            luaA_pushobject(L, ci->func + n);
            res = 1;
        }
    }

    return res;
}

const char* lua_getlocal(lua_State* L, int level, int n)
{
    if (unsigned(level) >= unsigned(L->ci - L->base_ci))
        return NULL;

    CallInfo* ci = L->ci - level;
    // changing tables in native functions externally may invalidate safety contracts wrt table state (metatable/size/readonly)
    if (ci->flags & LUA_CALLINFO_NATIVE)
        return NULL;

    Proto* fp = getluaproto(ci);
    const LocVar* var = fp ? luaF_getlocal(fp, n, currentpc(L, ci)) : NULL;
    if (var)
    {
        luaC_threadbarrier(L);
        luaA_pushobject(L, ci->base + var->reg);
    }
    const char* name = var ? getstr(var->varname) : NULL;
    return name;
}

const char* lua_setlocal(lua_State* L, int level, int n)
{
    if (unsigned(level) >= unsigned(L->ci - L->base_ci))
        return NULL;

    CallInfo* ci = L->ci - level;
    // changing registers in native functions externally may invalidate safety contracts wrt register type tags
    if (ci->flags & LUA_CALLINFO_NATIVE)
        return NULL;

    Proto* fp = getluaproto(ci);
    const LocVar* var = fp ? luaF_getlocal(fp, n, currentpc(L, ci)) : NULL;
    if (var)
        setobj2s(L, ci->base + var->reg, L->top - 1);
    L->top--; // pop value
    const char* name = var ? getstr(var->varname) : NULL;
    return name;
}

static Closure* auxgetinfo(lua_State* L, const char* what, lua_Debug* ar, Closure* f, CallInfo* ci)
{
    Closure* cl = NULL;
    for (; *what; what++)
    {
        switch (*what)
        {
        case 's':
        {
            if (f->isC)
            {
                /*=[C]*/ scrypt_def(STR_0, "\xc3\xa5\xbd\xa3");
                /*C*/ scrypt_def(STR_1, "\xbd");
                /*[C]*/ scrypt_def(STR_2, "\xa5\xbd\xa3");
                ar->source = STR_0->c_str();
                ar->what = STR_1->c_str();
                ar->linedefined = -1;
                ar->short_src = STR_2->c_str();
            }
            else
            {
                /*Lua*/ scrypt_def(STR_0, "\xb4\x8b\x9f");
                TString* source = f->l.p->source;
                ar->source = getstr(source);
                ar->what = STR_0->c_str();
                ar->linedefined = f->l.p->linedefined;
                ar->short_src = luaO_chunkid(ar->ssbuf, sizeof(ar->ssbuf), getstr(source), source->len);
            }
            break;
        }
        case 'l':
        {
            if (ci)
            {
                ar->currentline = isLua(ci) ? currentline(L, ci) : -1;
            }
            else
            {
                ar->currentline = f->isC ? -1 : f->l.p->linedefined;
            }

            break;
        }
        case 'u':
        {
            ar->nupvals = f->nupvalues;
            break;
        }
        case 'a':
        {
            if (f->isC)
            {
                ar->isvararg = 1;
                ar->nparams = 0;
            }
            else
            {
                ar->isvararg = f->l.p->is_vararg;
                ar->nparams = f->l.p->numparams;
            }
            break;
        }
        case 'n':
        {
            ar->name = ci ? getfuncname(ci_func(ci)) : getfuncname(f);
            break;
        }
        case 'f':
        {
            cl = f;
            break;
        }
        default:;
        }
    }
    return cl;
}

int lua_stackdepth(lua_State* L)
{
    return int(L->ci - L->base_ci);
}

int lua_getinfo(lua_State* L, int level, const char* what, lua_Debug* ar)
{
    Closure* f = NULL;
    CallInfo* ci = NULL;
    if (level < 0)
    {
        // element has to be within stack
        if (-level > L->top - L->base)
            return 0;

        StkId func = L->top + level;

        // and it has to be a function
        if (!ttisfunction(func))
            return 0;

        f = clvalue(func);
    }
    else if (unsigned(level) < unsigned(L->ci - L->base_ci))
    {
        ci = L->ci - level;
        LUAU_ASSERT(ttisfunction(ci->func));
        f = clvalue(ci->func);
    }
    if (f)
    {
        // auxgetinfo fills ar and optionally requests to put closure on stack
        if (Closure* fcl = auxgetinfo(L, what, ar, f, ci))
        {
            luaC_threadbarrier(L);
            setclvalue(L, L->top, fcl);
            incr_top(L);
        }
    }
    return f ? 1 : 0;
}

static const char* getfuncname(Closure* cl)
{
    if (cl->isC)
    {
        if (cl->c.debugname)
        {
            return cl->c.debugname;
        }
    }
    else
    {
        Proto* p = cl->l.p;

        if (p->debugname)
        {
            return getstr(p->debugname);
        }
    }
    return nullptr;
}

l_noret luaG_typeerrorL(lua_State* L, const TValue* o, const char* op)
{
    /*attempt to %s a %s value*/ scrypt_def(STR_0, "\x9f\x8c\x8c\x9b\x93\x90\x8c\xe0\x8c\x91\xe0\xdb\x8d\xe0\x9f\xe0\xdb\x8d\xe0\x8a\x9f\x94\x8b\x9b");

    const char* t = luaT_objtypename(L, o);

    luaG_runerror(L, STR_0->c_str(), op, t);
}

l_noret luaG_forerrorL(lua_State* L, const TValue* o, const char* what)
{
    /*invalid 'for' %s (number expected, got %s)*/ scrypt_def(STR_0, "\x97\x92\x8a\x9f\x94\x97\x9c\xe0\xd9\x9a\x91\x8e\xd9\xe0\xdb\x8d\xe0\xd8\x92\x8b\x93\x9e\x9b\x8e\xe0\x9b\x88\x90\x9b\x9d\x8c\x9b\x9c\xd4\xe0\x99\x91\x8c\xe0\xdb\x8d\xd7");

    const char* t = luaT_objtypename(L, o);

    luaG_runerror(L, STR_0->c_str(), what, t);
}

l_noret luaG_concaterror(lua_State* L, StkId p1, StkId p2)
{
    /*attempt to concatenate %s with %s*/ scrypt_def(STR_0, "\x9f\x8c\x8c\x9b\x93\x90\x8c\xe0\x8c\x91\xe0\x9d\x91\x92\x9d\x9f\x8c\x9b\x92\x9f\x8c\x9b\xe0\xdb\x8d\xe0\x89\x97\x8c\x98\xe0\xdb\x8d");

    const char* t1 = luaT_objtypename(L, p1);
    const char* t2 = luaT_objtypename(L, p2);

    luaG_runerror(L, STR_0->c_str(), t1, t2);
}

l_noret luaG_aritherror(lua_State* L, const TValue* p1, const TValue* p2, TMS op)
{
    const char* t1 = luaT_objtypename(L, p1);
    const char* t2 = luaT_objtypename(L, p2);
    const char* opname = luaT_eventname(op) + 2; // skip __ from metamethod name

    if (t1 == t2) {
        #define STR_0 /*attempt to perform arithmetic (%s) on %s*/ scrypt("\x9f\x8c\x8c\x9b\x93\x90\x8c\xe0\x8c\x91\xe0\x90\x9b\x8e\x9a\x91\x8e\x93\xe0\x9f\x8e\x97\x8c\x98\x93\x9b\x8c\x97\x9d\xe0\xd8\xdb\x8d\xd7\xe0\x91\x92\xe0\xdb\x8d").c_str()
        luaG_runerror(L, STR_0, opname, t1);
        #undef STR_0
    }
    else {
        #define STR_1 /*attempt to perform arithmetic (%s) on %s and %s*/ scrypt("\x9f\x8c\x8c\x9b\x93\x90\x8c\xe0\x8c\x91\xe0\x90\x9b\x8e\x9a\x91\x8e\x93\xe0\x9f\x8e\x97\x8c\x98\x93\x9b\x8c\x97\x9d\xe0\xd8\xdb\x8d\xd7\xe0\x91\x92\xe0\xdb\x8d\xe0\x9f\x92\x9c\xe0\xdb\x8d").c_str()
        luaG_runerror(L, STR_1, opname, t1, t2);
        #undef STR_1
    }
}


l_noret luaG_ordererror(lua_State* L, const TValue* p1, const TValue* p2, TMS op)
{
    /*attempt to compare %s %s %s*/ scrypt_def(STR_0, "\x9f\x8c\x8c\x9b\x93\x90\x8c\xe0\x8c\x91\xe0\x9d\x91\x93\x90\x9f\x8e\x9b\xe0\xdb\x8d\xe0\xdb\x8d\xe0\xdb\x8d");
    /*<*/ scrypt_def(STR_1, "\xc4");
    /*<=*/ scrypt_def(STR_2, "\xc4\xc3");
    /*==*/ scrypt_def(STR_3, "\xc3\xc3"); 

    const char* t1 = luaT_objtypename(L, p1);
    const char* t2 = luaT_objtypename(L, p2);
    const char* opname = (op == TM_LT) ? STR_1->c_str() : (op == TM_LE) ? STR_2->c_str() : STR_3->c_str();

    luaG_runerror(L, STR_0->c_str(), t1, opname, t2);
}

l_noret luaG_indexerror(lua_State* L, const TValue* p1, const TValue* p2)
{
    const char* t1 = luaT_objtypename(L, p1);
    const char* t2 = luaT_objtypename(L, p2);
    const TString* key = ttisstring(p2) ? tsvalue(p2) : 0;

    if (key && key->len <= 64) { // limit length to make sure we don't generate very long error messages for very long keys
        #define STR_0 /*attempt to index %s with '%s'*/ scrypt("\x9f\x8c\x8c\x9b\x93\x90\x8c\xe0\x8c\x91\xe0\x97\x92\x9c\x9b\x88\xe0\xdb\x8d\xe0\x89\x97\x8c\x98\xe0\xd9\xdb\x8d\xd9").c_str()
        luaG_runerror(L, STR_0, t1, getstr(key));
        #undef STR_0
    }
    else {
        #define STR_1 /*attempt to index %s with %s*/ scrypt("\x9f\x8c\x8c\x9b\x93\x90\x8c\xe0\x8c\x91\xe0\x97\x92\x9c\x9b\x88\xe0\xdb\x8d\xe0\x89\x97\x8c\x98\xe0\xdb\x8d").c_str()
        luaG_runerror(L, STR_1, t1, t2);
        #undef STR_1
    }
}

l_noret luaG_methoderror(lua_State* L, const TValue* p1, const TString* p2)
{
    /*attempt to call missing method '%s' of %s*/ scrypt_def(STR_0, "\x9f\x8c\x8c\x9b\x93\x90\x8c\xe0\x8c\x91\xe0\x9d\x9f\x94\x94\xe0\x93\x97\x8d\x8d\x97\x92\x99\xe0\x93\x9b\x8c\x98\x91\x9c\xe0\xd9\xdb\x8d\xd9\xe0\x91\x9a\xe0\xdb\x8d");

    const char* t1 = luaT_objtypename(L, p1);

    luaG_runerror(L, STR_0->c_str(), getstr(p2), t1);
}

l_noret luaG_readonlyerror(lua_State* L)
{
    /*attempt to modify a readonly table*/ scrypt_def(STR_0, "\x9f\x8c\x8c\x9b\x93\x90\x8c\xe0\x8c\x91\xe0\x93\x91\x9c\x97\x9a\x87\xe0\x9f\xe0\x8e\x9b\x9f\x9c\x91\x92\x94\x87\xe0\x8c\x9f\x9e\x94\x9b");
    luaG_runerror(L, STR_0->c_str());
}


static void pusherror(lua_State* L, const char* msg)
{
    CallInfo* ci = L->ci;
    if (isLua(ci))
    {
        /*%s:%d: %s*/ scrypt_def(STR_0, "\xdb\x8d\xc6\xdb\x9c\xc6\xe0\xdb\x8d");
        TString* source = getluaproto(ci)->source;
        char chunkbuf[LUA_IDSIZE]; // add file:line information
        const char* chunkid = luaO_chunkid(chunkbuf, sizeof(chunkbuf), getstr(source), source->len);
        int line = currentline(L, ci);
        luaO_pushfstring(L, STR_0->c_str(), chunkid, line, msg);
    }
    else
    {
        lua_pushstring(L, msg);
    }
}

l_noret luaG_runerrorL(lua_State* L, const char* fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    char result[LUA_BUFFERSIZE];
    vsnprintf(result, sizeof(result), fmt, argp);
    va_end(argp);

    lua_rawcheckstack(L, 1);

    pusherror(L, result);
    luaD_throw(L, LUA_ERRRUN);
}

void luaG_pusherror(lua_State* L, const char* error)
{
    lua_rawcheckstack(L, 1);

    pusherror(L, error);
}

void luaG_breakpoint(lua_State* L, Proto* p, int line, bool enable)
{
    void (*ondisable)(lua_State*, Proto*) = L->global->ecb.disable;

    // since native code doesn't support breakpoints, we would need to update all call frames with LUAU_CALLINFO_NATIVE that refer to p
    if (p->lineinfo && (ondisable || !p->execdata))
    {
        for (int i = 0; i < p->sizecode; ++i)
        {
            // note: we keep prologue as is, instead opting to break at the first meaningful instruction
            if (LUAU_INSN_OP(p->code[i]) == LOP_PREPVARARGS)
                continue;

            if (luaG_getline(p, i) != line)
                continue;

            // lazy copy of the original opcode array; done when the first breakpoint is set
            if (!p->debuginsn)
            {
                p->debuginsn = luaM_newarray(L, p->sizecode, uint8_t, p->memcat);
                for (int j = 0; j < p->sizecode; ++j)
                    p->debuginsn[j] = LUAU_INSN_OP(p->code[j]);
            }

            uint8_t op = enable ? LOP_BREAK : LUAU_INSN_OP(p->debuginsn[i]);

            // patch just the opcode byte, leave arguments alone
            p->code[i] &= ~0xff;
            p->code[i] |= op;
            LUAU_ASSERT(LUAU_INSN_OP(p->code[i]) == op);

            // currently we don't restore native code when breakpoint is disabled.
            // this will be addressed in the future.
            if (enable && p->execdata && ondisable)
                ondisable(L, p);

            // note: this is important!
            // we only patch the *first* instruction in each proto that's attributed to a given line
            // this can be changed, but if requires making patching a bit more nuanced so that we don't patch AUX words
            break;
        }
    }

    for (int i = 0; i < p->sizep; ++i)
    {
        luaG_breakpoint(L, p->p[i], line, enable);
    }
}

bool luaG_onbreak(lua_State* L)
{
    if (L->ci == L->base_ci)
        return false;

    if (!isLua(L->ci))
        return false;

    return LUAU_INSN_OP(*L->ci->savedpc) == LOP_BREAK;
}

int luaG_getline(Proto* p, int pc)
{
    LUAU_ASSERT(pc >= 0 && pc < p->sizecode);

    if (!p->lineinfo)
        return 0;

    return p->abslineinfo[pc >> p->linegaplog2] + p->lineinfo[pc];
}

int luaG_isnative(lua_State* L, int level)
{
    if (unsigned(level) >= unsigned(L->ci - L->base_ci))
        return 0;

    CallInfo* ci = L->ci - level;
    return (ci->flags & LUA_CALLINFO_NATIVE) != 0 ? 1 : 0;
}

int luaG_hasnative(lua_State* L, int level)
{
    if (unsigned(level) >= unsigned(L->ci - L->base_ci))
        return 0;

    CallInfo* ci = L->ci - level;

    Proto* proto = getluaproto(ci);
    if (proto == nullptr)
        return 0;

    return (proto->execdata != nullptr);
}

void lua_singlestep(lua_State* L, int enabled)
{
    L->singlestep = bool(enabled);
}

static int getmaxline(Proto* p)
{
    int result = -1;

    for (int i = 0; i < p->sizecode; ++i)
    {
        int line = luaG_getline(p, i);
        result = result < line ? line : result;
    }

    for (int i = 0; i < p->sizep; ++i)
    {
        int psize = getmaxline(p->p[i]);
        result = result < psize ? psize : result;
    }

    return result;
}

// Find the line number with instructions. If the provided line doesn't have any instruction, it should return the next valid line number.
static int getnextline(Proto* p, int line)
{
    int closest = -1;

    if (p->lineinfo)
    {
        for (int i = 0; i < p->sizecode; ++i)
        {
            // note: we keep prologue as is, instead opting to break at the first meaningful instruction
            if (LUAU_INSN_OP(p->code[i]) == LOP_PREPVARARGS)
                continue;

            int candidate = luaG_getline(p, i);

            if (candidate == line)
                return line;

            if (candidate > line && (closest == -1 || candidate < closest))
                closest = candidate;
        }
    }

    for (int i = 0; i < p->sizep; ++i)
    {
        int candidate = getnextline(p->p[i], line);

        if (candidate == line)
            return line;

        if (candidate > line && (closest == -1 || candidate < closest))
            closest = candidate;
    }

    return closest;
}

int lua_breakpoint(lua_State* L, int funcindex, int line, int enabled)
{
    const TValue* func = luaA_toobject(L, funcindex);
    api_check(L, ttisfunction(func) && !clvalue(func)->isC);

    Proto* p = clvalue(func)->l.p;

    // set the breakpoint to the next closest line with valid instructions
    int target = getnextline(p, line);

    if (target != -1)
        luaG_breakpoint(L, p, target, bool(enabled));

    return target;
}

static void getcoverage(Proto* p, int depth, int* buffer, size_t size, void* context, lua_Coverage callback)
{
    memset(buffer, -1, size * sizeof(int));

    for (int i = 0; i < p->sizecode; ++i)
    {
        Instruction insn = p->code[i];
        if (LUAU_INSN_OP(insn) != LOP_COVERAGE)
            continue;

        int line = luaG_getline(p, i);
        int hits = LUAU_INSN_E(insn);

        LUAU_ASSERT(size_t(line) < size);
        buffer[line] = buffer[line] < hits ? hits : buffer[line];
    }

    const char* debugname = p->debugname ? getstr(p->debugname) : NULL;
    int linedefined = p->linedefined;

    callback(context, debugname, linedefined, depth, buffer, size);

    for (int i = 0; i < p->sizep; ++i)
        getcoverage(p->p[i], depth + 1, buffer, size, context, callback);
}

void lua_getcoverage(lua_State* L, int funcindex, void* context, lua_Coverage callback)
{
    const TValue* func = luaA_toobject(L, funcindex);
    api_check(L, ttisfunction(func) && !clvalue(func)->isC);

    Proto* p = clvalue(func)->l.p;

    size_t size = getmaxline(p) + 1;
    if (size == 0)
        return;

    int* buffer = luaM_newarray(L, size, int, 0);

    getcoverage(p, 0, buffer, size, context, callback);

    luaM_freearray(L, buffer, size, int, 0);
}

static size_t append(char* buf, size_t bufsize, size_t offset, const char* data)
{
    size_t size = strlen(data);
    size_t copy = offset + size >= bufsize ? bufsize - offset - 1 : size;
    memcpy(buf + offset, data, copy);
    return offset + copy;
}

const char* lua_debugtrace(lua_State* L)
{
    static char buf[4096];

    const int limit1 = 10;
    const int limit2 = 10;

    int depth = int(L->ci - L->base_ci);
    size_t offset = 0;

    lua_Debug ar;
    /*sln*/ scrypt_def(STR_3, "\x8d\x94\x92");
    for (int level = 0; lua_getinfo(L, level, STR_3->c_str(), &ar); ++level)
    {
        if (ar.source)
            offset = append(buf, sizeof(buf), offset, ar.short_src);

        if (ar.currentline > 0)
        {
            /*:%d*/ scrypt_def(STR_0, "\xc6\xdb\x9c");

            char line[32];
            snprintf(line, sizeof(line), STR_0->c_str(), ar.currentline);

            offset = append(buf, sizeof(buf), offset, line);
        }

        if (ar.name)
        {
            /* function */ scrypt_def(STR_1, "\xe0\x9a\x8b\x92\x9d\x8c\x97\x91\x92\xe0");
            offset = append(buf, sizeof(buf), offset, STR_1->c_str());
            offset = append(buf, sizeof(buf), offset, ar.name);
        }

        offset = append(buf, sizeof(buf), offset, "\n");

        if (depth > limit1 + limit2 && level == limit1 - 1)
        {
            /*... (+%d frames)\n*/ scrypt_def(STR_2, "\xd2\xd2\xd2\xe0\xd8\xd5\xdb\x9c\xe0\x9a\x8e\x9f\x93\x9b\x8d\xd7\xf6");

            char skip[32];
            snprintf(skip, sizeof(skip), STR_2->c_str(), int(depth - limit1 - limit2));

            offset = append(buf, sizeof(buf), offset, skip);

            level = depth - limit2 - 1;
        }
    }

    LUAU_ASSERT(offset < sizeof(buf));
    buf[offset] = '\0';

    return buf;
}
