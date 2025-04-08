
struct v3_4x
{
    union
    {
        __m128 x;
        __m128 r;
    };

    union
    {
        __m128 y;
        __m128 g;
    };

    union
    {
        __m128 z;
        __m128 b;
    };
};

struct v4_4x
{
    union
    {
        __m128 x;
        __m128 r;
    };

    union
    {
        __m128 y;
        __m128 g;
    };

    union
    {
        __m128 z;
        __m128 b;
    };

    union
    {
        __m128 w;
        __m128 a;
    };
};

#define mmSquare(a) _mm_mul_ps(a, a)
#define M(a, i) ((float *)&(a))[i]
#define Mi(a, i) ((uint32_t *)&(a))[i]

inline v3_4x 
operator*(f32 As, v3_4x B)
{
    v3_4x Result;

    __m128 A = _mm_set1_ps(As);
    Result.x = _mm_mul_ps(A, B.x);
    Result.y = _mm_mul_ps(A, B.y);
    Result.z = _mm_mul_ps(A, B.z);

    return(Result);
}

inline v3_4x
operator+(v3_4x A, v3_4x B)
{
    v3_4x Result;

    Result.x = _mm_add_ps(A.x, B.x);
    Result.y = _mm_add_ps(A.y, B.y);
    Result.z = _mm_add_ps(A.z, B.z);

    return(Result);
}

inline v3_4x &
operator+=(v3_4x &A, v3_4x B)
{
    A.x = _mm_add_ps(A.x, B.x);
    A.y = _mm_add_ps(A.y, B.y);
    A.z = _mm_add_ps(A.z, B.z);

    return(A);
}

inline v4_4x 
operator*(f32 As, v4_4x B)
{
    v4_4x Result;

    __m128 A = _mm_set1_ps(As);
    Result.x = _mm_mul_ps(A, B.x);
    Result.y = _mm_mul_ps(A, B.y);
    Result.z = _mm_mul_ps(A, B.z);
    Result.w = _mm_mul_ps(A, B.w);

    return(Result);
}

inline v4_4x
operator+(v4_4x A, v4_4x B)
{
    v4_4x Result;

    Result.x = _mm_add_ps(A.x, B.x);
    Result.y = _mm_add_ps(A.y, B.y);
    Result.z = _mm_add_ps(A.z, B.z);
    Result.w = _mm_add_ps(A.w, B.w);

    return(Result);
}

inline v4_4x &
operator+=(v4_4x &A, v4_4x B)
{
    A.x = _mm_add_ps(A.x, B.x);
    A.y = _mm_add_ps(A.y, B.y);
    A.z = _mm_add_ps(A.z, B.z);
    A.w = _mm_add_ps(A.w, B.w);

    return(A);
}

inline v3_4x 
ToV34x(v3 A)
{
    v3_4x Result;

    Result.x = _mm_set1_ps(A.x);
    Result.y = _mm_set1_ps(A.y);
    Result.z = _mm_set1_ps(A.z);

    return(Result);
}

