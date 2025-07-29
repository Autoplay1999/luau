// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lualib.h"

#include "lstring.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>

// macro to `unsign' a character
#define uchar(c) ((unsigned char)(c))

static int str_len(lua_State* L)
{
    size_t l;
    luaL_checklstring(L, 1, &l);
    lua_pushinteger(L, (int)l);
    return 1;
}

static int posrelat(int pos, size_t len)
{
    // relative string position: negative means back from end
    if (pos < 0)
        pos += (int)len + 1;
    return (pos >= 0) ? pos : 0;
}

static int str_sub(lua_State* L)
{
    size_t l;
    const char* s = luaL_checklstring(L, 1, &l);
    int start = posrelat(luaL_checkinteger(L, 2), l);
    int end = posrelat(luaL_optinteger(L, 3, -1), l);
    if (start < 1)
        start = 1;
    if (end > (int)l)
        end = (int)l;
    if (start <= end)
        lua_pushlstring(L, s + start - 1, end - start + 1);
    else
        lua_pushliteral(L, "");
    return 1;
}

static int str_reverse(lua_State* L)
{
    size_t l;
    const char* s = luaL_checklstring(L, 1, &l);
    luaL_Strbuf b;
    char* ptr = luaL_buffinitsize(L, &b, l);
    while (l--)
        *ptr++ = s[l];
    luaL_pushresultsize(&b, ptr - b.p);
    return 1;
}

static int str_lower(lua_State* L)
{
    size_t l;
    const char* s = luaL_checklstring(L, 1, &l);
    luaL_Strbuf b;
    char* ptr = luaL_buffinitsize(L, &b, l);
    for (size_t i = 0; i < l; i++)
        *ptr++ = tolower(uchar(s[i]));
    luaL_pushresultsize(&b, l);
    return 1;
}

static int str_upper(lua_State* L)
{
    size_t l;
    const char* s = luaL_checklstring(L, 1, &l);
    luaL_Strbuf b;
    char* ptr = luaL_buffinitsize(L, &b, l);
    for (size_t i = 0; i < l; i++)
        *ptr++ = toupper(uchar(s[i]));
    luaL_pushresultsize(&b, l);
    return 1;
}

static int str_rep(lua_State* L)
{
    size_t l;
    const char* s = luaL_checklstring(L, 1, &l);
    int n = luaL_checkinteger(L, 2);

    if (n <= 0)
    {
        lua_pushliteral(L, "");
        return 1;
    }

    if (l > MAXSSIZE / (size_t)n) { // may overflow?
        #define STR_0 /*resulting string too large*/ scrypt("\x8e\x9b\x8d\x8b\x94\x8c\x97\x92\x99\xe0\x8d\x8c\x8e\x97\x92\x99\xe0\x8c\x91\x91\xe0\x94\x9f\x8e\x99\x9b").c_str()
        luaL_error(L, STR_0);
        #undef STR_0
    }

    luaL_Strbuf b;
    char* ptr = luaL_buffinitsize(L, &b, l * n);

    const char* start = ptr;

    size_t left = l * n;
    size_t step = l;

    memcpy(ptr, s, l);
    ptr += l;
    left -= l;

    // use the increasing 'pattern' inside our target buffer to fill the next part
    while (step < left)
    {
        memcpy(ptr, start, step);
        ptr += step;
        left -= step;
        step <<= 1;
    }

    // fill tail
    memcpy(ptr, start, left);
    ptr += left;

    luaL_pushresultsize(&b, l * n);

    return 1;
}

static int str_byte(lua_State* L)
{
    size_t l;
    const char* s = luaL_checklstring(L, 1, &l);
    int posi = posrelat(luaL_optinteger(L, 2, 1), l);
    int pose = posrelat(luaL_optinteger(L, 3, posi), l);
    int n, i;
    if (posi <= 0)
        posi = 1;
    if ((size_t)pose > l)
        pose = (int)l;
    if (posi > pose)
        return 0; // empty interval; return no values
    n = (int)(pose - posi + 1);
    /*string slice too long*/ scrypt_def(STR_0, "\x8d\x8c\x8e\x97\x92\x99\xe0\x8d\x94\x97\x9d\x9b\xe0\x8c\x91\x91\xe0\x94\x91\x92\x99");
    if (posi + n <= pose) // overflow?
        luaL_error(L, STR_0->c_str());
    luaL_checkstack(L, n, STR_0->c_str());
    for (i = 0; i < n; i++)
        lua_pushinteger(L, uchar(s[posi + i - 1]));
    return n;
}

static int str_char(lua_State* L)
{
    int n = lua_gettop(L); // number of arguments

    luaL_Strbuf b;
    char* ptr = luaL_buffinitsize(L, &b, n);
    /*invalid value*/ scrypt_def(STR_0, "\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x8a\x9f\x94\x8b\x9b");

    for (int i = 1; i <= n; i++)
    {
        int c = luaL_checkinteger(L, i);
        luaL_argcheck(L, uchar(c) == c, i, STR_0->c_str());

        *ptr++ = uchar(c);
    }
    luaL_pushresultsize(&b, n);
    return 1;
}

/*
** {======================================================
** PATTERN MATCHING
** =======================================================
*/

#define CAP_UNFINISHED (-1)
#define CAP_POSITION (-2)

typedef struct MatchState
{
    int matchdepth;       // control for recursive depth (to avoid C stack overflow)
    const char* src_init; // init of source string
    const char* src_end;  // end ('\0') of source string
    const char* p_end;    // end ('\0') of pattern
    lua_State* L;
    int level; // total number of captures (finished or unfinished)
    struct
    {
        const char* init;
        ptrdiff_t len;
    } capture[LUA_MAXCAPTURES];
} MatchState;

// recursive function
static const char* match(MatchState* ms, const char* s, const char* p);

#define L_ESC '%'
#define SPECIALS "^$*+?.([%-"

static int check_capture(MatchState* ms, int l)
{
    l -= '1';
    if (l < 0 || l >= ms->level || ms->capture[l].len == CAP_UNFINISHED) {
        #define STR_0 /*invalid capture index %%%d*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x9d\x9f\x90\x8c\x8b\x8e\x9b\xe0\x97\x92\x9c\x9b\x88\xe0\xdb\xdb\xdb\x9c").c_str()
        luaL_error(ms->L, STR_0, l + 1);
        #undef STR_0
    }
    return l;
}

static int capture_to_close(MatchState* ms)
{
    int level = ms->level;
    for (level--; level >= 0; level--)
        if (ms->capture[level].len == CAP_UNFINISHED)
            return level;
    #define STR_0 /*invalid pattern capture*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x90\x9f\x8c\x8c\x9b\x8e\x92\xe0\x9d\x9f\x90\x8c\x8b\x8e\x9b").c_str()
    luaL_error(ms->L, STR_0);
    #undef STR_0
}

static const char* classend(MatchState* ms, const char* p)
{
    switch (*p++)
    {
    case L_ESC:
    {
        if (p == ms->p_end) {
            #define STR_0 /*malformed pattern (ends with '%%')*/ scrypt("\x93\x9f\x94\x9a\x91\x8e\x93\x9b\x9c\xe0\x90\x9f\x8c\x8c\x9b\x8e\x92\xe0\xd8\x9b\x92\x9c\x8d\xe0\x89\x97\x8c\x98\xe0\xd9\xdb\xdb\xd9\xd7").c_str()
            luaL_error(ms->L, STR_0);
            #undef STR_0
        }
        return p + 1;
    }
    case '[':
    {
        if (*p == '^')
            p++;
        do
        { // look for a `]'
            if (p == ms->p_end) {
                #define STR_0 /*malformed pattern (missing ']')*/ scrypt("\x93\x9f\x94\x9a\x91\x8e\x93\x9b\x9c\xe0\x90\x9f\x8c\x8c\x9b\x8e\x92\xe0\xd8\x93\x97\x8d\x8d\x97\x92\x99\xe0\xd9\xa3\xd9\xd7").c_str()
                luaL_error(ms->L, STR_0);
                #undef STR_0
            }
            if (*(p++) == L_ESC && p < ms->p_end)
                p++; // skip escapes (e.g. `%]')
        } while (*p != ']');
        return p + 1;
    }
    default:
    {
        return p;
    }
    }
}

