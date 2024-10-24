// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#include "lualib.h"

#include "lstate.h"

#include <math.h>
#include <time.h>

LUAU_FASTFLAGVARIABLE(LuauMathMap)

#undef PI
#define PI (3.14159265358979323846)
#define RADIANS_PER_DEGREE (PI / 180.0)

#define PCG32_INC 105

static uint32_t pcg32_random(uint64_t* state)
{
    uint64_t oldstate = *state;
    *state = oldstate * 6364136223846793005ULL + (PCG32_INC | 1);
    uint32_t xorshifted = uint32_t(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = uint32_t(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-int32_t(rot)) & 31));
}

static void pcg32_seed(uint64_t* state, uint64_t seed)
{
    *state = 0;
    pcg32_random(state);
    *state += seed;
    pcg32_random(state);
}

static int math_abs(lua_State* L)
{
    lua_pushnumber(L, fabs(luaL_checknumber(L, 1)));
    return 1;
}

static int math_sin(lua_State* L)
{
    lua_pushnumber(L, sin(luaL_checknumber(L, 1)));
    return 1;
}

static int math_sinh(lua_State* L)
{
    lua_pushnumber(L, sinh(luaL_checknumber(L, 1)));
    return 1;
}

static int math_cos(lua_State* L)
{
    lua_pushnumber(L, cos(luaL_checknumber(L, 1)));
    return 1;
}

static int math_cosh(lua_State* L)
{
    lua_pushnumber(L, cosh(luaL_checknumber(L, 1)));
    return 1;
}

static int math_tan(lua_State* L)
{
    lua_pushnumber(L, tan(luaL_checknumber(L, 1)));
    return 1;
}

static int math_tanh(lua_State* L)
{
    lua_pushnumber(L, tanh(luaL_checknumber(L, 1)));
    return 1;
}

static int math_asin(lua_State* L)
{
    lua_pushnumber(L, asin(luaL_checknumber(L, 1)));
    return 1;
}

static int math_acos(lua_State* L)
{
    lua_pushnumber(L, acos(luaL_checknumber(L, 1)));
    return 1;
}

static int math_atan(lua_State* L)
{
    lua_pushnumber(L, atan(luaL_checknumber(L, 1)));
    return 1;
}

