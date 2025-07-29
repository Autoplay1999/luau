// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "lualib.h"

#include "lcommon.h"
#include "lnumutils.h"

#include <math.h>

static int vector_create(lua_State* L)
{
    // checking argument count to avoid accepting 'nil' as a valid value
    int count = lua_gettop(L);

    double x = luaL_checknumber(L, 1);
    double y = luaL_checknumber(L, 2);
    double z = count >= 3 ? luaL_checknumber(L, 3) : 0.0;

#if LUA_VECTOR_SIZE == 4
    double w = count >= 4 ? luaL_checknumber(L, 4) : 0.0;

    lua_pushvector(L, float(x), float(y), float(z), float(w));
#else
    lua_pushvector(L, float(x), float(y), float(z));
#endif

    return 1;
}

static int vector_magnitude(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    lua_pushnumber(L, sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]));
#else
    lua_pushnumber(L, sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
#endif

    return 1;
}

static int vector_normalize(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    float invSqrt = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]);

    lua_pushvector(L, v[0] * invSqrt, v[1] * invSqrt, v[2] * invSqrt, v[3] * invSqrt);
#else
    float invSqrt = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    lua_pushvector(L, v[0] * invSqrt, v[1] * invSqrt, v[2] * invSqrt);
#endif

    return 1;
}

static int vector_cross(lua_State* L)
{
    const float* a = luaL_checkvector(L, 1);
    const float* b = luaL_checkvector(L, 2);

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0], 0.0f);
#else
    lua_pushvector(L, a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
#endif

    return 1;
}

static int vector_dot(lua_State* L)
{
    const float* a = luaL_checkvector(L, 1);
    const float* b = luaL_checkvector(L, 2);

#if LUA_VECTOR_SIZE == 4
    lua_pushnumber(L, a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]);
#else
    lua_pushnumber(L, a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
#endif

    return 1;
}

static int vector_angle(lua_State* L)
{
    const float* a = luaL_checkvector(L, 1);
    const float* b = luaL_checkvector(L, 2);
    const float* axis = luaL_optvector(L, 3, nullptr);

    // cross(a, b)
    float cross[] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};

    double sinA = sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
    double cosA = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    double angle = atan2(sinA, cosA);

    if (axis)
    {
        if (cross[0] * axis[0] + cross[1] * axis[1] + cross[2] * axis[2] < 0.0f)
            angle = -angle;
    }

    lua_pushnumber(L, angle);
    return 1;
}

static int vector_floor(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, floorf(v[0]), floorf(v[1]), floorf(v[2]), floorf(v[3]));
#else
    lua_pushvector(L, floorf(v[0]), floorf(v[1]), floorf(v[2]));
#endif

    return 1;
}

static int vector_ceil(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, ceilf(v[0]), ceilf(v[1]), ceilf(v[2]), ceilf(v[3]));
#else
    lua_pushvector(L, ceilf(v[0]), ceilf(v[1]), ceilf(v[2]));
#endif

    return 1;
}

static int vector_abs(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, fabsf(v[0]), fabsf(v[1]), fabsf(v[2]), fabsf(v[3]));
#else
    lua_pushvector(L, fabsf(v[0]), fabsf(v[1]), fabsf(v[2]));
#endif

    return 1;
}

static int vector_sign(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, luaui_signf(v[0]), luaui_signf(v[1]), luaui_signf(v[2]), luaui_signf(v[3]));
#else
    lua_pushvector(L, luaui_signf(v[0]), luaui_signf(v[1]), luaui_signf(v[2]));
#endif

    return 1;
}