static int match_class(int c, int cl)
{
    int res;
    switch (tolower(cl))
    {
    case 'a':
        res = isalpha(c);
        break;
    case 'c':
        res = iscntrl(c);
        break;
    case 'd':
        res = isdigit(c);
        break;
    case 'g':
        res = isgraph(c);
        break;
    case 'l':
        res = islower(c);
        break;
    case 'p':
        res = ispunct(c);
        break;
    case 's':
        res = isspace(c);
        break;
    case 'u':
        res = isupper(c);
        break;
    case 'w':
        res = isalnum(c);
        break;
    case 'x':
        res = isxdigit(c);
        break;
    case 'z':
        res = (c == 0);
        break; // deprecated option
    default:
        return (cl == c);
    }
    return (islower(cl) ? res : !res);
}

static int matchbracketclass(int c, const char* p, const char* ec)
{
    int sig = 1;
    if (*(p + 1) == '^')
    {
        sig = 0;
        p++; // skip the `^'
    }
    while (++p < ec)
    {
        if (*p == L_ESC)
        {
            p++;
            if (match_class(c, uchar(*p)))
                return sig;
        }
        else if ((*(p + 1) == '-') && (p + 2 < ec))
        {
            p += 2;
            if (uchar(*(p - 2)) <= c && c <= uchar(*p))
                return sig;
        }
        else if (uchar(*p) == c)
            return sig;
    }
    return !sig;
}

static int singlematch(MatchState* ms, const char* s, const char* p, const char* ep)
{
    if (s >= ms->src_end)
        return 0;
    else
    {
        int c = uchar(*s);
        switch (*p)
        {
        case '.':
            return 1; // matches any char
        case L_ESC:
            return match_class(c, uchar(*(p + 1)));
        case '[':
            return matchbracketclass(c, p, ep - 1);
        default:
            return (uchar(*p) == c);
        }
    }
}

static const char* matchbalance(MatchState* ms, const char* s, const char* p)
{
    if (p >= ms->p_end - 1) {
        #define STR_0 /*malformed pattern (missing arguments to '%%b')*/ scrypt("\x93\x9f\x94\x9a\x91\x8e\x93\x9b\x9c\xe0\x90\x9f\x8c\x8c\x9b\x8e\x92\xe0\xd8\x93\x97\x8d\x8d\x97\x92\x99\xe0\x9f\x8e\x99\x8b\x93\x9b\x92\x8c\x8d\xe0\x8c\x91\xe0\xd9\xdb\xdb\x9e\xd9\xd7").c_str()
        luaL_error(ms->L, STR_0);
        #undef STR_0
    }
    if (*s != *p)
        return NULL;
    else
    {
        int b = *p;
        int e = *(p + 1);
        int cont = 1;
        while (++s < ms->src_end)
        {
            if (*s == e)
            {
                if (--cont == 0)
                    return s + 1;
            }
            else if (*s == b)
                cont++;
        }
    }
    return NULL; // string ends out of balance
}

static const char* max_expand(MatchState* ms, const char* s, const char* p, const char* ep)
{
    ptrdiff_t i = 0; // counts maximum expand for item
    while (singlematch(ms, s + i, p, ep))
        i++;
    // keeps trying to match with the maximum repetitions
    while (i >= 0)
    {
        const char* res = match(ms, (s + i), ep + 1);
        if (res)
            return res;
        i--; // else didn't match; reduce 1 repetition to try again
    }
    return NULL;
}

static const char* min_expand(MatchState* ms, const char* s, const char* p, const char* ep)
{
    for (;;)
    {
        const char* res = match(ms, s, ep + 1);
        if (res != NULL)
            return res;
        else if (singlematch(ms, s, p, ep))
            s++; // try with one more repetition
        else
            return NULL;
    }
}

static const char* start_capture(MatchState* ms, const char* s, const char* p, int what)
{
    const char* res;
    int level = ms->level;
    if (level >= LUA_MAXCAPTURES) {
        #define STR_0 /*too many captures*/ scrypt("\x8c\x91\x91\xe0\x93\x9f\x92\x87\xe0\x9d\x9f\x90\x8c\x8b\x8e\x9b\x8d").c_str()
        luaL_error(ms->L, STR_0);
        #undef STR_0
    }
    ms->capture[level].init = s;
    ms->capture[level].len = what;
    ms->level = level + 1;
    if ((res = match(ms, s, p)) == NULL) // match failed?
        ms->level--;                     // undo capture
    return res;
}

static const char* end_capture(MatchState* ms, const char* s, const char* p)
{
    int l = capture_to_close(ms);
    const char* res;
    ms->capture[l].len = s - ms->capture[l].init; // close capture
    if ((res = match(ms, s, p)) == NULL)          // match failed?
        ms->capture[l].len = CAP_UNFINISHED;      // undo capture
    return res;
}

static const char* match_capture(MatchState* ms, const char* s, int l)
{
    size_t len;
    l = check_capture(ms, l);
    len = ms->capture[l].len;
    if ((size_t)(ms->src_end - s) >= len && memcmp(ms->capture[l].init, s, len) == 0)
        return s + len;
    else
        return NULL;
}

