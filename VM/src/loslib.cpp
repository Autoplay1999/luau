// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lualib.h"

#include "lcommon.h"

#include <string.h>
#include <time.h>

#define LUA_STRFTIMEOPTIONS "aAbBcdHIjmMpSUwWxXyYzZ%"

#if defined(_WIN32)
static tm* gmtime_r(const time_t* timep, tm* result)
{
    return gmtime_s(result, timep) == 0 ? result : NULL;
}

static tm* localtime_r(const time_t* timep, tm* result)
{
    return localtime_s(result, timep) == 0 ? result : NULL;
}
#endif

static time_t os_timegm(struct tm* timep)
{
    // Julian day number calculation
    int day = timep->tm_mday;
    int month = timep->tm_mon + 1;
    int year = timep->tm_year + 1900;

    // year adjustment, pretend that it starts in March
    int a = timep->tm_mon % 12 < 2 ? 1 : 0;

    // also adjust for out-of-range month numbers in input
    a -= timep->tm_mon / 12;

    int y = year + 4800 - a;
    int m = month + (12 * a) - 3;

    int julianday = day + ((153 * m + 2) / 5) + (365 * y) + (y / 4) - (y / 100) + (y / 400) - 32045;

    const int utcstartasjulianday = 2440588;                              // Jan 1st 1970 offset in Julian calendar
    const int64_t utcstartasjuliansecond = utcstartasjulianday * 86400ll; // same in seconds

    // fail the dates before UTC start
    if (julianday < utcstartasjulianday)
        return time_t(-1);

    int64_t daysecond = timep->tm_hour * 3600ll + timep->tm_min * 60ll + timep->tm_sec;
    int64_t julianseconds = int64_t(julianday) * 86400ull + daysecond;

    if (julianseconds < utcstartasjuliansecond)
        return time_t(-1);

    int64_t utc = julianseconds - utcstartasjuliansecond;
    return time_t(utc);
}

static int os_clock(lua_State* L)
{
    lua_pushnumber(L, lua_clock());
    return 1;
}

/*
** {======================================================
** Time/Date operations
** { year=%Y, month=%m, day=%d, hour=%H, min=%M, sec=%S,
**   wday=%w+1, yday=%j, isdst=? }
** =======================================================
*/

static void setfield(lua_State* L, const char* key, int value)
{
    lua_pushinteger(L, value);
    lua_setfield(L, -2, key);
}

static void setboolfield(lua_State* L, const char* key, int value)
{
    if (value < 0) // undefined?
        return;    // does not set field
    lua_pushboolean(L, value);
    lua_setfield(L, -2, key);
}

static int getboolfield(lua_State* L, const char* key)
{
    int res;
    lua_rawgetfield(L, -1, key);
    res = lua_isnil(L, -1) ? -1 : lua_toboolean(L, -1);
    lua_pop(L, 1);
    return res;
}

static int getfield(lua_State* L, const char* key, int d)
{
    int res;
    lua_rawgetfield(L, -1, key);
    if (lua_isnumber(L, -1))
        res = (int)lua_tointeger(L, -1);
    else
    {
        if (d < 0) {
            /*field '%s' missing in date table*/ scrypt_def(STR_0, "\x9a\x97\x9b\x94\x9c\xe0\xd9\xdb\x8d\xd9\xe0\x93\x97\x8d\x8d\x97\x92\x99\xe0\x97\x92\xe0\x9c\x9f\x8c\x9b\xe0\x8c\x9f\x9e\x94\x9b");
            luaL_error(L, STR_0->c_str(), key);
        }
        res = d;
    }
    lua_pop(L, 1);
    return res;
}

