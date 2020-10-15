#ifndef JIAA_MATH_H
#define JIAA_MATH_H

// various math


#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif


#ifndef M_PI_F
#define M_PI_F		((float)(M_PI))
#endif

#ifndef DEG2RAD
#define DEG2RAD( x  )  ( (float)(x) * (float)(M_PI_F / 180.f) )
#endif

#ifndef RAD2DEG
#define RAD2DEG( x  )  ( (float)(x) * (float)(180.f / M_PI_F) )
#endif

struct Vector
{
    float x, y, z;
};

static inline float VectorDotProduct( const struct Vector *a, const struct Vector *b )
{
    return ( a->x * b->x ) + ( a->y * b->y ) + ( a->z * b->z );
}

struct Vector2D
{
    float x, y;
};

struct ViewMatrix
{
    float m[4][4];
};

static inline struct ViewMatrix MatrixTranspose( const struct ViewMatrix *matrix )
{
    struct ViewMatrix result;
    result.m[0][0] = matrix->m[0][0]; result.m[0][1] = matrix->m[1][0]; result.m[0][2] = matrix->m[2][0]; result.m[0][3] = matrix->m[3][0];
    result.m[1][0] = matrix->m[0][1]; result.m[1][1] = matrix->m[1][1]; result.m[1][2] = matrix->m[2][1]; result.m[1][3] = matrix->m[3][1];
    result.m[2][0] = matrix->m[0][2]; result.m[2][1] = matrix->m[1][2]; result.m[2][2] = matrix->m[2][2]; result.m[2][3] = matrix->m[3][2];
    result.m[3][0] = matrix->m[0][3]; result.m[3][1] = matrix->m[1][3]; result.m[3][2] = matrix->m[2][3]; result.m[3][3] = matrix->m[3][3];

    return result;
}

#endif //JIAA_MATH_H