static const char* match(MatchState* ms, const char* s, const char* p)
{
    if (ms->matchdepth-- == 0) {
        #define STR_0 /*pattern too complex*/ scrypt("\x90\x9f\x8c\x8c\x9b\x8e\x92\xe0\x8c\x91\x91\xe0\x9d\x91\x93\x90\x94\x9b\x88").c_str()
        luaL_error(ms->L, STR_0);
        #undef STR_0
    }

    lua_State* L = ms->L;
    void (*interrupt)(lua_State*, int) = L->global->cb.interrupt;

    if (LUAU_UNLIKELY(!!interrupt))
    {
        // this interrupt is not yieldable
        L->nCcalls++;
        interrupt(L, -1);
        L->nCcalls--;
    }

init: // using goto's to optimize tail recursion
    if (p != ms->p_end)
    { // end of pattern?
        switch (*p)
        {
        case '(':
        {                        // start capture
            if (*(p + 1) == ')') // position capture?
                s = start_capture(ms, s, p + 2, CAP_POSITION);
            else
                s = start_capture(ms, s, p + 1, CAP_UNFINISHED);
            break;
        }
        case ')':
        { // end capture
            s = end_capture(ms, s, p + 1);
            break;
        }
        case '$':
        {
            if ((p + 1) != ms->p_end)          // is the `$' the last char in pattern?
                goto dflt;                     // no; go to default
            s = (s == ms->src_end) ? s : NULL; // check end of string
            break;
        }
        case L_ESC:
        { // escaped sequences not in the format class[*+?-]?
            switch (*(p + 1))
            {
            case 'b':
            { // balanced string?
                s = matchbalance(ms, s, p + 2);
                if (s != NULL)
                {
                    p += 4;
                    goto init; // return match(ms, s, p + 4);
                }              // else fail (s == NULL)
                break;
            }
            case 'f':
            { // frontier?
                const char* ep;
                char previous;
                p += 2;
                if (*p != '[') {
                    #define STR_1 /*missing '[' after '%%f' in pattern*/ scrypt("\x93\x97\x8d\x8d\x97\x92\x99\xe0\xd9\xa5\xd9\xe0\x9f\x9a\x8c\x9b\x8e\xe0\xd9\xdb\xdb\x9a\xd9\xe0\x97\x92\xe0\x90\x9f\x8c\x8c\x9b\x8e\x92").c_str()
                    luaL_error(ms->L, STR_1);
                    #undef STR_1
                }
                ep = classend(ms, p); // points to what is next
                previous = (s == ms->src_init) ? '\0' : *(s - 1);
                if (!matchbracketclass(uchar(previous), p, ep - 1) && matchbracketclass(uchar(*s), p, ep - 1))
                {
                    p = ep;
                    goto init; // return match(ms, s, ep);
                }
                s = NULL; // match failed
                break;
            }
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            { // capture results (%0-%9)?
                s = match_capture(ms, s, uchar(*(p + 1)));
                if (s != NULL)
                {
                    p += 2;
                    goto init; // return match(ms, s, p + 2)
                }
                break;
            }
            default:
                goto dflt;
            }
            break;
        }
        default:
        dflt:
        {                                     // pattern class plus optional suffix
            const char* ep = classend(ms, p); // points to optional suffix
            // does not match at least once?
            if (!singlematch(ms, s, p, ep))
            {
                if (*ep == '*' || *ep == '?' || *ep == '-')
                { // accept empty?
                    p = ep + 1;
                    goto init; // return match(ms, s, ep + 1);
                }
                else          // '+' or no suffix
                    s = NULL; // fail
            }
            else
            { // matched once
                switch (*ep)
                { // handle optional suffix
                case '?':
                { // optional
                    const char* res;
                    if ((res = match(ms, s + 1, ep + 1)) != NULL)
                        s = res;
                    else
                    {
                        p = ep + 1;
                        goto init; // else return match(ms, s, ep + 1);
                    }
                    break;
                }
                case '+':             // 1 or more repetitions
                    s++;              // 1 match already done
                    LUAU_FALLTHROUGH; // go through
                case '*': // 0 or more repetitions
                    s = max_expand(ms, s, p, ep);
                    break;
                case '-': // 0 or more repetitions (minimum)
                    s = min_expand(ms, s, p, ep);
                    break;
                default: // no suffix
                    s++;
                    p = ep;
                    goto init; // return match(ms, s + 1, ep);
                }
            }
            break;
        }
        }
    }
    ms->matchdepth++;
    return s;
}

static const char* lmemfind(const char* s1, size_t l1, const char* s2, size_t l2)
{
    if (l2 == 0)
        return s1; // empty strings are everywhere
    else if (l2 > l1)
        return NULL; // avoids a negative `l1'
    else
    {
        const char* init; // to search for a `*s2' inside `s1'
        l2--;             // 1st char will be checked by `memchr'
        l1 = l1 - l2;     // `s2' cannot be found after that
        while (l1 > 0 && (init = (const char*)memchr(s1, *s2, l1)) != NULL)
        {
            init++; // 1st char is already checked
            if (memcmp(init, s2 + 1, l2) == 0)
                return init - 1;
            else
            { // correct `l1' and `s1' to try again
                l1 -= init - s1;
                s1 = init;
            }
        }
        return NULL; // not found
    }
}

static void push_onecapture(MatchState* ms, int i, const char* s, const char* e)
{
    if (i >= ms->level)
    {
        if (i == 0)                           // ms->level == 0, too
            lua_pushlstring(ms->L, s, e - s); // add whole match
        else {
            #define STR_0 /*invalid capture index*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x9d\x9f\x90\x8c\x8b\x8e\x9b\xe0\x97\x92\x9c\x9b\x88").c_str()
            luaL_error(ms->L, STR_0);
            #undef STR_0
        }
    }
    else
    {
        ptrdiff_t l = ms->capture[i].len;
        if (l == CAP_UNFINISHED) {
            #define STR_1 /*unfinished capture*/ scrypt("\x8b\x92\x9a\x97\x92\x97\x8d\x98\x9b\x9c\xe0\x9d\x9f\x90\x8c\x8b\x8e\x9b").c_str()
            luaL_error(ms->L, STR_1);
            #undef STR_1
        }
        if (l == CAP_POSITION)
            lua_pushinteger(ms->L, (int)(ms->capture[i].init - ms->src_init) + 1);
        else
            lua_pushlstring(ms->L, ms->capture[i].init, l);
    }
}

static int push_captures(MatchState* ms, const char* s, const char* e)
{
    int i;
    int nlevels = (ms->level == 0 && s) ? 1 : ms->level;
    /*too many captures*/ scrypt_def(STR_0, "\x8c\x91\x91\xe0\x93\x9f\x92\x87\xe0\x9d\x9f\x90\x8c\x8b\x8e\x9b\x8d");
    luaL_checkstack(ms->L, nlevels, STR_0->c_str());
    for (i = 0; i < nlevels; i++)
        push_onecapture(ms, i, s, e);
    return nlevels; // number of strings pushed
}

// check whether pattern has no special characters
static int nospecials(const char* p, size_t l)
{
    /*^$*+?.([%-*/ scrypt_def(STR_0, "\xa2\xdc\xd6\xd5\xc1\xd2\xd8\xa5\xdb\xd3");
    size_t upto = 0;
    do
    {
        if (strpbrk(p + upto, STR_0->c_str()))
            return 0;                 // pattern has a special character
        upto += strlen(p + upto) + 1; // may have more after \0
    } while (upto <= l);
    return 1; // no special chars found
}

static void prepstate(MatchState* ms, lua_State* L, const char* s, size_t ls, const char* p, size_t lp)
{
    ms->L = L;
    ms->matchdepth = LUAI_MAXCCALLS;
    ms->src_init = s;
    ms->src_end = s + ls;
    ms->p_end = p + lp;
}

static void reprepstate(MatchState* ms)
{
    ms->level = 0;
    LUAU_ASSERT(ms->matchdepth == LUAI_MAXCCALLS);
}

static int str_find_aux(lua_State* L, int find)
{
    size_t ls, lp;
    const char* s = luaL_checklstring(L, 1, &ls);
    const char* p = luaL_checklstring(L, 2, &lp);
    int init = posrelat(luaL_optinteger(L, 3, 1), ls);
    if (init < 1)
        init = 1;
    else if (init > (int)ls + 1)
    {                   // start after string's end?
        lua_pushnil(L); // cannot find anything
        return 1;
    }
    // explicit request or no special characters?
    if (find && (lua_toboolean(L, 4) || nospecials(p, lp)))
    {
        // do a plain search
        const char* s2 = lmemfind(s + init - 1, ls - init + 1, p, lp);
        if (s2)
        {
            lua_pushinteger(L, (int)(s2 - s + 1));
            lua_pushinteger(L, (int)(s2 - s + lp));
            return 2;
        }
    }
    else
    {
        MatchState ms;
        const char* s1 = s + init - 1;
        int anchor = (*p == '^');
        if (anchor)
        {
            p++;
            lp--; // skip anchor character
        }
        prepstate(&ms, L, s, ls, p, lp);
        do
        {
            const char* res;
            reprepstate(&ms);
            if ((res = match(&ms, s1, p)) != NULL)
            {
                if (find)
                {
                    lua_pushinteger(L, (int)(s1 - s + 1)); // start
                    lua_pushinteger(L, (int)(res - s));    // end
                    return push_captures(&ms, NULL, 0) + 2;
                }
                else
                    return push_captures(&ms, s1, res);
            }
        } while (s1++ < ms.src_end && !anchor);
    }
    lua_pushnil(L); // not found
    return 1;
}

static int str_find(lua_State* L)
{
    return str_find_aux(L, 1);
}

static int str_match(lua_State* L)
{
    return str_find_aux(L, 0);
}