static int math_atan2(lua_State* L)
{
    lua_pushnumber(L, atan2(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int math_ceil(lua_State* L)
{
    lua_pushnumber(L, ceil(luaL_checknumber(L, 1)));
    return 1;
}

static int math_floor(lua_State* L)
{
    lua_pushnumber(L, floor(luaL_checknumber(L, 1)));
    return 1;
}

static int math_fmod(lua_State* L)
{
    lua_pushnumber(L, fmod(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int math_modf(lua_State* L)
{
    double ip;
    double fp = modf(luaL_checknumber(L, 1), &ip);
    lua_pushnumber(L, ip);
    lua_pushnumber(L, fp);
    return 2;
}

static int math_sqrt(lua_State* L)
{
    lua_pushnumber(L, sqrt(luaL_checknumber(L, 1)));
    return 1;
}

static int math_pow(lua_State* L)
{
    lua_pushnumber(L, pow(luaL_checknumber(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int math_log(lua_State* L)
{
    double x = luaL_checknumber(L, 1);
    double res;
    if (lua_isnoneornil(L, 2))
        res = log(x);
    else
    {
        double base = luaL_checknumber(L, 2);
        if (base == 2.0)
            res = log2(x);
        else if (base == 10.0)
            res = log10(x);
        else
            res = log(x) / log(base);
    }
    lua_pushnumber(L, res);
    return 1;
}

static int math_log10(lua_State* L)
{
    lua_pushnumber(L, log10(luaL_checknumber(L, 1)));
    return 1;
}

static int math_exp(lua_State* L)
{
    lua_pushnumber(L, exp(luaL_checknumber(L, 1)));
    return 1;
}

static int math_deg(lua_State* L)
{
    lua_pushnumber(L, luaL_checknumber(L, 1) / RADIANS_PER_DEGREE);
    return 1;
}

static int math_rad(lua_State* L)
{
    lua_pushnumber(L, luaL_checknumber(L, 1) * RADIANS_PER_DEGREE);
    return 1;
}

static int math_frexp(lua_State* L)
{
    int e;
    lua_pushnumber(L, frexp(luaL_checknumber(L, 1), &e));
    lua_pushinteger(L, e);
    return 2;
}

static int math_ldexp(lua_State* L)
{
    lua_pushnumber(L, ldexp(luaL_checknumber(L, 1), luaL_checkinteger(L, 2)));
    return 1;
}

static int math_min(lua_State* L)
{
    int n = lua_gettop(L); // number of arguments
    double dmin = luaL_checknumber(L, 1);
    int i;
    for (i = 2; i <= n; i++)
    {
        double d = luaL_checknumber(L, i);
        if (d < dmin)
            dmin = d;
    }
    lua_pushnumber(L, dmin);
    return 1;
}

static int math_max(lua_State* L)
{
    int n = lua_gettop(L); // number of arguments
    double dmax = luaL_checknumber(L, 1);
    int i;
    for (i = 2; i <= n; i++)
    {
        double d = luaL_checknumber(L, i);
        if (d > dmax)
            dmax = d;
    }
    lua_pushnumber(L, dmax);
    return 1;
}

static int math_random(lua_State* L)
{
    /*interval is empty*/ scrypt_def(STR_0, "\x97\x92\x8c\x9b\x8e\x8a\x9f\x94\xe0\x97\x8d\xe0\x9b\x93\x90\x8c\x87");
    /*interval is too large*/ scrypt_def(STR_1, "\x97\x92\x8c\x9b\x8e\x8a\x9f\x94\xe0\x97\x8d\xe0\x8c\x91\x91\xe0\x94\x9f\x8e\x99\x9b");
    /*wrong number of arguments*/ scrypt_def(STR_2, "\x89\x8e\x91\x92\x99\xe0\x92\x8b\x93\x9e\x9b\x8e\xe0\x91\x9a\xe0\x9f\x8e\x99\x8b\x93\x9b\x92\x8c\x8d");

    global_State* g = L->global;
    switch (lua_gettop(L))
    { // check number of arguments
    case 0:
    { // no arguments
        // Using ldexp instead of division for speed & clarity.
        // See http://mumble.net/~campbell/tmp/random_real.c for details on generating doubles from integer ranges.
        uint32_t rl = pcg32_random(&g->rngstate);
        uint32_t rh = pcg32_random(&g->rngstate);
        double rd = ldexp(double(rl | (uint64_t(rh) << 32)), -64);
        lua_pushnumber(L, rd); // number between 0 and 1
        break;
    }
    case 1:
    { // only upper limit
        int u = luaL_checkinteger(L, 1);
        luaL_argcheck(L, 1 <= u, 1, STR_0->c_str());

        uint64_t x = uint64_t(u) * pcg32_random(&g->rngstate);
        int r = int(1 + (x >> 32));
        lua_pushinteger(L, r); // int between 1 and `u'
        break;
    }
    case 2:
    { // lower and upper limits
        int l = luaL_checkinteger(L, 1);
        int u = luaL_checkinteger(L, 2);
        luaL_argcheck(L, l <= u, 2, STR_0->c_str());

        uint32_t ul = uint32_t(u) - uint32_t(l);
        luaL_argcheck(L, ul < UINT_MAX, 2, STR_1->c_str()); // -INT_MIN..INT_MAX interval can result in integer overflow
        uint64_t x = uint64_t(ul + 1) * pcg32_random(&g->rngstate);
        int r = int(l + (x >> 32));
        lua_pushinteger(L, r); // int between `l' and `u'
        break;
    }
    default:
        luaL_error(L, STR_2->c_str());
    }
    return 1;
}

static int math_randomseed(lua_State* L)
{
    int seed = luaL_checkinteger(L, 1);

    pcg32_seed(&L->global->rngstate, seed);
    return 0;
}

static const unsigned char kPerlinHash[257] = {
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,
    190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,
    125, 136, 171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,
    41,  55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130,
    116, 188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126, 255, 82,  85,  212, 207,
    206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,
    172, 9,   129, 22,  39,  253, 19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,  242, 193, 238, 210, 144,
    12,  191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157, 184, 84,  204, 176, 115, 121, 50,  45,
    127, 4,   150, 254, 138, 236, 205, 93,  222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180, 151
};

const float kPerlinGrad[16][3] = {
    {1, 1, 0},
    {-1, 1, 0},
    {1, -1, 0},
    {-1, -1, 0},
    {1, 0, 1},
    {-1, 0, 1},
    {1, 0, -1},
    {-1, 0, -1},
    {0, 1, 1},
    {0, -1, 1},
    {0, 1, -1},
    {0, -1, -1},
    {1, 1, 0},
    {0, -1, 1},
    {-1, 1, 0},
    {0, -1, -1}
};

inline float perlin_fade(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

inline float perlin_lerp(float t, float a, float b)
{
    return a + t * (b - a);
}

inline float perlin_grad(int hash, float x, float y, float z)
{
    const float* g = kPerlinGrad[hash & 15];
    return g[0] * x + g[1] * y + g[2] * z;
}

static float perlin(float x, float y, float z)
{
    float xflr = floorf(x);
    float yflr = floorf(y);
    float zflr = floorf(z);

    int xi = int(xflr) & 255;
    int yi = int(yflr) & 255;
    int zi = int(zflr) & 255;

    float xf = x - xflr;
    float yf = y - yflr;
    float zf = z - zflr;

    float u = perlin_fade(xf);
    float v = perlin_fade(yf);
    float w = perlin_fade(zf);

    const unsigned char* p = kPerlinHash;

    int a = (p[xi] + yi) & 255;
    int aa = (p[a] + zi) & 255;
    int ab = (p[a + 1] + zi) & 255;

    int b = (p[xi + 1] + yi) & 255;
    int ba = (p[b] + zi) & 255;
    int bb = (p[b + 1] + zi) & 255;

    float la = perlin_lerp(u, perlin_grad(p[aa], xf, yf, zf), perlin_grad(p[ba], xf - 1, yf, zf));
    float lb = perlin_lerp(u, perlin_grad(p[ab], xf, yf - 1, zf), perlin_grad(p[bb], xf - 1, yf - 1, zf));
    float la1 = perlin_lerp(u, perlin_grad(p[aa + 1], xf, yf, zf - 1), perlin_grad(p[ba + 1], xf - 1, yf, zf - 1));
    float lb1 = perlin_lerp(u, perlin_grad(p[ab + 1], xf, yf - 1, zf - 1), perlin_grad(p[bb + 1], xf - 1, yf - 1, zf - 1));

    return perlin_lerp(w, perlin_lerp(v, la, lb), perlin_lerp(v, la1, lb1));
}

static int math_noise(lua_State* L)
{
    /*number*/ scrypt_def(STR_0, "\x92\x8b\x93\x9e\x9b\x8e");

    int nx, ny, nz;
    double x = lua_tonumberx(L, 1, &nx);
    double y = lua_tonumberx(L, 2, &ny);
    double z = lua_tonumberx(L, 3, &nz);

    luaL_argexpected(L, nx, 1, STR_0->c_str());
    luaL_argexpected(L, ny || lua_isnoneornil(L, 2), 2, STR_0->c_str());
    luaL_argexpected(L, nz || lua_isnoneornil(L, 3), 3, STR_0->c_str());

    double r = perlin((float)x, (float)y, (float)z);

    lua_pushnumber(L, r);
    return 1;
}

static int math_clamp(lua_State* L)
{
    double v = luaL_checknumber(L, 1);
    double min = luaL_checknumber(L, 2);
    double max = luaL_checknumber(L, 3);

    /*max must be greater than or equal to min*/ scrypt_def(STR_0, "\x93\x9f\x88\xe0\x93\x8b\x8d\x8c\xe0\x9e\x9b\xe0\x99\x8e\x9b\x9f\x8c\x9b\x8e\xe0\x8c\x98\x9f\x92\xe0\x91\x8e\xe0\x9b\x8f\x8b\x9f\x94\xe0\x8c\x91\xe0\x93\x97\x92");
    luaL_argcheck(L, min <= max, 3, STR_0->c_str());

    double r = v < min ? min : v;
    r = r > max ? max : r;

    lua_pushnumber(L, r);
    return 1;
}

static int math_sign(lua_State* L)
{
    double v = luaL_checknumber(L, 1);
    lua_pushnumber(L, v > 0.0 ? 1.0 : v < 0.0 ? -1.0 : 0.0);
    return 1;
}

static int math_round(lua_State* L)
{
    lua_pushnumber(L, round(luaL_checknumber(L, 1)));
    return 1;
}

static int math_map(lua_State* L)
{
    double x = luaL_checknumber(L, 1);
    double inmin = luaL_checknumber(L, 2);
    double inmax = luaL_checknumber(L, 3);
    double outmin = luaL_checknumber(L, 4);
    double outmax = luaL_checknumber(L, 5);

    double result = outmin + (x - inmin) * (outmax - outmin) / (inmax - inmin);
    lua_pushnumber(L, result);
    return 1;
}

/*
** Open math library
*/
int luaopen_math(lua_State* L)
{
    std::string STR_0 = /*abs*/ scrypt("\x9f\x9e\x8d");
    std::string STR_1 = /*acos*/ scrypt("\x9f\x9d\x91\x8d");
    std::string STR_2 = /*asin*/ scrypt("\x9f\x8d\x97\x92");
    std::string STR_3 = /*atan2*/ scrypt("\x9f\x8c\x9f\x92\xce");
    std::string STR_4 = /*atan*/ scrypt("\x9f\x8c\x9f\x92");
    std::string STR_5 = /*ceil*/ scrypt("\x9d\x9b\x97\x94");
    std::string STR_6 = /*cosh*/ scrypt("\x9d\x91\x8d\x98");
    std::string STR_7 = /*cos*/ scrypt("\x9d\x91\x8d");
    std::string STR_8 = /*deg*/ scrypt("\x9c\x9b\x99");
    std::string STR_9 = /*exp*/ scrypt("\x9b\x88\x90");
    std::string STR_10 = /*floor*/ scrypt("\x9a\x94\x91\x91\x8e");
    std::string STR_11 = /*fmod*/ scrypt("\x9a\x93\x91\x9c");
    std::string STR_12 = /*frexp*/ scrypt("\x9a\x8e\x9b\x88\x90");
    std::string STR_13 = /*ldexp*/ scrypt("\x94\x9c\x9b\x88\x90");
    std::string STR_14 = /*log10*/ scrypt("\x94\x91\x99\xcf\xd0");
    std::string STR_15 = /*log*/ scrypt("\x94\x91\x99");
    std::string STR_16 = /*max*/ scrypt("\x93\x9f\x88");
    std::string STR_17 = /*min*/ scrypt("\x93\x97\x92");
    std::string STR_18 = /*modf*/ scrypt("\x93\x91\x9c\x9a");
    std::string STR_19 = /*pow*/ scrypt("\x90\x91\x89");
    std::string STR_20 = /*rad*/ scrypt("\x8e\x9f\x9c");
    std::string STR_21 = /*random*/ scrypt("\x8e\x9f\x92\x9c\x91\x93");
    std::string STR_22 = /*randomseed*/ scrypt("\x8e\x9f\x92\x9c\x91\x93\x8d\x9b\x9b\x9c");
    std::string STR_23 = /*sinh*/ scrypt("\x8d\x97\x92\x98");
    std::string STR_24 = /*sin*/ scrypt("\x8d\x97\x92");
    std::string STR_25 = /*sqrt*/ scrypt("\x8d\x8f\x8e\x8c");
    std::string STR_26 = /*tanh*/ scrypt("\x8c\x9f\x92\x98");
    std::string STR_27 = /*tan*/ scrypt("\x8c\x9f\x92");
    std::string STR_28 = /*noise*/ scrypt("\x92\x91\x97\x8d\x9b");
    std::string STR_29 = /*clamp*/ scrypt("\x9d\x94\x9f\x93\x90");
    std::string STR_30 = /*sign*/ scrypt("\x8d\x97\x99\x92");
    std::string STR_31 = /*round*/ scrypt("\x8e\x91\x8b\x92\x9c");
    std::string STR_32 = /*pi*/ scrypt("\x90\x97");
    std::string STR_33 = /*huge*/ scrypt("\x98\x8b\x99\x9b");
    std::string STR_34 = /*map*/ scrypt("\x93\x9f\x90");
    std::string STR_35 = /*math*/ scrypt("\x93\x9f\x8c\x98");

    const luaL_Reg mathlib[] = {
        {STR_0.c_str(), math_abs},
        {STR_1.c_str(), math_acos},
        {STR_2.c_str(), math_asin},
        {STR_3.c_str(), math_atan2},
        {STR_4.c_str(), math_atan},
        {STR_5.c_str(), math_ceil},
        {STR_6.c_str(), math_cosh},
        {STR_7.c_str(), math_cos},
        {STR_8.c_str(), math_deg},
        {STR_9.c_str(), math_exp},
        {STR_10.c_str(), math_floor},
        {STR_11.c_str(), math_fmod},
        {STR_12.c_str(), math_frexp},
        {STR_13.c_str(), math_ldexp},
        {STR_14.c_str(), math_log10},
        {STR_15.c_str(), math_log},
        {STR_16.c_str(), math_max},
        {STR_17.c_str(), math_min},
        {STR_18.c_str(), math_modf},
        {STR_19.c_str(), math_pow},
        {STR_20.c_str(), math_rad},
        {STR_21.c_str(), math_random},
        {STR_22.c_str(), math_randomseed},
        {STR_23.c_str(), math_sinh},
        {STR_24.c_str(), math_sin},
        {STR_25.c_str(), math_sqrt},
        {STR_26.c_str(), math_tanh},
        {STR_27.c_str(), math_tan},
        {STR_28.c_str(), math_noise},
        {STR_29.c_str(), math_clamp},
        {STR_30.c_str(), math_sign},
        {STR_31.c_str(), math_round},
        {NULL, NULL},
    };

    uint64_t seed = uintptr_t(L);
    seed ^= time(NULL);
    seed ^= clock();

    pcg32_seed(&L->global->rngstate, seed);

    luaL_register(L, STR_35.c_str(), mathlib);
    lua_pushnumber(L, PI);
    lua_setfield(L, -2, STR_32.c_str());
    lua_pushnumber(L, HUGE_VAL);
    lua_setfield(L, -2, STR_33.c_str());

    if (FFlag::LuauMathMap)
    {
        lua_pushcfunction(L, math_map, STR_34.c_str());
        lua_setfield(L, -2, STR_34.c_str());
    }

    return 1;
}
