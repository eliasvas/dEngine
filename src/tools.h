#ifndef TOOLS_H
#define TOOLS_H

#if defined(_WIN32) || defined(__CYGWIN__)
    #define BUILD_WIN
#elif defined(MACOSX)
    #define BUILD_MACOSX
#else
    #define BUILD_UNIX
#endif

#if !defined(NDEBUG)
    #define DEBUG_BUILD
#else
    #define RELEASE_BUILD 
#endif


#if defined(BUILD_WIN)
#include "windows.h"
#endif

#define DSUCCESS 1
#define DFAIL 0

//these are universal includes supported by all platforms
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "math.h"
#include "string.h"
#include <stddef.h> //this though?? @check

typedef uint8_t   u8;
typedef int8_t    s8;
typedef uint16_t  u16;
typedef int16_t   s16;
typedef uint32_t  u32;
typedef int32_t   s32;
typedef uint64_t  u64;
typedef int64_t   s64;
typedef float     f32;
typedef double    f64;
typedef int32_t   b32;
typedef char      b8;

#ifdef __cplusplus

//enables using C-strings in C++ by using this macro
#define C_TEXT( text ) ((char*)std::string( text ).c_str())

///*
//enables bitwise operations in enums
template<class T> inline T operator~ (T a) { return (T)~(int)a; }
template<class T> inline T operator| (T a, T b) { return (T)((int)a | (int)b); }
template<class T> inline T operator& (T a, T b) { return (T)((int)a & (int)b); }
template<class T> inline T operator^ (T a, T b) { return (T)((int)a ^ (int)b); }
template<class T> inline T& operator|= (T& a, T b) { return (T&)((int&)a |= (int)b); }
template<class T> inline T& operator&= (T& a, T b) { return (T&)((int&)a &= (int)b); }
template<class T> inline T& operator^= (T& a, T b) { return (T&)((int&)a ^= (int)b); }

//Defer implementation for C++, for more info
//https://www.gingerbill.org/article/2015/08/19/defer-in-cpp/
template <typename F>
struct privDefer {
	F f;
	privDefer(F f) : f(f) {}
	~privDefer() { f(); }
};