static int gmatch_aux(lua_State* L)
{
    MatchState ms;
    size_t ls, lp;
    const char* s = lua_tolstring(L, lua_upvalueindex(1), &ls);
    const char* p = lua_tolstring(L, lua_upvalueindex(2), &lp);
    const char* src;
    prepstate(&ms, L, s, ls, p, lp);
    for (src = s + (size_t)lua_tointeger(L, lua_upvalueindex(3)); src <= ms.src_end; src++)
    {
        const char* e;
        reprepstate(&ms);
        if ((e = match(&ms, src, p)) != NULL)
        {
            int newstart = (int)(e - s);
            if (e == src)
                newstart++; // empty match? go at least one position
            lua_pushinteger(L, newstart);
            lua_replace(L, lua_upvalueindex(3));
            return push_captures(&ms, src, e);
        }
    }
    return 0; // not found
}

static int gmatch(lua_State* L)
{
    luaL_checkstring(L, 1);
    luaL_checkstring(L, 2);
    lua_settop(L, 2);
    lua_pushinteger(L, 0);
    lua_pushcclosure(L, gmatch_aux, NULL, 3);
    return 1;
}

static void add_s(MatchState* ms, luaL_Strbuf* b, const char* s, const char* e)
{
    size_t l, i;
    const char* news = lua_tolstring(ms->L, 3, &l);

    luaL_prepbuffsize(b, l);

    for (i = 0; i < l; i++)
    {
        if (news[i] != L_ESC)
            luaL_addchar(b, news[i]);
        else
        {
            i++; // skip ESC
            if (!isdigit(uchar(news[i])))
            {
                if (news[i] != L_ESC) {
                    #define STR_0 /*invalid use of '%c' in replacement string*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x8b\x8d\x9b\xe0\x91\x9a\xe0\xd9\xdb\x9d\xd9\xe0\x97\x92\xe0\x8e\x9b\x90\x94\x9f\x9d\x9b\x93\x9b\x92\x8c\xe0\x8d\x8c\x8e\x97\x92\x99").c_str()
                    luaL_error(ms->L, STR_0, L_ESC);
                    #undef STR_0
                }
                luaL_addchar(b, news[i]);
            }
            else if (news[i] == '0')
                luaL_addlstring(b, s, e - s);
            else
            {
                push_onecapture(ms, news[i] - '1', s, e);
                luaL_addvalue(b); // add capture to accumulated result
            }
        }
    }
}

static void add_value(MatchState* ms, luaL_Strbuf* b, const char* s, const char* e, int tr)
{
    lua_State* L = ms->L;
    switch (tr)
    {
    case LUA_TFUNCTION:
    {
        int n;
        lua_pushvalue(L, 3);
        n = push_captures(ms, s, e);
        lua_call(L, n, 1);
        break;
    }
    case LUA_TTABLE:
    {
        push_onecapture(ms, 0, s, e);
        lua_gettable(L, 3);
        break;
    }
    default:
    { // LUA_TNUMBER or LUA_TSTRING
        add_s(ms, b, s, e);
        return;
    }
    }
    if (!lua_toboolean(L, -1))
    { // nil or false?
        lua_pop(L, 1);
        lua_pushlstring(L, s, e - s); // keep original text
    }
    else if (!lua_isstring(L, -1)) {
        #define STR_0 /*invalid replacement value (a %s)*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x8e\x9b\x90\x94\x9f\x9d\x9b\x93\x9b\x92\x8c\xe0\x8a\x9f\x94\x8b\x9b\xe0\xd8\x9f\xe0\xdb\x8d\xd7").c_str()
        luaL_error(L, STR_0, luaL_typename(L, -1));
        #undef STR_0
    }
    luaL_addvalue(b); // add result to accumulator
}

static int str_gsub(lua_State* L)
{
    /*string/function/table*/ scrypt_def(STR_0, "\x8d\x8c\x8e\x97\x92\x99\xd1\x9a\x8b\x92\x9d\x8c\x97\x91\x92\xd1\x8c\x9f\x9e\x94\x9b");

    size_t srcl, lp;
    const char* src = luaL_checklstring(L, 1, &srcl);
    const char* p = luaL_checklstring(L, 2, &lp);
    int tr = lua_type(L, 3);
    int max_s = luaL_optinteger(L, 4, (int)srcl + 1);
    int anchor = (*p == '^');
    int n = 0;
    MatchState ms;
    luaL_Strbuf b;
    luaL_argexpected(L, tr == LUA_TNUMBER || tr == LUA_TSTRING || tr == LUA_TFUNCTION || tr == LUA_TTABLE, 3, STR_0->c_str());
    luaL_buffinit(L, &b);
    if (anchor)
    {
        p++;
        lp--; // skip anchor character
    }
    prepstate(&ms, L, src, srcl, p, lp);
    while (n < max_s)
    {
        const char* e;
        reprepstate(&ms);
        e = match(&ms, src, p);
        if (e)
        {
            n++;
            add_value(&ms, &b, src, e, tr);
        }
        if (e && e > src) // non empty match?
            src = e;      // skip it
        else if (src < ms.src_end)
            luaL_addchar(&b, *src++);
        else
            break;
        if (anchor)
            break;
    }
    luaL_addlstring(&b, src, ms.src_end - src);
    luaL_pushresult(&b);
    lua_pushinteger(L, n); // number of substitutions
    return 2;
}

// }======================================================

// valid flags in a format specification
#define FLAGS "-+ #0"
// maximum size of each formatted item (> len(format('%99.99f', -1e308)))
#define MAX_ITEM 512
// maximum size of each format specification (such as '%-099.99d')
#define MAX_FORMAT 32

static void addquoted(lua_State* L, luaL_Strbuf* b, int arg)
{
    size_t l;
    const char* s = luaL_checklstring(L, arg, &l);

    luaL_prepbuffsize(b, l + 2);

    luaL_addchar(b, '"');
    while (l--)
    {
        switch (*s)
        {
        case '"':
        case '\\':
        case '\n':
        {
            luaL_addchar(b, '\\');
            luaL_addchar(b, *s);
            break;
        }
        case '\r':
        {
            luaL_addlstring(b, "\\r", 2);
            break;
        }
        case '\0':
        {
            luaL_addlstring(b, "\\000", 4);
            break;
        }
        default:
        {
            luaL_addchar(b, *s);
            break;
        }
        }
        s++;
    }
    luaL_addchar(b, '"');
}

static const char* scanformat(lua_State* L, const char* strfrmt, char* form, size_t* size)
{
    /*-+ #0*/ scrypt_def(STR_0, "\xd3\xd5\xe0\xdd\xd0");
    const char* p = strfrmt;
    while (*p != '\0' && strchr(STR_0->c_str(), *p) != NULL)
        p++; // skip flags
    if ((size_t)(p - strfrmt) >= sizeof(FLAGS)) {
        #define STR_0 /*invalid format (repeated flags)*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x9a\x91\x8e\x93\x9f\x8c\xe0\xd8\x8e\x9b\x90\x9b\x9f\x8c\x9b\x9c\xe0\x9a\x94\x9f\x99\x8d\xd7").c_str()
        luaL_error(L, STR_0);
        #undef STR_0
    }
    if (isdigit(uchar(*p)))
        p++; // skip width
    if (isdigit(uchar(*p)))
        p++; // (2 digits at most)
    if (*p == '.')
    {
        p++;
        if (isdigit(uchar(*p)))
            p++; // skip precision
        if (isdigit(uchar(*p)))
            p++; // (2 digits at most)
    }
    if (isdigit(uchar(*p))) {
        #define STR_0 /*invalid format (width or precision too long)*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x9a\x91\x8e\x93\x9f\x8c\xe0\xd8\x89\x97\x9c\x8c\x98\xe0\x91\x8e\xe0\x90\x8e\x9b\x9d\x97\x8d\x97\x91\x92\xe0\x8c\x91\x91\xe0\x94\x91\x92\x99\xd7").c_str()
        luaL_error(L, STR_0);
        #undef STR_0
    }
    *(form++) = '%';
    *size = p - strfrmt + 1;
    strncpy(form, strfrmt, *size);
    form += *size;
    *form = '\0';
    return p;
}