static int os_date(lua_State* L)
{
    /*%c*/ scrypt_def(STR_0, "\xdb\x9d");
    const char* s = luaL_optstring(L, 1, STR_0->c_str());
    time_t t = luaL_opt(L, (time_t)luaL_checknumber, 2, time(NULL));

    struct tm tm;
    struct tm* stm;
    if (*s == '!')
    { // UTC?
        stm = gmtime_r(&t, &tm);
        s++; // skip `!'
    }
    else
    {
        // on Windows, localtime() fails with dates before epoch start so we disallow that
        stm = t < 0 ? NULL : localtime_r(&t, &tm);
    }

    /**t*/ scrypt_def(STR_1, "\xd6\x8c");

    if (stm == NULL) // invalid date?
    {
        lua_pushnil(L);
    }
    else if (strcmp(s, STR_1->c_str()) == 0)
    {
        /*sec*/ scrypt_def(STR_2, "\x8d\x9b\x9d");
        /*min*/ scrypt_def(STR_3, "\x93\x97\x92");
        /*hour*/ scrypt_def(STR_4, "\x98\x91\x8b\x8e");
        /*day*/ scrypt_def(STR_5, "\x9c\x9f\x87");
        /*month*/ scrypt_def(STR_6, "\x93\x91\x92\x8c\x98");
        /*year*/ scrypt_def(STR_7, "\x87\x9b\x9f\x8e");
        /*wday*/ scrypt_def(STR_8, "\x89\x9c\x9f\x87");
        /*yday*/ scrypt_def(STR_9, "\x87\x9c\x9f\x87");
        /*isdst*/ scrypt_def(STR_10, "\x97\x8d\x9c\x8d\x8c");
        lua_createtable(L, 0, 9); // 9 = number of fields
        setfield(L, STR_2->c_str(), stm->tm_sec);
        setfield(L, STR_3->c_str(), stm->tm_min);
        setfield(L, STR_4->c_str(), stm->tm_hour);
        setfield(L, STR_5->c_str(), stm->tm_mday);
        setfield(L, STR_6->c_str(), stm->tm_mon + 1);
        setfield(L, STR_7->c_str(), stm->tm_year + 1900);
        setfield(L, STR_8->c_str(), stm->tm_wday + 1);
        setfield(L, STR_9->c_str(), stm->tm_yday + 1);
        setboolfield(L, STR_10->c_str(), stm->tm_isdst);
    }
    else
    {
        /*aAbBcdHIjmMpSUwWxXyYzZ%*/ scrypt_def(STR_11, "\x9f\xbf\x9e\xbe\x9d\x9c\xb8\xb7\x96\x93\xb3\x90\xad\xab\x89\xa9\x88\xa8\x87\xa7\x86\xa6\xdb");

        char cc[3];
        cc[0] = '%';
        cc[2] = '\0';

        luaL_Strbuf b;
        luaL_buffinit(L, &b);
        for (; *s; s++)
        {
            if (*s != '%' || *(s + 1) == '\0') // no conversion specifier?
            {
                luaL_addchar(&b, *s);
            }
            else if (strchr(STR_11->c_str(), *(s + 1)) == 0)
            {
                #define STR_12 /*invalid conversion specifier*/ scrypt("\x97\x92\x8a\x9f\x94\x97\x9c\xe0\x9d\x91\x92\x8a\x9b\x8e\x8d\x97\x91\x92\xe0\x8d\x90\x9b\x9d\x97\x9a\x97\x9b\x8e").c_str()
                luaL_argerror(L, 1, STR_12);
                #undef STR_12
            }
            else
            {
                size_t reslen;
                char buff[200]; // should be big enough for any conversion result
                cc[1] = *(++s);
                reslen = strftime(buff, sizeof(buff), cc, stm);
                luaL_addlstring(&b, buff, reslen);
            }
        }
        luaL_pushresult(&b);
    }
    return 1;
}

static int os_time(lua_State* L)
{
    time_t t;
    if (lua_isnoneornil(L, 1)) // called without args?
        t = time(NULL);        // get current time
    else
    {
        /*sec*/ scrypt_def(STR_0, "\x8d\x9b\x9d");
        /*min*/ scrypt_def(STR_1, "\x93\x97\x92");
        /*hour*/ scrypt_def(STR_2, "\x98\x91\x8b\x8e");
        /*day*/ scrypt_def(STR_3, "\x9c\x9f\x87");
        /*month*/ scrypt_def(STR_4, "\x93\x91\x92\x8c\x98");
        /*year*/ scrypt_def(STR_5, "\x87\x9b\x9f\x8e");
        /*isdst*/ scrypt_def(STR_6, "\x97\x8d\x9c\x8d\x8c");
        struct tm ts;
        luaL_checktype(L, 1, LUA_TTABLE);
        lua_settop(L, 1); // make sure table is at the top
        ts.tm_sec = getfield(L, STR_0->c_str(), 0);
        ts.tm_min = getfield(L, STR_1->c_str(), 0);
        ts.tm_hour = getfield(L, STR_2->c_str(), 12);
        ts.tm_mday = getfield(L, STR_3->c_str(), -1);
        ts.tm_mon = getfield(L, STR_4->c_str(), -1) - 1;
        ts.tm_year = getfield(L, STR_5->c_str(), -1) - 1900;
        ts.tm_isdst = getboolfield(L, STR_6->c_str());

        // Note: upstream Lua uses mktime() here which assumes input is local time, but we prefer UTC for consistency
        t = os_timegm(&ts);
    }
    if (t == (time_t)(-1))
        lua_pushnil(L);
    else
        lua_pushnumber(L, (double)t);
    return 1;
}

static int os_difftime(lua_State* L)
{
    lua_pushnumber(L, difftime((time_t)(luaL_checknumber(L, 1)), (time_t)(luaL_optnumber(L, 2, 0))));
    return 1;
}

int luaopen_os(lua_State* L)
{
    std::string STR_0 = /*clock*/ scrypt("\x9d\x94\x91\x9d\x95");
    std::string STR_1 = /*date*/ scrypt("\x9c\x9f\x8c\x9b");
    std::string STR_2 = /*difftime*/ scrypt("\x9c\x97\x9a\x9a\x8c\x97\x93\x9b");
    std::string STR_3 = /*time*/ scrypt("\x8c\x97\x93\x9b");
    std::string STR_4 = /*os*/ scrypt("\x91\x8d");

    const luaL_Reg syslib[] = {
        {STR_0.c_str(), os_clock},
        {STR_1.c_str(), os_date},
        {STR_2.c_str(), os_difftime},
        {STR_3.c_str(), os_time},
        {NULL, NULL},
    };
    
    luaL_register(L, STR_4.c_str(), syslib);
    return 1;
}