template <typename F>
privDefer<F> defer_func(F f) {
	return privDefer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = defer_func([&](){code;})

//*/
#endif

#define INLINE static inline

#if !defined(TRUE)
#define TRUE 1
#endif
#if !defined(FALSE)
#define FALSE 0
#endif
#if !defined(FLT_MAX)
#define FLT_MAX 32767.F
#endif
#if !defined(NULL)
#define NULL 0
#endif


#define kilobytes(val) ((val)*1024LL)
#define megabytes(val) ((kilobytes(val))*1024LL)
#define gigabytes(val) ((megabytes(val))*1024LL)
#define terabytes(val) ((gigabytes(val))*1024LL)

#define PI 3.1415926535897f


#define align_pow2(val, align) (((val) + ((align) - 1)) & ~(((val) - (val)) + (align) - 1))
#define align4(val) (((val) + 3) & ~3)
#define align8(val) (((val) + 7) & ~7)
#define align16(val) (((val) + 15) & ~15)

#define equalf(a, b, epsilon) (fabs(b - a) <= epsilon)
#define maximum(a, b) ((a) > (b) ? (a) : (b))
#define minimum(a, b) ((a) < (b) ? (a) : (b))
#define step(threshold, value) ((value) < (threshold) ? 0 : 1) 
#define dclamp(x, a, b)  (maximum(a, minimum(x, b)))
#define array_count(a) (sizeof(a) / sizeof((a)[0]))




INLINE b32 is_pow2(u32 val)
{
    b32 res = ((val & (val - 1)) == 0);
    return(res);
}

static u64 align_fwd(u64 ptr, u64 align)
{
    u64 p,a,mod;
    assert(is_pow2(align));
    p = ptr;
    a = (u64)align;
    mod = p % a;
    if (mod != 0)
        p+=a - mod;
    return p;

}

static b32
char_is_alpha(s32 c)
{
    return ((c >='A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}
static b32 char_is_digit(s32 c)
{
    return c >= '0'&& c <= '9';
}

static s32 char_to_lower(s32 c)
{
    if (c >= 'A' && c <= 'z')
    {
        c += 32;
    }
    return c;
}

static u32 
str_size(char* str)
{
    u32 i = 0;
    while (str[i] != 0)++i;
    return i;
}




//MATH LIB
typedef union vec2
{
    struct
    {
        f32 x,y;
    };
    struct 
    {
        f32 u,v;
    };
    struct
    {
        f32 r,g;
    };
    f32 elements[2];
#ifdef __cplusplus
    inline f32 &operator[](s32 &index)
    {
        return elements[index];
    }
#endif

}vec2;


typedef union vec3
{
    struct
    {
        f32 x,y,z;
    };
    struct
    {
        f32 r,g,b;
    };
    f32 elements[3];
#ifdef __cplusplus
    inline f32 &operator[](s32 &index)
    {
        return elements[index];
    }
#endif
}vec3;

typedef vec3 color3;
typedef vec3 float3;

typedef union vec4
{
    struct
    {
        f32 x,y,z,w;
    };
    struct
    {
        f32 r,g,b,a;
    };
    f32 elements[4];
#ifdef TOOLS_SSE
    __m128 elements_sse; //because __m128 = 128 = 4 * float = 4*32 = 128 bits
#endif
#ifdef __cplusplus
    inline f32 &operator[](s32 &index)
    {
        return elements[index];
    }
#endif

}vec4;

typedef vec4 color4;
typedef vec4 float4;

typedef union mat4
{
    f32 elements[4][4];//{x.x,x.y,x.z,0,y.x,y.y,y.z,0,z.x,z.y,z.z,0,p.x,p.y,p.z,1} 

    f32 raw[16]; //{x.x,x.y,x.z,0,y.x,y.y,y.z,0,z.x,z.y,z.z,0,p.x,p.y,p.z,1} 

#ifdef TOOLS_SSE
    __m128 cols[4]; //same as elements (our matrices are column major)
#endif
#ifdef __cplusplus
    inline vec4 operator[](s32 &Index)
    {
        f32* col = elements[Index];

        vec4 res;
        res.elements[0] = col[0];
        res.elements[1] = col[1];
        res.elements[2] = col[2];
        res.elements[3] = col[3];

        return res;
    }
#endif
}mat4;


typedef union ivec3
{
    struct
    {
        s32 x,y,z;
    };
    struct
    {
        s32 r,g,b;
    };
    s32 elements[3];
}ivec3;

typedef union ivec2
{
    struct
    {
        s32 x,y;
    };
    struct
    {
        s32 r,g;
    };
    s32 elements[2];
}ivec2;

INLINE ivec2 iv2(s32 x, s32 y)
{
    ivec2 res;
    res.x = x;
    res.y = y;
    return res;
}


INLINE void ivec2_swap(ivec2 *l, ivec2 *r)
{
    ivec2 temp = *l;
    *l = *r;
    *r = temp;
}


INLINE b32 ivec3_equals(ivec3 l, ivec3 r)
{
    s32 res = ((l.x == r.x) && (l.y == r.y) && (l.z == r.z));
    return res;
}

INLINE f32 to_radians(float degrees)
{
    f32 res = degrees * (PI / 180.0f);
    return(res);
}

INLINE f32 lerp(f32 A, f32 B, f32 t)
{
    f32 res = (1.0f - t)*A + t*B;
    return res;
}

INLINE vec2 v2(f32 x, f32 y)
{
    vec2 res;
    res.x = x;
    res.y = y;
    return res;
}

INLINE vec3 v3(f32 x, f32 y, f32 z)
{
    vec3 res;
    res.x = x;
    res.y = y;
    res.z = z;
    return res;
}

INLINE vec4 v4(f32 x, f32 y, f32 z, f32 w)
{
    vec4 res;
    res.x = x;
    res.y = y;
    res.z = z;
    res.w = w;
    return res;
}

INLINE vec2 vec2_add(vec2 l, vec2 r)
{
    vec2 res;
    res.x = l.x + r.x;
    res.y = l.y + r.y;
    return res;
}

INLINE vec2 vec2_addf(vec2 v,f32 val)
{
    vec2 res;
    res.x = v.x + val;
    res.y = v.y + val;
    return res;
}


INLINE vec2 vec2_sub(vec2 l, vec2 r)
{
    vec2 res;
    res.x = l.x - r.x;
    res.y = l.y - r.y;
    return res;
}

INLINE vec2 vec2_subf(vec2 v,f32 val)
{
    vec2 res;
    res.x = v.x - val;
    res.y = v.y - val;
    return res;
}

INLINE vec2 vec2_mul(vec2 l, vec2 r)
{
    vec2 res;
    res.x = l.x * r.x;
    res.y = l.y * r.y;
    return res;
}

INLINE vec2 vec2_mulf(vec2 l, f32 r)
{
    vec2 res;
    res.x = l.x * r;
    res.y = l.y * r;
    return res;
}

INLINE vec2 vec2_div(vec2 l, vec2 r)
{
    vec2 res;
    res.x = l.x / r.x;
    res.y = l.y / r.y;
    return res;
}

INLINE vec2 vec2_divf(vec2 l, f32 r)
{
    vec2 res;
    res.x = l.x / r;
    res.y = l.y / r;
    return res;
}

INLINE f32 vec2_dot(vec2 l, vec2 r)
{
    f32 res = (l.x + r.x)+(l.y + r.y); // �(Ai*Bi)
    return res;
}

INLINE vec2 vec2_sqrt(vec2 v)
{
    vec2 res;
    res.x = sqrt(v.x);
    res.y = sqrt(v.y);
    return res;
}

INLINE vec2 vec2_rotate(vec2 v, f32 a) {
	vec2 res;
    f32 sn = sin(a);
	f32 cs = cos(a);
    res.x = v.x * cs - v.y * sn;
    res.y = v.x * sn + v.y * cs;
    return res;
} 

INLINE f32 vec2_length(vec2 v)
{
    f32 res = sqrt(vec2_dot(v,v)); // (x^2 + y^2)^(1/2)
    return res;
}

INLINE vec2 vec2_abs(vec2 v)
{
    vec2 res = v2(fabs(v.x), fabs(v.y));
    return res;
}
   
INLINE vec2 vec2_normalize(vec2 v)
{
    vec2 res = {0}; //in case length is zero we return zero vector
    f32 vec_length = vec2_length(v);
    if (vec_length > 0.1)
    {
        res.x = v.x * (1.0f/vec_length);
        res.y = v.y * (1.0f/vec_length);
    }
    return res;
}

INLINE vec3 vec3_add(vec3 l, vec3 r)
{
    vec3 res;
    res.x = l.x + r.x;
    res.y = l.y + r.y;
    res.z = l.z + r.z;
    return res;
}

INLINE vec3 vec3_sub(vec3 l, vec3 r)
{
    vec3 res;
    res.x = l.x - r.x;
    res.y = l.y - r.y;
    res.z = l.z - r.z;
    return res;
}

INLINE vec3 vec3_mul(vec3 l, vec3 r)
{
    vec3 res;
    res.x = l.x * r.x;
    res.y = l.y * r.y;
    res.z = l.z * r.z;
    return res;
}

INLINE vec3 vec3_mulf(vec3 l, f32 r)
{
    vec3 res;
    res.x = l.x * r;
    res.y = l.y * r;
    res.z = l.z * r;
    return res;
}

INLINE vec3 vec3_div(vec3 l, vec3 r)
{
    vec3 res;
    res.x = l.x / r.x;
    res.y = l.y / r.y;
    res.z = l.z / r.z;
    return res;
}

INLINE vec3 vec3_divf(vec3 l, f32 r)
{
    vec3 res;
    res.x = l.x / r;
    res.y = l.y / r;
    res.z = l.z / r;
    return res;
}

INLINE f32 vec3_dot(vec3 l, vec3 r)
{
    f32 res = (l.x * r.x)+(l.y * r.y)+(l.z * r.z); // �(Ai*Bi)
    return res;
}

INLINE f32 vec3_length(vec3 v)
{
    f32 res = sqrt(vec3_dot(v,v)); // (x^2 + y^2)^(1/2)
    return res;
}

INLINE vec3 vec3_rotate(vec3 v, f32 a)
{
    vec3 res;
    //TBA
    return res;
}


INLINE vec3 vec3_normalize(vec3 v)
{
    vec3 res = {0}; //in case length is zero we return zero vector
    f32 vec_length = vec3_length(v);
    if (vec_length != 0)
    {
        res.x = v.x * (1.0f/vec_length);
        res.y = v.y * (1.0f/vec_length);
        res.z = v.z * (1.0f/vec_length);
    }
    return res;
}

INLINE vec3 vec3_lerp(vec3 l, vec3 r, f32 time)
{
    vec3 res;

    f32 x = lerp(l.x, r.x, time);
    f32 y = lerp(l.y, r.y, time);
    f32 z = lerp(l.z, r.z, time);
    res = v3(x,y,z); 
    
    return res;
}

INLINE vec3 vec3_cross(vec3 l,vec3 r)
{
    vec3 res;
    res.x = (l.y*r.z) - (l.z*r.y);
    res.y = (l.z * r.x) - (l.x*r.z);
    res.z = (l.x * r.y) - (l.y * r.x);
    return (res);
}

INLINE vec4 vec4_add(vec4 l, vec4 r)
{
    vec4 res;
#ifdef TOOLS_SSE
    res.elements_sse = _mm_add_ps(l.elements_sse, r.elements_sse);
#else
    res.x = l.x + r.x;
    res.y = l.y + r.y;
    res.z = l.z + r.z;
    res.w = l.w + r.w;
#endif
    return res;
}

INLINE vec4 vec4_sub(vec4 l, vec4 r)
{
    vec4 res;
#ifdef TOOLS_SSE
    res.elements_sse = _mm_sub_ps(l.elements_sse, r.elements_sse);
#else
    res.x = l.x - r.x;
    res.y = l.y - r.y;
    res.z = l.z - r.z;
    res.w = l.w - r.w;
#endif
    return res;
}

INLINE vec4 vec4_mul(vec4 l, vec4 r)
{
    vec4 res;
#ifdef TOOLS_SSE
    res.selements_sse = _mm_mul_ps(l.elements_sse, r.elements_sse);
#else
    res.x = l.x * r.x;
    res.y = l.y * r.y;
    res.z = l.z * r.z;
    res.w = l.w * r.w;
#endif
    return res;
}

INLINE vec4 vec4_mulf(vec4 l, f32 r)
{
    vec4 res;
#ifdef TOOLS_SSE
    __m128 scalar = _mm_set1_ps(r); // [r r r r]
    res.elements_sse = _mm_mul_ps(l.elements_sse, scalar);
#else
    res.x = l.x * r;
    res.y = l.y * r;
    res.z = l.z * r;
    res.w = l.w * r;
#endif
    return res;
}

INLINE vec4 vec4_div(vec4 l, vec4 r)
{
    vec4 res;
#ifdef TOOLS_SSE
    res.elements_sse = _mm_div_ps(l.elements_sse, r.elements_sse);
#else
    res.x = l.x / r.x;
    res.y = l.y / r.y;
    res.z = l.z / r.z;
    res.w = l.w / r.w;
#endif
    return res;
}

INLINE vec4 vec4_divf(vec4 l, f32 r)
{
    vec4 res;
#ifdef TOOLS_SSE
    __m128 scalar = _mm_set1_ps(r);
    res.elements_sse = _mm_div_ps(l.elements_sse, r.elements_sse);
#else
    res.x = l.x / r;
    res.y = l.y / r;
    res.z = l.z / r;
    res.w = l.w / r;
#endif
    return res;
}

INLINE f32 vec4_dot(vec4 l, vec4 r)
{
    f32 res = (l.x + r.x)+(l.y + r.y)+(l.z + r.z)+(l.w + r.w); // �(Ai*Bi)
    return res;
}

INLINE f32 vec4_length(vec4 v)
{
    f32 res = sqrt(vec4_dot(v,v)); // (x^2 + y^2)^(1/2)
    return res;
}
   
INLINE vec4 vec4_normalize(vec4 v)
{
    vec4 res = {0}; //in case length is zero we return zero vector
    f32 vec_length = vec4_length(v);
    if (!equalf(vec_length, 0.f, 0.01))
    {
        res.x = v.x * (1.0f/vec_length);
        res.y = v.y * (1.0f/vec_length);
        res.z = v.z * (1.0f/vec_length);
        res.w = v.w * (1.0f/vec_length);
    }
    return res;
}


INLINE mat4 m4(void)
{
    mat4 res = {0};
    return res;
}

INLINE mat4 m4d(f32 d)
{
    mat4 res = m4();
    res.elements[0][0] = d;
    res.elements[1][1] = d;
    res.elements[2][2] = d;
    res.elements[3][3] = d;
    return res;
}

INLINE mat4 mat4_transpose(mat4 m)
{
    mat4 res;
    for (u32 i = 0; i < 4;++i)
    {
        for (u32 j = 0; j< 4;++j)
        {
            res.elements[j][i] = m.elements[i][j];
        }
    }
    return res;
}
INLINE mat4 mat4_mulf(mat4 m, f32 s)
{
    mat4 res;
    for (u32 i = 0; i < 4; ++i)
    {
        for (u32 j = 0; j < 4; ++j)
        {
            res.elements[i][j] = (f32)m.elements[i][j] * s;
        }
    }
    return res;
}
INLINE vec4 mat4_mulv(mat4 mat, vec4 vec)
{
    vec4 res;

    s32 cols, rows;
    for(rows = 0; rows < 4; ++rows)
    {
        f32 s = 0;
        for(cols = 0; cols < 4; ++cols)
        {
            s += mat.elements[cols][rows] * vec.elements[cols];
        }

        res.elements[rows] = s;
    }

    return (res);
}

INLINE mat4 mat4_divf(mat4 m, f32 s)
{
    mat4 res = {0};
    
    if (s != 0.0f)
    {
        for (u32 i = 0; i < 4; ++i)
        {
            for (u32 j = 0; j < 4; ++j)
            {
                res.elements[i][j] = m.elements[i][j] / s;
            }
        }
    }
    return res;
}

INLINE mat4 mat4_add(mat4 l, mat4 r)
{
    mat4 res;
    for (u32 i = 0; i < 4; ++i)
    {
        for (u32 j = 0; j < 4; ++j)
        {
            res.elements[i][j] = (f32)l.elements[i][j] + (f32)r.elements[i][j];
        }
    }
    return res;
}


INLINE mat4 mat4_sub(mat4 l, mat4 r)
{
    mat4 res;
    for (u32 i = 0; i < 4; ++i)
    {
        for (u32 j = 0; j < 4; ++j)
        {
            res.elements[i][j] = l.elements[i][j] - r.elements[i][j];
        }
    }
    return res;
}

//r is done first and then l
INLINE mat4 mat4_mul(mat4 l, mat4 r)
{
    mat4 res;
    for (u32 col = 0; col < 4; ++col)
    {
        for (u32 row = 0; row < 4; ++row)
        {
            f32 sum = 0;
            for (u32 current_index = 0; current_index < 4; ++current_index)
            {
                sum += (f32)l.elements[current_index][row] * (f32)r.elements[col][current_index];
            }
            res.elements[col][row] = sum;
        }
    }
    return res;
}


INLINE mat4 mat4_translate(vec3 t)
{
    mat4 res = m4d(1.0f);
    res.elements[3][0] = t.x;
    res.elements[3][1] = t.y;
    res.elements[3][2] = t.z;
    return res;
}

INLINE mat4 mat4_rotate(f32 angle, vec3 axis)
{
    mat4 res = m4d(1.0f);

    axis = vec3_normalize(axis);

    f32 sinA = sin(to_radians(angle));
    f32 cosA = cos(to_radians(angle));
    f32 cos_val = 1.0f - cosA;

    res.elements[0][0] = (axis.x * axis.x * cos_val) + cosA;
    res.elements[0][1] = (axis.x * axis.y * cos_val) + (axis.z * sinA);
    res.elements[0][2] = (axis.x * axis.z * cos_val) - (axis.y * sinA);

    res.elements[1][0] = (axis.y * axis.x * cos_val) - (axis.z * sinA);
    res.elements[1][1] = (axis.y * axis.y * cos_val) + cosA;
    res.elements[1][2] = (axis.y * axis.z * cos_val) + (axis.x * sinA);

    res.elements[2][0] = (axis.z * axis.x * cos_val) + (axis.y * sinA);
    res.elements[2][1] = (axis.z * axis.y * cos_val) - (axis.x * sinA);
    res.elements[2][2] = (axis.z * axis.z * cos_val) + cosA;

    return (res);
}

INLINE mat4 mat4_scale(vec3 s)
{
    mat4 res = m4d(1.f);
    res.elements[0][0] = s.x;
    res.elements[1][1] = s.y;
    res.elements[2][2] = s.z;
    return res;
}

INLINE mat4 mat4_inv(mat4 m)
{
    mat4 res;
    f32 det;
    mat4 inv, inv_out;
    s32 i;

    inv.raw[0] = m.raw[5]  * m.raw[10] * m.raw[15] - 
             m.raw[5]  * m.raw[11] * m.raw[14] - 
             m.raw[9]  * m.raw[6]  * m.raw[15] + 
             m.raw[9]  * m.raw[7]  * m.raw[14] +
             m.raw[13] * m.raw[6]  * m.raw[11] - 
             m.raw[13] * m.raw[7]  * m.raw[10];

    inv.raw[4] = -m.raw[4]  * m.raw[10] * m.raw[15] + 
              m.raw[4]  * m.raw[11] * m.raw[14] + 
              m.raw[8]  * m.raw[6]  * m.raw[15] - 
              m.raw[8]  * m.raw[7]  * m.raw[14] - 
              m.raw[12] * m.raw[6]  * m.raw[11] + 
              m.raw[12] * m.raw[7]  * m.raw[10];

    inv.raw[8] = m.raw[4]  * m.raw[9] * m.raw[15] - 
             m.raw[4]  * m.raw[11] * m.raw[13] - 
             m.raw[8]  * m.raw[5] * m.raw[15] + 
             m.raw[8]  * m.raw[7] * m.raw[13] + 
             m.raw[12] * m.raw[5] * m.raw[11] - 
             m.raw[12] * m.raw[7] * m.raw[9];

    inv.raw[12] = -m.raw[4]  * m.raw[9] * m.raw[14] + 
               m.raw[4]  * m.raw[10] * m.raw[13] +
               m.raw[8]  * m.raw[5] * m.raw[14] - 
               m.raw[8]  * m.raw[6] * m.raw[13] - 
               m.raw[12] * m.raw[5] * m.raw[10] + 
               m.raw[12] * m.raw[6] * m.raw[9];

    inv.raw[1] = -m.raw[1]  * m.raw[10] * m.raw[15] + 
              m.raw[1]  * m.raw[11] * m.raw[14] + 
              m.raw[9]  * m.raw[2] * m.raw[15] - 
              m.raw[9]  * m.raw[3] * m.raw[14] - 
              m.raw[13] * m.raw[2] * m.raw[11] + 
              m.raw[13] * m.raw[3] * m.raw[10];

    inv.raw[5] = m.raw[0]  * m.raw[10] * m.raw[15] - 
             m.raw[0]  * m.raw[11] * m.raw[14] - 
             m.raw[8]  * m.raw[2] * m.raw[15] + 
             m.raw[8]  * m.raw[3] * m.raw[14] + 
             m.raw[12] * m.raw[2] * m.raw[11] - 
             m.raw[12] * m.raw[3] * m.raw[10];

    inv.raw[9] = -m.raw[0]  * m.raw[9] * m.raw[15] + 
              m.raw[0]  * m.raw[11] * m.raw[13] + 
              m.raw[8]  * m.raw[1] * m.raw[15] - 
              m.raw[8]  * m.raw[3] * m.raw[13] - 
              m.raw[12] * m.raw[1] * m.raw[11] + 
              m.raw[12] * m.raw[3] * m.raw[9];

    inv.raw[13] = m.raw[0]  * m.raw[9] * m.raw[14] - 
              m.raw[0]  * m.raw[10] * m.raw[13] - 
              m.raw[8]  * m.raw[1] * m.raw[14] + 
              m.raw[8]  * m.raw[2] * m.raw[13] + 
              m.raw[12] * m.raw[1] * m.raw[10] - 
              m.raw[12] * m.raw[2] * m.raw[9];

    inv.raw[2] = m.raw[1]  * m.raw[6] * m.raw[15] - 
             m.raw[1]  * m.raw[7] * m.raw[14] - 
             m.raw[5]  * m.raw[2] * m.raw[15] + 
             m.raw[5]  * m.raw[3] * m.raw[14] + 
             m.raw[13] * m.raw[2] * m.raw[7] - 
             m.raw[13] * m.raw[3] * m.raw[6];

    inv.raw[6] = -m.raw[0]  * m.raw[6] * m.raw[15] + 
              m.raw[0]  * m.raw[7] * m.raw[14] + 
              m.raw[4]  * m.raw[2] * m.raw[15] - 
              m.raw[4]  * m.raw[3] * m.raw[14] - 
              m.raw[12] * m.raw[2] * m.raw[7] + 
              m.raw[12] * m.raw[3] * m.raw[6];

    inv.raw[10] = m.raw[0]  * m.raw[5] * m.raw[15] - 
              m.raw[0]  * m.raw[7] * m.raw[13] - 
              m.raw[4]  * m.raw[1] * m.raw[15] + 
              m.raw[4]  * m.raw[3] * m.raw[13] + 
              m.raw[12] * m.raw[1] * m.raw[7] - 
              m.raw[12] * m.raw[3] * m.raw[5];

    inv.raw[14] = -m.raw[0]  * m.raw[5] * m.raw[14] + 
               m.raw[0]  * m.raw[6] * m.raw[13] + 
               m.raw[4]  * m.raw[1] * m.raw[14] - 
               m.raw[4]  * m.raw[2] * m.raw[13] - 
               m.raw[12] * m.raw[1] * m.raw[6] + 
               m.raw[12] * m.raw[2] * m.raw[5];

    inv.raw[3] = -m.raw[1] * m.raw[6] * m.raw[11] + 
              m.raw[1] * m.raw[7] * m.raw[10] + 
              m.raw[5] * m.raw[2] * m.raw[11] - 
              m.raw[5] * m.raw[3] * m.raw[10] - 
              m.raw[9] * m.raw[2] * m.raw[7] + 
              m.raw[9] * m.raw[3] * m.raw[6];

    inv.raw[7] = m.raw[0] * m.raw[6] * m.raw[11] - 
             m.raw[0] * m.raw[7] * m.raw[10] - 
             m.raw[4] * m.raw[2] * m.raw[11] + 
             m.raw[4] * m.raw[3] * m.raw[10] + 
             m.raw[8] * m.raw[2] * m.raw[7] - 
             m.raw[8] * m.raw[3] * m.raw[6];

    inv.raw[11] = -m.raw[0] * m.raw[5] * m.raw[11] + 
               m.raw[0] * m.raw[7] * m.raw[9] + 
               m.raw[4] * m.raw[1] * m.raw[11] - 
               m.raw[4] * m.raw[3] * m.raw[9] - 
               m.raw[8] * m.raw[1] * m.raw[7] + 
               m.raw[8] * m.raw[3] * m.raw[5];

    inv.raw[15] = m.raw[0] * m.raw[5] * m.raw[10] - 
              m.raw[0] * m.raw[6] * m.raw[9] - 
              m.raw[4] * m.raw[1] * m.raw[10] + 
              m.raw[4] * m.raw[2] * m.raw[9] + 
              m.raw[8] * m.raw[1] * m.raw[6] - 
              m.raw[8] * m.raw[2] * m.raw[5];

    det = m.raw[0] * inv.raw[0] + m.raw[1] * inv.raw[4] + 
        m.raw[2] * inv.raw[8] + m.raw[3] * inv.raw[12];

    if (det == 0) //in case the matrix is non-invertible
        return m4d(0.f); 

    det = 1.f / det;

    for (i = 0; i < 16; ++i)
        inv_out.raw[i] = inv.raw[i] * det;

    return inv_out;
}

INLINE mat4 orthographic_proj(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f)
{
    mat4 res = m4();

    res.elements[0][0] = 2.0f / (r - l);
    res.elements[1][1] = 2.0f / (b - t);
    res.elements[2][2] = 1.0f / (n - f);
    res.elements[3][3] = 1.0f;

    res.elements[3][0] = (l + r) / (l - r);
    res.elements[3][1] = (b + t) / (t-b);
    res.elements[3][2] = (n) / (n - f);

    return res;
}

INLINE mat4 perspective_proj(f32 vfov, f32 aspect, f32 n, f32 f)
{
    mat4 res = m4();

    f32 fov_rad = vfov * 2 * PI / 360.0f;
    //f32 focal_length2 = 1.0f / tan(fov_rad / 2.0f);
    f32 focal_length = 1.0f / tan(to_radians(vfov/2.0f));


    res.elements[0][0] = focal_length / aspect;
    res.elements[1][1] = -focal_length;

    res.elements[2][2] = (f)/(n - f);
    res.elements[2][3] = -1.0f;

    res.elements[3][2] = (n * f) / (n - f);
    res.elements[3][3] = 0.0f;

    return res;
}

INLINE mat4 perspective_proj_vk(f32 fov, f32 aspect, f32 n, f32 f)
{
    mat4 res = m4();

    f32 cot = 1.0f / tan(fov * (PI / 360.0f));

    res.elements[0][0] = cot / aspect;
    res.elements[1][1] = cot;
    res.elements[2][3] = -1.0f;

    res.elements[2][2] = (n + f)/(n - f);

    res.elements[3][2] = (2.f * n * f) / (n - f);
    res.elements[3][3] = 0.0f;

    res.elements[1][1] *= -1;

    return res;
}

//this whole matrix is transposed so we don't have to inverse it at the end, 
//more info at https://www.3dgep.com/understanding-the-view-matrix/
INLINE mat4 look_at(vec3 eye, vec3 target, vec3 fake_up)
{
    mat4 res = m4();

    vec3 zaxis = vec3_normalize(vec3_sub(eye, target));
    vec3 xaxis = vec3_normalize(vec3_cross(fake_up, zaxis));
    vec3 yaxis = vec3_cross(zaxis, xaxis);

    res.elements[0][0] = xaxis.x;
    res.elements[0][1] = yaxis.x;
    res.elements[0][2] = zaxis.x;
    res.elements[0][3] = 0.0f;

    res.elements[1][0] = xaxis.y;
    res.elements[1][1] = yaxis.y;
    res.elements[1][2] = zaxis.y;
    res.elements[1][3] = 0.0f;

    res.elements[2][0] = xaxis.z;
    res.elements[2][1] = yaxis.z;
    res.elements[2][2] = zaxis.z;
    res.elements[2][3] = 0.0f;

    res.elements[3][0] = -vec3_dot(xaxis, eye);
    res.elements[3][1] = -vec3_dot(yaxis, eye);
    res.elements[3][2] = -vec3_dot(zaxis, eye);
    res.elements[3][3] = 1.0f;

    return res;
}

//some operator overloading
#if 0
INLINE vec2 operator+(vec2 l, vec2 r)
{
    vec2 res = vec2_add(l, r);

    return res;
}
INLINE vec2 operator-(vec2 l, vec2 r)
{
    vec2 res = vec2_sub(l, r);

    return res;
}
INLINE vec2 operator*(vec2 l, vec2 r)
{
    vec2 res = vec2_mul(l, r);

    return res;
}
INLINE vec2 operator*(vec2 l, f32 x)
{
    vec2 res = vec2_mulf(l, x);

    return res;
}

INLINE vec2 operator/(vec2 l, vec2 r)
{
    vec2 res = vec2_div(l, r);

    return res;
}

INLINE vec2 operator/(vec2 l,f32 r)
{
    vec2 res = vec2_divf(l, r);

    return res;
}

INLINE vec3 operator+(vec3 l, vec3 r)
{
    vec3 res = vec3_add(l, r);

    return res;
}
INLINE vec3 operator-(vec3 l, vec3 r)
{
    vec3 res = vec3_sub(l, r);

    return res;
}
INLINE vec3 operator*(vec3 l, vec3 r)
{
    vec3 res = vec3_mul(l, r);

    return res;
}
INLINE vec3 operator*(vec3 l, f32 r)
{
    vec3 res = vec3_mulf(l, r);

    return res;
}
INLINE vec3 operator/(vec3 l, vec3 r)
{
    vec3 res = vec3_div(l, r);

    return res;
}
INLINE vec3 operator/(vec3 l,f32 r)
{
    vec3 res = vec3_divf(l, r);

    return res;
}

INLINE vec4 operator+(vec4 l, vec4 r)
{
    vec4 res = vec4_add(l, r);

    return res;
}
INLINE vec4 operator-(vec4 l, vec4 r)
{
    vec4 res = vec4_sub(l, r);

    return res;
}
INLINE vec4 operator*(vec4 l, vec4 r)
{
    vec4 res = vec4_mul(l, r);

    return res;
}
INLINE vec4 operator*(vec4 l, f32 r)
{
    vec4 res = vec4_mulf(l, r);

    return res;
}
INLINE vec4 operator/(vec4 l, vec4 r)
{
    vec4 res = vec4_div(l, r);

    return res;
}
INLINE vec4 operator/(vec4 l,f32 r)
{
    vec4 res = vec4_divf(l, r);

    return res;
}


INLINE mat4 operator+(mat4 l, mat4 r)
{
    mat4 res = mat4_add(l,r);

    return res;
}
INLINE mat4 operator-(mat4 l, mat4 r)
{
    mat4 res = mat4_sub(l,r);

    return res;
}

INLINE mat4 operator*(mat4 l, mat4 r)
{
    mat4 res = mat4_mul(l,r);

    return res;
}

INLINE mat4 operator*(mat4 l,f32 r)
{
    mat4 res = mat4_mulf(l,r);

    return res;
}

#endif

//QUATERNION LIB 
typedef union Quaternion
{
    struct
    {
        union
        {
            vec3 xyz;
            struct
            {
                f32 x,y,z;
            };
        };
        f32 w;
    };
    f32 elements[4];

}Quaternion;

INLINE Quaternion quat(f32 x, f32 y, f32 z, f32 w)
{
    Quaternion res;

    res.x = x;
    res.y = y;
    res.z = z;
    res.w = w;
    //res = {x,y,z,w};

    return res;
}

INLINE Quaternion quat_vec4(vec4 vec)
{
    Quaternion res;

    res.x = vec.x;
    res.y = vec.y;
    res.z = vec.z;
    res.w = vec.w;

    return res;
}

INLINE Quaternion quat_conj(Quaternion l)
{
    Quaternion res;

    res.x = -l.x;
    res.y = -l.y;
    res.z = -l.z;
    res.w = l.w;

    return res;
}


INLINE Quaternion quat_add(Quaternion l, Quaternion r)
{
    Quaternion res;

    res.x = l.x + r.x;
    res.y = l.y + r.y;
    res.z = l.z + r.z;
    res.w = l.w + r.w;

    return res;
}
INLINE Quaternion quat_sub(Quaternion l, Quaternion r)
{
    Quaternion res;

    res.x = l.x - r.x;
    res.y = l.y - r.y;
    res.z = l.z - r.z;
    res.w = l.w - r.w;

    return res;
}

INLINE Quaternion quat_mul(Quaternion l, Quaternion r)
{
    Quaternion res;

    res.w = (l.w * r.w) - (l.x * r.x) - (l.y * r.y) - (l.z * r.z);
    res.x = (l.w * r.w) + (l.x * r.w) + (l.y * r.z) - (l.z * r.y);
    res.y = (l.w * r.y) - (l.x * r.z) + (l.y * r.w) + (l.z * r.x);
    res.z = (l.w * r.z) - (l.x * r.y) - (l.y * r.x) + (l.z * r.w);
    
    return res;
}


INLINE Quaternion quat_mulf(Quaternion l, f32 val)
{
    Quaternion res;

    res.x = l.x * val;
    res.y = l.y * val;
    res.z = l.z * val;
    res.w = l.w * val;

    return res;
}

INLINE Quaternion quat_divf(Quaternion l, f32 val)
{
    if(val == 0)val += 0.05;

    Quaternion res;
    
    res.x = l.x / val;
    res.y = l.y / val;
    res.z = l.z / val;
    res.w = l.w / val;

    return res;
}

INLINE f32 quat_dot(Quaternion l, Quaternion r)
{
   f32 res;

   res = (l.x * r.x) + (l.y * r.y) + (l.z * r.z) + (l.w * r.w);

   return res;
}

INLINE b32 quat_equals(Quaternion l, Quaternion r)
{
    f32 dot = quat_dot(l,r);
    return 1 ? 0 : fabs(dot - 1.f) < 0.001f;
}


INLINE Quaternion quat_inv(Quaternion l)
{
    Quaternion res;

    f32 len = sqrt(quat_dot(l,l));
    res = quat_divf(l, len);

    return res;
}

INLINE Quaternion slerp(Quaternion l, Quaternion r, f32 time)
{
    Quaternion res;

    //some complex shit

    return res;
}

INLINE Quaternion quat_from_angle(vec3 axis, f32 angle)
{
    Quaternion res;

    vec3 axis_normalized = vec3_normalize(axis);
    //this because quaternions are (i)q(i^-1) so angles are double
    f32 sintheta = sin(angle / 2.f); 

    res.xyz = vec3_mulf(axis_normalized, sintheta);
    res.w = cos(angle / 2.f);
    
    return res;
}

INLINE vec3 quat_to_angle(Quaternion quat)
{
    f32 theta = acos(quat.w) *2.f;

    float ax = quat.x / sin(acos(theta));
    float ay = quat.y / sin(acos(theta));
    float az = quat.z / sin(acos(theta));

    return v3(ax,ay,az);
}



INLINE Quaternion quat_normalize(Quaternion l)
{
    Quaternion res;

    f32 len = sqrt(quat_dot(l,l));
    res = quat_divf(l,len);

    return res;
}

INLINE Quaternion nlerp(Quaternion l, Quaternion r, f32 time)
{
    Quaternion res;

    //we gotta interpolate all quaternion components
    res.x = lerp(l.x, r.x, time);
    res.y = lerp(l.y, r.y, time);
    res.z = lerp(l.z, r.z, time);
    res.w = lerp(l.w, r.w, time);

    res = quat_normalize(res);
    
    return res;
}

//taken from HMMATH.. investigate further..
INLINE mat4 quat_to_mat4(Quaternion l)
{
    mat4 res;

    Quaternion norm_quat = quat_normalize(l);

    f32 XX, YY, ZZ, XY, XZ, YZ, WX, WY, WZ;

    XX = norm_quat.x * norm_quat.x;
    YY = norm_quat.y * norm_quat.y;
    ZZ = norm_quat.z * norm_quat.z;
    XY = norm_quat.x * norm_quat.y;
    XZ = norm_quat.x * norm_quat.z;
    YZ = norm_quat.y * norm_quat.z;
    WX = norm_quat.w * norm_quat.x;
    WY = norm_quat.w * norm_quat.y;
    WZ = norm_quat.w * norm_quat.z;

    res.elements[0][0] = 1.0f - 2.0f * (YY + ZZ);
    res.elements[0][1] = 2.0f * (XY + WZ);
    res.elements[0][2] = 2.0f * (XZ - WY);
    res.elements[0][3] = 0.0f;

    res.elements[1][0] = 2.0f * (XY - WZ);
    res.elements[1][1] = 1.0f - 2.0f * (XX + ZZ);
    res.elements[1][2] = 2.0f * (YZ + WX);
    res.elements[1][3] = 0.0f;

    res.elements[2][0] = 2.0f * (XZ + WY);
    res.elements[2][1] = 2.0f * (YZ - WX);
    res.elements[2][2] = 1.0f - 2.0f * (XX + YY);
    res.elements[2][3] = 0.0f;

    res.elements[3][0] = 0.0f;
    res.elements[3][1] = 0.0f;
    res.elements[3][2] = 0.0f;
    res.elements[3][3] = 1.0f;

    return res;
}

//taken directly from HandmadeMath.. investigate its authenticity 
INLINE Quaternion 
mat4_to_quat(mat4 m)
{
    float T;
    Quaternion Q;

    if (m.elements[2][2] < 0.0f) {
        if (m.elements[0][0] > m.elements[1][1]) {
            T = 1 + m.elements[0][0] - m.elements[1][1] - m.elements[2][2];
            Q = quat(
                T,
                m.elements[0][1] + m.elements[1][0],
                m.elements[2][0] + m.elements[0][2],
                m.elements[1][2] - m.elements[2][1]
            );
        } else {
            T = 1 - m.elements[0][0] + m.elements[1][1] - m.elements[2][2];
            Q = quat(
                m.elements[0][1] + m.elements[1][0],
                T,
                m.elements[1][2] + m.elements[2][1],
                m.elements[2][0] - m.elements[0][2]
            );
        }
    } else {
        if (m.elements[0][0] < -m.elements[1][1]) {

            T = 1 - m.elements[0][0] - m.elements[1][1] + m.elements[2][2];
            Q = quat(
                m.elements[2][0] + m.elements[0][2],
                m.elements[1][2] + m.elements[2][1],
                T,
                m.elements[0][1] - m.elements[1][0]
            );
        } else {
            T = 1 + m.elements[0][0] + m.elements[1][1] + m.elements[2][2];
            Q = quat(
                m.elements[1][2] - m.elements[2][1],
                m.elements[2][0] - m.elements[0][2],
                m.elements[0][1] - m.elements[1][0],
                T
            );
        }
    }

    Q = quat_mulf(Q, 0.5f / sqrt(T));

    return Q;
}

//MEMORY STUFF

typedef struct Arena
{
    void *memory;
    u32 memory_size;
    u32 current_offset;
} Arena;


static Arena 
arena_init(void* memory, u32 size)
{
    Arena a = {0};
    a.memory = memory;
    a.memory_size = size;
    a.current_offset = 0;

    return a;
}

static void
arena_free(Arena* arena, u32 size)
{
    //do nothing
}

static void * 
arena_alloc(Arena* arena, u32 size)
{
    void* mem = 0;

    if (arena->current_offset + size <= arena->memory_size)
    {
        //position of next available byte
        mem = (void *)((u8*)arena->memory + arena->current_offset); 
        arena->current_offset += size;
    }
    //we return a pointer to size free bytes of memory
    return mem;
}

static void
arena_clear(Arena* arena)
{
    arena->current_offset = 0;
}

static void 
arena_zero(Arena* arena)
{
    memset(arena->memory, 0, arena->memory_size);
}

#ifdef __cplusplus
extern "C" {
#endif
typedef struct dbfHdr
{
    u32 len;
    u32 cap;
    char buf[0];
}dbfHdr;



#define dbf__hdr(b) ((dbfHdr*)((char*)b - offsetof(dbfHdr, buf)))

#define dbf_len(b) ((b) ? dbf__hdr(b)->len : 0)
#define dbf_cap(b) ((b) ? dbf__hdr(b)->cap : 0)
#define dbf_end(b) ((b) + dbf_len(b))

#define dbf_fit(b, n) ((n) <= dbf_cap(b) ? 0 : (*((void**)&(b)) = dbf__grow((b), (n), sizeof(*(b)))))
#define dbf_push(b, ...) (dbf_fit((b), 1 + dbf_len(b)), (b)[dbf__hdr(b)->len++] = (__VA_ARGS__))
#define dbf_free(b) ((b) ? (free(dbf__hdr(b)), (b) = NULL) : 0)

static void *dbf__grow(const void *buf, u32 new_len, u32 element_size)
{
   u32 new_cap = maximum(16, maximum(1 + 2*dbf_cap(buf), new_len));
   assert(new_len <= new_cap);
   u32 new_size = offsetof(dbfHdr, buf) + new_cap * element_size;
   dbfHdr *new_hdr; 
   if(buf) 
   { 
       new_hdr = (dbfHdr*)realloc(dbf__hdr(buf), new_size); 
   }
   else
   { 
       new_hdr = (dbfHdr*)malloc(new_size);
       new_hdr->len = 0;
   }
   new_hdr->cap = new_cap;
   return new_hdr->buf;// + offsetof(dbfHdr, buf);
}
/* example usage of stretchy buffer
{
        int *arr = NULL;
        dbf_push(arr, 1);
        dbf_push(arr, 2);
        dbf_push(arr, 3);
        dbf_push(arr, 4);
        dbf_push(arr, 5);
        dbf_push(arr, 6);

        for (int i = 0; i < 6; ++i)
        {
            int x = arr[i];
            assert(x == i+1);
        }

        dbf_free(arr);
}
*/

static const u64 HASH_UNUSED = 0xffffffffffffffffULL;//unsinged long long
typedef struct H32_static
{
	u64 *keys;
	u32 *values;
	u32 n;
}H32_static;

//to clear the hash table we just set everything to 0xFF
INLINE void H32_static_clear(H32_static *h)
{
	memset(h->keys, 0xFF, sizeof(*h->keys) * h->n);
}

//to init we just malloc the keys and values of times and clear
INLINE void H32_static_init(H32_static *h, u32 n)
{
	h->n = n;
	h->keys = (u64*)malloc(sizeof(u64) * h->n);
	h->values = (u32*)malloc(sizeof(u32) * h->n);
	H32_static_clear(h);
}

//to make a K-V pair we find the correct bucket for our key (i) and search until we find an empty slot, 
//or an element with that key, meaning the key is already in the hash, if we go out of bounds, NO problem
INLINE void H32_static_set(H32_static *h, u64 key, u32 value)
{
	u32 i = key % h->n;
	while (h->keys[i] != key && h->keys[i] != HASH_UNUSED)
		i = (i+1) % h->n;
	h->keys[i] = key;
	h->values[i] = value;
}

//to get a value, we find the correct bucket for our key (i), and we search until we find our exact key, 
//meaning the K-V pair we search for, then we just return the value or 0 for a failed search
INLINE u32 H32_static_get(H32_static *h, u64 key)
{
	u32 i = key % h->n;
	while (h->keys[i] != key && h->keys[i] != HASH_UNUSED)
		i = (i + 1) % h->n;
	return h->keys[i] == HASH_UNUSED ? 0 : h->values[i];
}

INLINE void H32_static_free(H32_static *h)
{
    free(h->keys);
    free(h->values);
}



//djb2 hash for more info visit http://www.cse.yorku.ca/~oz/hash.html
INLINE u64 hash_str(char *s)
{
    unsigned long hash = 5381;
    int c;

    while (c = *s++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;

}

INLINE b32 H32_static_ok(void)
{
    H32_static h;
    H32_static_init(&h, 20);
    char *s1 = "Alex";
    char *s2 = "Ilias";
    char *s3 = "Leo";
    H32_static_set(&h, hash_str(s3), 21);
    H32_static_set(&h, hash_str(s2), 22);
    b32 res = (H32_static_get(&h, hash_str(s2)) == 22)&&
        (H32_static_get(&h, hash_str(s3)) == 21);
    H32_static_free(&h);
    return res;
}

#ifdef __cplusplus
}
#endif
INLINE char * 
read_whole_file_binary(char *filename, u32 *size)
{
    FILE *f = fopen(filename, "rb");
	
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
	*size = fsize;
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    char *string = (char*)malloc(fsize);
    fread(string, 1, fsize, f);
    fclose(f); 

    return (char*)string;
}


static int read_file(const char *path, u32 **buffer, u32 *word_count)
{
	long len;
	FILE *file = fopen(path, "rb");

	if (!file)
		return -1;

	fseek(file, 0, SEEK_END);
	len = ftell(file);
	rewind(file);

	*buffer = (u32*)malloc(len);
	if (fread(*buffer, 1, len, file) != (u32)len)
	{
		fclose(file);
		free(*buffer);
		printf("uhoh couldnt read some file :P\n");
		return -1;
	}

	fclose(file);
	*word_count = len;
	return 0;
}


static f64 r01(void)
{
    return (double)rand() / (double)RAND_MAX;
}

#endif