static void addInt64Format(char form[MAX_FORMAT], char formatIndicator, size_t formatItemSize)
{
    LUAU_ASSERT((formatItemSize + 3) <= MAX_FORMAT);
    LUAU_ASSERT(form[0] == '%');
    LUAU_ASSERT(form[formatItemSize] != 0);
    LUAU_ASSERT(form[formatItemSize + 1] == 0);
    form[formatItemSize + 0] = 'l';
    form[formatItemSize + 1] = 'l';
    form[formatItemSize + 2] = formatIndicator;
    form[formatItemSize + 3] = 0;
}

static int str_format(lua_State* L)
{
    int top = lua_gettop(L);
    int arg = 1;
    size_t sfl;
    const char* strfrmt = luaL_checklstring(L, arg, &sfl);
    const char* strfrmt_end = strfrmt + sfl;
    luaL_Strbuf b;
    luaL_buffinit(L, &b);
    while (strfrmt < strfrmt_end)
    {
        if (*strfrmt != L_ESC)
            luaL_addchar(&b, *strfrmt++);
        else if (*++strfrmt == L_ESC)
            luaL_addchar(&b, *strfrmt++); // %%
        else if (*strfrmt == '*')
        {
            strfrmt++;
            if (++arg > top) {
                #define STR_0 /*missing argument #%d*/ scrypt("\x93\x97\x8d\x8d\x97\x92\x99\xe0\x9f\x8e\x99\x8b\x93\x9b\x92\x8c\xe0\xdd\xdb\x9c").c_str()
                luaL_error(L, STR_0, arg);
                #undef STR_0
            }

            luaL_addvalueany(&b, arg);
        }
        else
        {                          // format item
            char form[MAX_FORMAT]; // to store the format (`%...')
            char buff[MAX_ITEM];   // to store the formatted item
            if (++arg > top) {
                #define STR_1 /*missing argument #%d*/ scrypt("\x93\x97\x8d\x8d\x97\x92\x99\xe0\x9f\x8e\x99\x8b\x93\x9b\x92\x8c\xe0\xdd\xdb\x9c").c_str()
                luaL_error(L, STR_1, arg);
                #undef STR_1
            }
            size_t formatItemSize = 0;
            strfrmt = scanformat(L, strfrmt, form, &formatItemSize);
            char formatIndicator = *strfrmt++;
            switch (formatIndicator)
            {
            case 'c':
            {
                int count = snprintf(buff, sizeof(buff), form, (int)luaL_checknumber(L, arg));
                luaL_addlstring(&b, buff, count);
                continue; // skip the 'luaL_addlstring' at the end
            }
            case 'd':
            case 'i':
            {
                addInt64Format(form, formatIndicator, formatItemSize);
                snprintf(buff, sizeof(buff), form, (long long)luaL_checknumber(L, arg));
                break;
            }
            case 'o':
            case 'u':
            case 'x':
            case 'X':
            {
                double argValue = luaL_checknumber(L, arg);
                addInt64Format(form, formatIndicator, formatItemSize);
                unsigned long long v = (argValue < 0) ? (unsigned long long)(long long)argValue : (unsigned long long)argValue;
                snprintf(buff, sizeof(buff), form, v);
                break;
            }
            case 'e':
            case 'E':
            case 'f':
            case 'g':
            case 'G':
            {
                snprintf(buff, sizeof(buff), form, (double)luaL_checknumber(L, arg));
                break;
            }
            case 'q':
            {
                addquoted(L, &b, arg);
                continue; // skip the 'luaL_addlstring' at the end
            }
            case 's':
            {
                size_t l;
                const char* s = luaL_checklstring(L, arg, &l);
                // no precision and string is too long to be formatted, or no format necessary to begin with
                if (form[2] == '\0' || (!strchr(form, '.') && l >= 100))
                {
                    luaL_addlstring(&b, s, l);
                    continue; // skip the `luaL_addlstring' at the end
                }
                else
                {
                    snprintf(buff, sizeof(buff), form, s);
                    break;
                }
            }
            case '*':
            {
                // %* is parsed above, so if we got here we must have %...*
                #define STR_2 /*'%%*' does not take a form*/ scrypt("\xd9\xdb\xdb\xd6\xd9\xe0\x9c\x91\x9b\x8d\xe0\x92\x91\x8c\xe0\x8c\x9f\x95\x9b\xe0\x9f\xe0\x9a\x91\x8e\x93").c_str()
                luaL_error(L, STR_2);
                #undef STR_2
            }
            default:
            { // also treat cases `pnLlh'
                #define STR_3 /*invalid option '%%%c' to 'format'*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x91\x90\x8c\x97\x91\x92\xe0\xd9\xdb\xdb\xdb\x9d\xd9\xe0\x8c\x91\xe0\xd9\x9a\x91\x8e\x93\x9f\x8c\xd9").c_str()
                luaL_error(L, STR_3, *(strfrmt - 1));
                #undef STR_3
            }
            }
            luaL_addlstring(&b, buff, strlen(buff));
        }
    }
    luaL_pushresult(&b);
    return 1;
}

static int str_split(lua_State* L)
{
    size_t haystackLen;
    const char* haystack = luaL_checklstring(L, 1, &haystackLen);
    size_t needleLen;
    const char* needle = luaL_optlstring(L, 2, ",", &needleLen);

    const char* begin = haystack;
    const char* end = haystack + haystackLen;
    const char* spanStart = begin;
    int numMatches = 0;

    lua_createtable(L, 0, 0);

    if (needleLen == 0)
        begin++;

    // Don't iterate the last needleLen - 1 bytes of the string - they are
    // impossible to be splits and would let us memcmp past the end of the
    // buffer.
    for (const char* iter = begin; iter <= end - needleLen; iter++)
    {
        // Use of memcmp here instead of strncmp is so that we allow embedded
        // nulls to be used in either of the haystack or the needle strings.
        // Most Lua string APIs allow embedded nulls, and this should be no
        // exception.
        if (memcmp(iter, needle, needleLen) == 0)
        {
            lua_pushinteger(L, ++numMatches);
            lua_pushlstring(L, spanStart, iter - spanStart);
            lua_settable(L, -3);

            spanStart = iter + needleLen;
            if (needleLen > 0)
                iter += needleLen - 1;
        }
    }

    if (needleLen > 0)
    {
        lua_pushinteger(L, ++numMatches);
        lua_pushlstring(L, spanStart, end - spanStart);
        lua_settable(L, -3);
    }

    return 1;
}

/*
** {======================================================
** PACK/UNPACK
** =======================================================
*/

// value used for padding
#if !defined(LUAL_PACKPADBYTE)
#define LUAL_PACKPADBYTE 0x00
#endif

// maximum size for the binary representation of an integer
#define MAXINTSIZE 16

// number of bits in a character
#define NB CHAR_BIT

// mask for one character (NB 1's)
#define MC ((1 << NB) - 1)

// internal size of integers used for pack/unpack
#define SZINT (int)sizeof(long long)

// dummy union to get native endianness
static const union
{
    int dummy;
    char little; // true iff machine is little endian
} nativeendian = {1};

// assume we need to align for double & pointers
#define MAXALIGN 8

/*
** Union for serializing floats
*/
typedef union Ftypes
{
    float f;
    double d;
    double n;
    char buff[5 * sizeof(double)]; // enough for any float type
} Ftypes;

/*
** information to pack/unpack stuff
*/
typedef struct Header
{
    lua_State* L;
    int islittle;
    int maxalign;
} Header;