static int vector_clamp(lua_State* L)
{
    /*max.x must be greater than or equal to min.x*/ scrypt_def(STR_0, "\x93\x9f\x88\xd2\x88\xe0\x93\x8b\x8d\x8c\xe0\x9e\x9b\xe0\x99\x8e\x9b\x9f\x8c\x9b\x8e\xe0\x8c\x98\x9f\x92\xe0\x91\x8e\xe0\x9b\x8f\x8b\x9f\x94\xe0\x8c\x91\xe0\x93\x97\x92\xd2\x88");
    /*max.y must be greater than or equal to min.y*/ scrypt_def(STR_1, "\x93\x9f\x88\xd2\x87\xe0\x93\x8b\x8d\x8c\xe0\x9e\x9b\xe0\x99\x8e\x9b\x9f\x8c\x9b\x8e\xe0\x8c\x98\x9f\x92\xe0\x91\x8e\xe0\x9b\x8f\x8b\x9f\x94\xe0\x8c\x91\xe0\x93\x97\x92\xd2\x87");
    /*max.z must be greater than or equal to min.z*/ scrypt_def(STR_2, "\x93\x9f\x88\xd2\x86\xe0\x93\x8b\x8d\x8c\xe0\x9e\x9b\xe0\x99\x8e\x9b\x9f\x8c\x9b\x8e\xe0\x8c\x98\x9f\x92\xe0\x91\x8e\xe0\x9b\x8f\x8b\x9f\x94\xe0\x8c\x91\xe0\x93\x97\x92\xd2\x86");

    const float* v = luaL_checkvector(L, 1);
    const float* min = luaL_checkvector(L, 2);
    const float* max = luaL_checkvector(L, 3);

    luaL_argcheck(L, min[0] <= max[0], 3, STR_0->c_str());
    luaL_argcheck(L, min[1] <= max[1], 3, STR_1->c_str());
    luaL_argcheck(L, min[2] <= max[2], 3, STR_2->c_str());

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(
        L,
        luaui_clampf(v[0], min[0], max[0]),
        luaui_clampf(v[1], min[1], max[1]),
        luaui_clampf(v[2], min[2], max[2]),
        luaui_clampf(v[3], min[3], max[3])
    );
#else
    lua_pushvector(L, luaui_clampf(v[0], min[0], max[0]), luaui_clampf(v[1], min[1], max[1]), luaui_clampf(v[2], min[2], max[2]));
#endif

    return 1;
}

static int vector_min(lua_State* L)
{
    int n = lua_gettop(L);
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    float result[] = {v[0], v[1], v[2], v[3]};
#else
    float result[] = {v[0], v[1], v[2]};
#endif

    for (int i = 2; i <= n; i++)
    {
        const float* b = luaL_checkvector(L, i);

        if (b[0] < result[0])
            result[0] = b[0];
        if (b[1] < result[1])
            result[1] = b[1];
        if (b[2] < result[2])
            result[2] = b[2];
#if LUA_VECTOR_SIZE == 4
        if (b[3] < result[3])
            result[3] = b[3];
#endif
    }

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, result[0], result[1], result[2], result[3]);
#else
    lua_pushvector(L, result[0], result[1], result[2]);
#endif

    return 1;
}

static int vector_max(lua_State* L)
{
    int n = lua_gettop(L);
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    float result[] = {v[0], v[1], v[2], v[3]};
#else
    float result[] = {v[0], v[1], v[2]};
#endif

    for (int i = 2; i <= n; i++)
    {
        const float* b = luaL_checkvector(L, i);

        if (b[0] > result[0])
            result[0] = b[0];
        if (b[1] > result[1])
            result[1] = b[1];
        if (b[2] > result[2])
            result[2] = b[2];
#if LUA_VECTOR_SIZE == 4
        if (b[3] > result[3])
            result[3] = b[3];
#endif
    }

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, result[0], result[1], result[2], result[3]);
#else
    lua_pushvector(L, result[0], result[1], result[2]);
#endif

    return 1;
}

static int vector_index(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);
    size_t namelen = 0;
    const char* name = luaL_checklstring(L, 2, &namelen);

    // field access implementation mirrors the fast-path we have in the VM
    if (namelen == 1)
    {
        int ic = (name[0] | ' ') - 'x';

#if LUA_VECTOR_SIZE == 4
        // 'w' is before 'x' in ascii, so ic is -1 when indexing with 'w'
        if (ic == -1)
            ic = 3;
#endif

        if (unsigned(ic) < LUA_VECTOR_SIZE)
        {
            lua_pushnumber(L, v[ic]);
            return 1;
        }
    }

    #define STR_0 /*attempt to index vector with '%s'*/ scrypt("\x9f\x8c\x8c\x9b\x93\x90\x8c\xe0\x8c\x91\xe0\x97\x92\x9c\x9b\x88\xe0\x8a\x9b\x9d\x8c\x91\x8e\xe0\x89\x97\x8c\x98\xe0\xd9\xdb\x8d\xd9").c_str() 
    luaL_error(L, STR_0, name);
    #undef STR_0
}