/*
** options for pack/unpack
*/
typedef enum KOption
{
    Kint,       // signed integers
    Kuint,      // unsigned integers
    Kfloat,     // floating-point numbers
    Kchar,      // fixed-length strings
    Kstring,    // strings with prefixed length
    Kzstr,      // zero-terminated strings
    Kpadding,   // padding
    Kpaddalign, // padding for alignment
    Knop        // no-op (configuration or spaces)
} KOption;

/*
** Read an integer numeral from string 'fmt' or return 'df' if
** there is no numeral
*/
static int digit(int c)
{
    return '0' <= c && c <= '9';
}

static int getnum(Header* h, const char** fmt, int df)
{
    if (!digit(**fmt)) // no number?
        return df;     // return default value
    else
    {
        int a = 0;
        do
        {
            a = a * 10 + (*((*fmt)++) - '0');
        } while (digit(**fmt) && a <= (INT_MAX - 9) / 10);
        if (a > MAXSSIZE || digit(**fmt)) {
            #define STR_0 /*size specifier is too large*/ scrypt("\x8d\x97\x86\x9b\xe0\x8d\x90\x9b\x9d\x97\x9a\x97\x9b\x8e\xe0\x97\x8d\xe0\x8c\x91\x91\xe0\x94\x9f\x8e\x99\x9b").c_str()
            luaL_error(h->L, STR_0);
            #undef STR_0
        }
        return a;
    }
}

/*
** Read an integer numeral and raises an error if it is larger
** than the maximum size for integers.
*/
static int getnumlimit(Header* h, const char** fmt, int df)
{
    int sz = getnum(h, fmt, df);
    if (sz > MAXINTSIZE || sz <= 0) {
        #define STR_0 /*integral size (%d) out of limits [1,%d]*/ scrypt("\x97\x92\x8c\x9b\x99\x8e\x9f\x94\xe0\x8d\x97\x86\x9b\xe0\xd8\xdb\x9c\xd7\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x94\x97\x93\x97\x8c\x8d\xe0\xa5\xcf\xd4\xdb\x9c\xa3").c_str()
        luaL_error(h->L, STR_0, sz, MAXINTSIZE);
        #undef STR_0
    }
    return sz;
}

/*
** Initialize Header
*/
static void initheader(lua_State* L, Header* h)
{
    h->L = L;
    h->islittle = nativeendian.little;
    h->maxalign = 1;
}

/*
** Read and classify next option. 'size' is filled with option's size.
*/
static KOption getoption(Header* h, const char** fmt, int* size)
{
    int opt = *((*fmt)++);
    *size = 0; // default
    switch (opt)
    {
    case 'b':
        *size = 1;
        return Kint;
    case 'B':
        *size = 1;
        return Kuint;
    case 'h':
        *size = 2;
        return Kint;
    case 'H':
        *size = 2;
        return Kuint;
    case 'l':
        *size = 8;
        return Kint;
    case 'L':
        *size = 8;
        return Kuint;
    case 'j':
        *size = 4;
        return Kint;
    case 'J':
        *size = 4;
        return Kuint;
    case 'T':
        *size = 4;
        return Kuint;
    case 'f':
        *size = 4;
        return Kfloat;
    case 'd':
        *size = 8;
        return Kfloat;
    case 'n':
        *size = 8;
        return Kfloat;
    case 'i':
        *size = getnumlimit(h, fmt, 4);
        return Kint;
    case 'I':
        *size = getnumlimit(h, fmt, 4);
        return Kuint;
    case 's':
        *size = getnumlimit(h, fmt, 4);
        return Kstring;
    case 'c':
        *size = getnum(h, fmt, -1);
        if (*size == -1) {
            #define STR_0 /*missing size for format option 'c'*/ scrypt("\x93\x97\x8d\x8d\x97\x92\x99\xe0\x8d\x97\x86\x9b\xe0\x9a\x91\x8e\xe0\x9a\x91\x8e\x93\x9f\x8c\xe0\x91\x90\x8c\x97\x91\x92\xe0\xd9\x9d\xd9").c_str()
            luaL_error(h->L, STR_0);
            #undef STR_0
        }
        return Kchar;
    case 'z':
        return Kzstr;
    case 'x':
        *size = 1;
        return Kpadding;
    case 'X':
        return Kpaddalign;
    case ' ':
        break;
    case '<':
        h->islittle = 1;
        break;
    case '>':
        h->islittle = 0;
        break;
    case '=':
        h->islittle = nativeendian.little;
        break;
    case '!':
        h->maxalign = getnumlimit(h, fmt, MAXALIGN);
        break;
    default:
        #define STR_1 /*invalid format option '%c'*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x9a\x91\x8e\x93\x9f\x8c\xe0\x91\x90\x8c\x97\x91\x92\xe0\xd9\xdb\x9d\xd9").c_str()
        luaL_error(h->L, STR_1, opt);
        #undef STR_1
    }
    return Knop;
}

/*
** Read, classify, and fill other details about the next option.
** 'psize' is filled with option's size, 'notoalign' with its
** alignment requirements.
** Local variable 'size' gets the size to be aligned. (Kpadal option
** always gets its full alignment, other options are limited by
** the maximum alignment ('maxalign'). Kchar option needs no alignment
** despite its size.
*/
static KOption getdetails(Header* h, size_t totalsize, const char** fmt, int* psize, int* ntoalign)
{
    KOption opt = getoption(h, fmt, psize);
    int align = *psize; // usually, alignment follows size
    if (opt == Kpaddalign)
    { // 'X' gets alignment from following option
        if (**fmt == '\0' || getoption(h, fmt, &align) == Kchar || align == 0) {
            #define STR_0 /*invalid next option for option 'X'*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x92\x9b\x88\x8c\xe0\x91\x90\x8c\x97\x91\x92\xe0\x9a\x91\x8e\xe0\x91\x90\x8c\x97\x91\x92\xe0\xd9\xa8\xd9").c_str()
            luaL_argerror(h->L, 1, STR_0);
            #undef STR_0
        }
    }
    if (align <= 1 || opt == Kchar) // need no alignment?
        *ntoalign = 0;
    else
    {
        if (align > h->maxalign) // enforce maximum alignment
            align = h->maxalign;
        if ((align & (align - 1)) != 0) { // is 'align' not a power of 2?
            #define STR_1 /*format asks for alignment not power of 2*/ scrypt("\x9a\x91\x8e\x93\x9f\x8c\xe0\x9f\x8d\x95\x8d\xe0\x9a\x91\x8e\xe0\x9f\x94\x97\x99\x92\x93\x9b\x92\x8c\xe0\x92\x91\x8c\xe0\x90\x91\x89\x9b\x8e\xe0\x91\x9a\xe0\xce").c_str()
            luaL_argerror(h->L, 1, STR_1);
            #undef STR_1
        }
        *ntoalign = (align - (int)(totalsize & (align - 1))) & (align - 1);
    }
    return opt;
}

/*
** Pack integer 'n' with 'size' bytes and 'islittle' endianness.
** The final 'if' handles the case when 'size' is larger than
** the size of a Lua integer, correcting the extra sign-extension
** bytes if necessary (by default they would be zeros).
*/
static void packint(luaL_Strbuf* b, unsigned long long n, int islittle, int size, int neg)
{
    LUAU_ASSERT(size <= MAXINTSIZE);
    char buff[MAXINTSIZE];
    int i;
    buff[islittle ? 0 : size - 1] = (char)(n & MC); // first byte
    for (i = 1; i < size; i++)
    {
        n >>= NB;
        buff[islittle ? i : size - 1 - i] = (char)(n & MC);
    }
    if (neg && size > SZINT)
    {                                  // negative number need sign extension?
        for (i = SZINT; i < size; i++) // correct extra bytes
            buff[islittle ? i : size - 1 - i] = (char)MC;
    }
    luaL_addlstring(b, buff, size); // add result to buffer
}

/*
** Copy 'size' bytes from 'src' to 'dest', correcting endianness if
** given 'islittle' is different from native endianness.
*/
static void copywithendian(volatile char* dest, volatile const char* src, int size, int islittle)
{
    if (islittle == nativeendian.little)
    {
        while (size-- != 0)
            *(dest++) = *(src++);
    }
    else
    {
        dest += size - 1;
        while (size-- != 0)
            *(dest--) = *(src++);
    }
}

static int str_pack(lua_State* L)
{
    luaL_Strbuf b;
    Header h;
    const char* fmt = luaL_checkstring(L, 1); // format string
    int arg = 1;                              // current argument to pack
    size_t totalsize = 0;                     // accumulate total size of result
    initheader(L, &h);
    lua_pushnil(L); // mark to separate arguments from string buffer
    luaL_buffinit(L, &b);
    while (*fmt != '\0')
    {
        int size, ntoalign;
        KOption opt = getdetails(&h, totalsize, &fmt, &size, &ntoalign);
        totalsize += ntoalign + size;
        while (ntoalign-- > 0)
            luaL_addchar(&b, LUAL_PACKPADBYTE); // fill alignment
        arg++;
        switch (opt)
        {
        case Kint:
        { // signed integers
            long long n = (long long)luaL_checknumber(L, arg);
            if (size < SZINT)
            { // need overflow check?
                /*integer overflow*/ scrypt_def(STR_0, "\x97\x92\x8c\x9b\x99\x9b\x8e\xe0\x91\x8a\x9b\x8e\x9a\x94\x91\x89");
                long long lim = (long long)1 << ((size * NB) - 1);
                luaL_argcheck(L, -lim <= n && n < lim, arg, STR_0->c_str());
            }
            packint(&b, n, h.islittle, size, (n < 0));
            break;
        }
        case Kuint:
        { // unsigned integers
            long long n = (long long)luaL_checknumber(L, arg);
            if (size < SZINT) { // need overflow check?
                /*unsigned overflow*/ scrypt_def(STR_1, "\x8b\x92\x8d\x97\x99\x92\x9b\x9c\xe0\x91\x8a\x9b\x8e\x9a\x94\x91\x89");
                luaL_argcheck(L, (unsigned long long)n < ((unsigned long long)1 << (size * NB)), arg, STR_1->c_str());
            }
            packint(&b, (unsigned long long)n, h.islittle, size, 0);
            break;
        }
        case Kfloat:
        { // floating-point options
            volatile Ftypes u;
            char buff[MAXINTSIZE];
            double n = luaL_checknumber(L, arg); // get argument
            if (size == sizeof(u.f))
                u.f = (float)n; // copy it into 'u'
            else if (size == sizeof(u.d))
                u.d = (double)n;
            else
                u.n = n;
            // move 'u' to final result, correcting endianness if needed
            copywithendian(buff, u.buff, size, h.islittle);
            luaL_addlstring(&b, buff, size);
            break;
        }
        case Kchar:
        { // fixed-size string
            size_t len;
            const char* s = luaL_checklstring(L, arg, &len);
            /*string longer than given size*/ scrypt_def(STR_2, "\x8d\x8c\x8e\x97\x92\x99\xe0\x94\x91\x92\x99\x9b\x8e\xe0\x8c\x98\x9f\x92\xe0\x99\x97\x8a\x9b\x92\xe0\x8d\x97\x86\x9b");
            luaL_argcheck(L, len <= (size_t)size, arg, STR_2->c_str());
            luaL_addlstring(&b, s, len); // add string
            while (len++ < (size_t)size) // pad extra space
                luaL_addchar(&b, LUAL_PACKPADBYTE);
            break;
        }
        case Kstring:
        { // strings with length count
            size_t len;
            const char* s = luaL_checklstring(L, arg, &len);
            /*string length does not fit in given size*/ scrypt_def(STR_3, "\x8d\x8c\x8e\x97\x92\x99\xe0\x94\x9b\x92\x99\x8c\x98\xe0\x9c\x91\x9b\x8d\xe0\x92\x91\x8c\xe0\x9a\x97\x8c\xe0\x97\x92\xe0\x99\x97\x8a\x9b\x92\xe0\x8d\x97\x86\x9b");
            luaL_argcheck(L, size >= (int)sizeof(size_t) || len < ((size_t)1 << (size * NB)), arg, STR_3->c_str());
            packint(&b, len, h.islittle, size, 0); // pack length
            luaL_addlstring(&b, s, len);
            totalsize += len;
            break;
        }
        case Kzstr:
        { // zero-terminated string
            size_t len;
            const char* s = luaL_checklstring(L, arg, &len);
            /*string contains zeros*/ scrypt_def(STR_4, "\x8d\x8c\x8e\x97\x92\x99\xe0\x9d\x91\x92\x8c\x9f\x97\x92\x8d\xe0\x86\x9b\x8e\x91\x8d");
            luaL_argcheck(L, strlen(s) == len, arg, STR_4->c_str());
            luaL_addlstring(&b, s, len);
            luaL_addchar(&b, '\0'); // add zero at the end
            totalsize += len + 1;
            break;
        }
        case Kpadding:
            luaL_addchar(&b, LUAL_PACKPADBYTE);
            LUAU_FALLTHROUGH;
        case Kpaddalign:
        case Knop:
            arg--; // undo increment
            break;
        }
    }
    luaL_pushresult(&b);
    return 1;
}

static int str_packsize(lua_State* L)
{
    Header h;
    const char* fmt = luaL_checkstring(L, 1); // format string
    int totalsize = 0;                        // accumulate total size of result
    initheader(L, &h);
    /*variable-length format*/ scrypt_def(STR_0, "\x8a\x9f\x8e\x97\x9f\x9e\x94\x9b\xd3\x94\x9b\x92\x99\x8c\x98\xe0\x9a\x91\x8e\x93\x9f\x8c");
    /*format result too large*/ scrypt_def(STR_1, "\x9a\x91\x8e\x93\x9f\x8c\xe0\x8e\x9b\x8d\x8b\x94\x8c\xe0\x8c\x91\x91\xe0\x94\x9f\x8e\x99\x9b");
    while (*fmt != '\0')
    {
        int size, ntoalign;
        KOption opt = getdetails(&h, totalsize, &fmt, &size, &ntoalign);
        luaL_argcheck(L, opt != Kstring && opt != Kzstr, 1, STR_0->c_str());
        size += ntoalign; // total space used by option
        luaL_argcheck(L, totalsize <= MAXSSIZE - size, 1, STR_1->c_str());
        totalsize += size;
    }
    lua_pushinteger(L, totalsize);
    return 1;
}

/*
** Unpack an integer with 'size' bytes and 'islittle' endianness.
** If size is smaller than the size of a Lua integer and integer
** is signed, must do sign extension (propagating the sign to the
** higher bits); if size is larger than the size of a Lua integer,
** it must check the unread bytes to see whether they do not cause an
** overflow.
*/
static long long unpackint(lua_State* L, const char* str, int islittle, int size, int issigned)
{
    unsigned long long res = 0;
    int i;
    int limit = (size <= SZINT) ? size : SZINT;
    for (i = limit - 1; i >= 0; i--)
    {
        res <<= NB;
        res |= (unsigned char)str[islittle ? i : size - 1 - i];
    }
    if (size < SZINT)
    { // real size smaller than int?
        if (issigned)
        { // needs sign extension?
            unsigned long long mask = (unsigned long long)1 << (size * NB - 1);
            res = ((res ^ mask) - mask); // do sign extension
        }
    }
    else if (size > SZINT)
    { // must check unread bytes
        int mask = (!issigned || (long long)res >= 0) ? 0 : MC;
        for (i = limit; i < size; i++)
        {
            if ((unsigned char)str[islittle ? i : size - 1 - i] != mask) {
                #define STR_0 /*%d-byte integer does not fit into Lua Integer*/ scrypt("\xdb\x9c\xd3\x9e\x87\x8c\x9b\xe0\x97\x92\x8c\x9b\x99\x9b\x8e\xe0\x9c\x91\x9b\x8d\xe0\x92\x91\x8c\xe0\x9a\x97\x8c\xe0\x97\x92\x8c\x91\xe0\xb4\x8b\x9f\xe0\xb7\x92\x8c\x9b\x99\x9b\x8e").c_str()
                luaL_error(L, STR_0, size);
                #undef STR_0
            }
        }
    }
    return (long long)res;
}