static void createmetatable(lua_State* L)
{
    /*__index*/ scrypt_def(STR_0, "\xa1\xa1\x97\x92\x9c\x9b\x88");
    lua_createtable(L, 0, 1); // create metatable for vectors

    // push dummy vector
#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, 0.0f, 0.0f, 0.0f, 0.0f);
#else
    lua_pushvector(L, 0.0f, 0.0f, 0.0f);
#endif

    lua_pushvalue(L, -2);
    lua_setmetatable(L, -2); // set vector metatable
    lua_pop(L, 1);           // pop dummy vector

    lua_pushcfunction(L, vector_index, nullptr);
    lua_setfield(L, -2, STR_0->c_str());

    lua_setreadonly(L, -1, true);
    lua_pop(L, 1); // pop the metatable
}

int luaopen_vector(lua_State* L)
{
    std::string STR_0 = /*create*/ scrypt("\x9d\x8e\x9b\x9f\x8c\x9b");
    std::string STR_1 = /*magnitude*/ scrypt("\x93\x9f\x99\x92\x97\x8c\x8b\x9c\x9b");
    std::string STR_2 = /*normalize*/ scrypt("\x92\x91\x8e\x93\x9f\x94\x97\x86\x9b");
    std::string STR_3 = /*cross*/ scrypt("\x9d\x8e\x91\x8d\x8d");
    std::string STR_4 = /*dot*/ scrypt("\x9c\x91\x8c");
    std::string STR_5 = /*angle*/ scrypt("\x9f\x92\x99\x94\x9b");
    std::string STR_6 = /*floor*/ scrypt("\x9a\x94\x91\x91\x8e");
    std::string STR_7 = /*ceil*/ scrypt("\x9d\x9b\x97\x94");
    std::string STR_8 = /*abs*/ scrypt("\x9f\x9e\x8d");
    std::string STR_9 = /*sign*/ scrypt("\x8d\x97\x99\x92");
    std::string STR_10 = /*clamp*/ scrypt("\x9d\x94\x9f\x93\x90");
    std::string STR_11 = /*max*/ scrypt("\x93\x9f\x88");
    std::string STR_12 = /*min*/ scrypt("\x93\x97\x92");
    std::string STR_13 = /*zero*/ scrypt("\x86\x9b\x8e\x91");
    std::string STR_14 = /*one*/ scrypt("\x91\x92\x9b");

    luaL_Reg vectorlib[] = {
        {STR_0.c_str(), vector_create},
        {STR_1.c_str(), vector_magnitude},
        {STR_2.c_str(), vector_normalize},
        {STR_3.c_str(), vector_cross},
        {STR_4.c_str(), vector_dot},
        {STR_5.c_str(), vector_angle},
        {STR_6.c_str(), vector_floor},
        {STR_7.c_str(), vector_ceil},
        {STR_8.c_str(), vector_abs},
        {STR_9.c_str(), vector_sign},
        {STR_10.c_str(), vector_clamp},
        {STR_11.c_str(), vector_max},
        {STR_12.c_str(), vector_min},
        {NULL, NULL},
    };

    luaL_register(L, LUA_VECLIBNAME, vectorlib);

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, 0.0f, 0.0f, 0.0f, 0.0f);
    lua_setfield(L, -2, STR_13.c_str());
    lua_pushvector(L, 1.0f, 1.0f, 1.0f, 1.0f);
    lua_setfield(L, -2, STR_14.c_str());
#else
    lua_pushvector(L, 0.0f, 0.0f, 0.0f);
    lua_setfield(L, -2, STR_13.c_str());
    lua_pushvector(L, 1.0f, 1.0f, 1.0f);
    lua_setfield(L, -2, STR_14.c_str());
#endif

    createmetatable(L);

    return 1;
}