static int str_unpack(lua_State* L)
{
    Header h;
    const char* fmt = luaL_checkstring(L, 1);
    size_t ld;
    const char* data = luaL_checklstring(L, 2, &ld);
    int pos = posrelat(luaL_optinteger(L, 3, 1), ld) - 1;
    if (pos < 0)
        pos = 0;
    int n = 0; // number of results
    /*initial position out of string*/ scrypt_def(STR_0, "\x97\x92\x97\x8c\x97\x9f\x94\xe0\x90\x91\x8d\x97\x8c\x97\x91\x92\xe0\x91\x8b\x8c\xe0\x91\x9a\xe0\x8d\x8c\x8e\x97\x92\x99");
    luaL_argcheck(L, size_t(pos) <= ld, 3, STR_0->c_str());
    initheader(L, &h);
    /*data string too short*/ scrypt_def(STR_1, "\x9c\x9f\x8c\x9f\xe0\x8d\x8c\x8e\x97\x92\x99\xe0\x8c\x91\x91\xe0\x8d\x98\x91\x8e\x8c");
    /*too many results*/ scrypt_def(STR_2, "\x8c\x91\x91\xe0\x93\x9f\x92\x87\xe0\x8e\x9b\x8d\x8b\x94\x8c\x8d");
    /*unfinished string for format 'z'*/ scrypt_def(STR_3, "\x8b\x92\x9a\x97\x92\x97\x8d\x98\x9b\x9c\xe0\x8d\x8c\x8e\x97\x92\x99\xe0\x9a\x91\x8e\xe0\x9a\x91\x8e\x93\x9f\x8c\xe0\xd9\x86\xd9");
    while (*fmt != '\0')
    {
        int size, ntoalign;
        KOption opt = getdetails(&h, pos, &fmt, &size, &ntoalign);
        luaL_argcheck(L, (size_t)ntoalign + size <= ld - pos, 2, STR_1->c_str());
        pos += ntoalign; // skip alignment
        // stack space for item + next position
        luaL_checkstack(L, 2, STR_2->c_str());
        n++;
        switch (opt)
        {
        case Kint:
        {
            long long res = unpackint(L, data + pos, h.islittle, size, true);
            lua_pushnumber(L, (double)res);
            break;
        }
        case Kuint:
        {
            unsigned long long res = unpackint(L, data + pos, h.islittle, size, false);
            lua_pushnumber(L, (double)res);
            break;
        }
        case Kfloat:
        {
            volatile Ftypes u;
            double num;
            copywithendian(u.buff, data + pos, size, h.islittle);
            if (size == sizeof(u.f))
                num = (double)u.f;
            else if (size == sizeof(u.d))
                num = (double)u.d;
            else
                num = u.n;
            lua_pushnumber(L, num);
            break;
        }
        case Kchar:
        {
            lua_pushlstring(L, data + pos, size);
            break;
        }
        case Kstring:
        {
            size_t len = (size_t)unpackint(L, data + pos, h.islittle, size, 0);
            luaL_argcheck(L, len <= ld - pos - size, 2, STR_1->c_str());
            lua_pushlstring(L, data + pos + size, len);
            pos += (int)len; // skip string
            break;
        }
        case Kzstr:
        {
            size_t len = strlen(data + pos);
            luaL_argcheck(L, pos + len < ld, 2, STR_3->c_str());
            lua_pushlstring(L, data + pos, len);
            pos += (int)len + 1; // skip string plus final '\0'
            break;
        }
        case Kpaddalign:
        case Kpadding:
        case Knop:
            n--; // undo increment
            break;
        }
        pos += size;
    }
    lua_pushinteger(L, pos + 1); // next position
    return n + 1;
}

// }======================================================

static void createmetatable(lua_State* L)
{
    /*__index*/ scrypt_def(STR_0, "\xa1\xa1\x97\x92\x9c\x9b\x88");
    lua_createtable(L, 0, 1); // create metatable for strings
    lua_pushliteral(L, "");   // dummy string
    lua_pushvalue(L, -2);
    lua_setmetatable(L, -2);        // set string metatable
    lua_pop(L, 1);                  // pop dummy string
    lua_pushvalue(L, -2);           // string library...
    lua_setfield(L, -2, STR_0->c_str()); // ...is the __index metamethod
    lua_pop(L, 1);                  // pop metatable
}

/*
** Open string library
*/
int luaopen_string(lua_State* L)
{
    std::string STR_0 = /*byte*/ scrypt("\x9e\x87\x8c\x9b");
    std::string STR_1 = /*char*/ scrypt("\x9d\x98\x9f\x8e");
    std::string STR_2 = /*find*/ scrypt("\x9a\x97\x92\x9c");
    std::string STR_3 = /*format*/ scrypt("\x9a\x91\x8e\x93\x9f\x8c");
    std::string STR_4 = /*gmatch*/ scrypt("\x99\x93\x9f\x8c\x9d\x98");
    std::string STR_5 = /*gsub*/ scrypt("\x99\x8d\x8b\x9e");
    std::string STR_6 = /*len*/ scrypt("\x94\x9b\x92");
    std::string STR_7 = /*lower*/ scrypt("\x94\x91\x89\x9b\x8e");
    std::string STR_8 = /*match*/ scrypt("\x93\x9f\x8c\x9d\x98");
    std::string STR_9 = /*rep*/ scrypt("\x8e\x9b\x90");
    std::string STR_10 = /*reverse*/ scrypt("\x8e\x9b\x8a\x9b\x8e\x8d\x9b");
    std::string STR_11 = /*sub*/ scrypt("\x8d\x8b\x9e");
    std::string STR_12 = /*upper*/ scrypt("\x8b\x90\x90\x9b\x8e");
    std::string STR_13 = /*split*/ scrypt("\x8d\x90\x94\x97\x8c");
    std::string STR_14 = /*pack*/ scrypt("\x90\x9f\x9d\x95");
    std::string STR_15 = /*packsize*/ scrypt("\x90\x9f\x9d\x95\x8d\x97\x86\x9b");
    std::string STR_16 = /*unpack*/ scrypt("\x8b\x92\x90\x9f\x9d\x95");
    std::string STR_17 = /*string*/ scrypt("\x8d\x8c\x8e\x97\x92\x99");

    static const luaL_Reg strlib[] = {
        {STR_0.c_str(), str_byte},
        {STR_1.c_str(), str_char},
        {STR_2.c_str(), str_find},
        {STR_3.c_str(), str_format},
        {STR_4.c_str(), gmatch},
        {STR_5.c_str(), str_gsub},
        {STR_6.c_str(), str_len},
        {STR_7.c_str(), str_lower},
        {STR_8.c_str(), str_match},
        {STR_9.c_str(), str_rep},
        {STR_10.c_str(), str_reverse},
        {STR_11.c_str(), str_sub},
        {STR_12.c_str(), str_upper},
        {STR_13.c_str(), str_split},
        {STR_14.c_str(), str_pack},
        {STR_15.c_str(), str_packsize},
        {STR_16.c_str(), str_unpack},
        {NULL, NULL},
    };

    luaL_register(L, STR_17.c_str(), strlib);
    createmetatable(L);

    return 1;
}
