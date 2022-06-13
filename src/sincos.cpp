#include <stdio.h>
//#include <intrin.h>
#include <x86intrin.h>

#define internal static
typedef unsigned int u32;
typedef float f32;

#define F32_PI 3.141592653589793238462643383279502884197169399f

typedef union f32_4x
{
    __m128 m;
    f32 e[4];
} f32_4x;

typedef struct SinCos
{
    f32_4x sin;
    f32_4x cos;
} SinCos;

internal f32_4x
F32_4x(f32 f)
{
    f32_4x result;
    result.m = _mm_set1_ps(f);
    return result;
}

internal f32_4x
F32_4x(f32 f0, f32 f1, f32 f2, f32 f3)
{
    f32_4x result;
    result.m = _mm_setr_ps(f0, f1, f2, f3);
    return result;
}

internal f32_4x
zero_f32_4x(void)
{
    f32_4x result;
    result.m = _mm_setzero_ps();
    return result;
}

internal f32_4x
operator -(f32_4x f4)
{
    u32 signMask = (u32)(1 << 31);
    __m128 flipMask = _mm_set1_ps(*(f32 *)&signMask);
    f32_4x result;
    result.m = _mm_xor_ps(f4.m, flipMask);
    return result;
}

internal f32_4x
operator +(f32_4x a, f32_4x b)
{
    f32_4x result;
    result.m = _mm_add_ps(a.m, b.m);
    return result;
}

internal f32_4x
operator -(f32_4x a, f32_4x b)
{
    f32_4x result;
    result.m = _mm_sub_ps(a.m, b.m);
    return result;
}

internal f32_4x
operator *(f32_4x a, f32_4x b)
{
    f32_4x result;
    result.m = _mm_mul_ps(a.m, b.m);
    return result;
}

internal f32_4x
operator ==(f32_4x a, f32_4x b)
{
    f32_4x result;
    result.m = _mm_cmpeq_ps(a.m, b.m);
    return result;
}

internal f32_4x
operator >(f32_4x a, f32_4x b)
{
    f32_4x result;
    result.m = _mm_cmpgt_ps(a.m, b.m);
    return result;
}

internal f32_4x
operator &(f32_4x a, f32_4x b)
{
    f32_4x result;
    result.m = _mm_and_ps(a.m, b.m);
    return result;
}

internal f32_4x
operator |(f32_4x a, f32_4x b)
{
    f32_4x result;
    result.m = _mm_or_ps(a.m, b.m);
    return result;
}

internal f32_4x
floor(f32_4x f4)
{
    f32_4x result;
    result.m = _mm_floor_ps(f4.m);
    return result;
}

internal f32_4x
fraction(f32_4x a)
{
    f32_4x result;
    result = a - floor(a);
    return result;
}

internal f32_4x
and_not(f32_4x a, f32_4x b)
{
    // NOTE(michiel): Returns a & ~b
    f32_4x result;
    result.m = _mm_andnot_ps(b.m, a.m);
    return result;
}

internal f32_4x
square(f32_4x f4)
{
    f32_4x result;
    result = f4 * f4;
    return result;
}

internal f32_4x
select(f32_4x op0, f32_4x mask, f32_4x op1)
{
    f32_4x result;
    result.m = _mm_or_ps(_mm_andnot_ps(mask.m, op0.m),
                         _mm_and_ps(mask.m, op1.m));
    return result;
}

internal SinCos
sin_cos(f32_4x angles)
{
    // NOTE(michiel): Angles are in radians / 2pi, so instead of a range of [0, 2pi] it has a range of [0, 1],
    // this simplifies the argument reduction scheme and pushes the burden of big values upon the user.
    // See https://www.desmos.com/calculator/uilrk0fvzk for the used splitting of the two functions.
    // Only a small range around 0 for both approximations is used.

    // NOTE(michiel): Constants
    f32_4x eights_4x = F32_4x(0.125f);
    f32_4x one_4x    = F32_4x(1.0f);
    f32_4x two_4x    = F32_4x(2.0f);
    f32_4x three_4x  = F32_4x(3.0f);
    f32_4x four_4x   = F32_4x(4.0f);
    f32_4x half_4x   = F32_4x(0.5f);
    f32_4x oneAndHalf_4x = one_4x + half_4x;
    f32_4x twoAndHalf_4x = two_4x + half_4x;

    angles = angles + eights_4x;
    angles = fraction(angles);
    angles = angles - eights_4x;
    angles = angles * four_4x;

    f32_4x quadrant4 = angles > twoAndHalf_4x;
    f32_4x quadrant3 = angles > oneAndHalf_4x;
    f32_4x quadrant2 = angles > half_4x;

    quadrant2 = and_not(quadrant2, quadrant3);
    quadrant3 = and_not(quadrant3, quadrant4);

    f32_4x quadrants = (three_4x & quadrant4) | (two_4x & quadrant3) | (one_4x & quadrant2);
    angles = angles - quadrants;

    f32_4x do_cos_mask = quadrant3 | (quadrants == zero_f32_4x());
    f32_4x do_invc_mask = quadrant2 | quadrant3;
    f32_4x do_invs_mask = quadrant3 | quadrant4;

    SinCos sincos;
    // SinCos sincos_f32_4x_approx_small(f32_4x angles)
    {
        // NOTE(michiel): Valid for angles between -0.5 and 0.5
        f32_4x halfPi_4x = F32_4x(0.5f * F32_PI);

        // cos(x) = 1 - (x^2/2!) + (x^4/4!) - (x^6/6!) + (x^8/8!)
        // cosA(x) = x^2/2! + x^6/6! = x^2 * (1/2! + x^4/6!)
        // cosB(x) = x^4/4! + x^8/8! = x^4 * (1/4! + x^4/8!)
        // cos(x) = 1 - cosA(x) + cosB(x)
        f32_4x c0 = F32_4x(1.2337005501361697490381175157381221652030944824f); // NOTE(michiel): (pi^2 /   4) / 2!
        f32_4x c1 = F32_4x(0.2536695079010479747516626503056613728404045105f); // NOTE(michiel): (pi^4 /  16) / 4!
        f32_4x c2 = F32_4x(0.0208634807633529574533159944849103339947760105f); // NOTE(michiel): (pi^6 /  64) / 6!
        f32_4x c3 = F32_4x(0.0009192602748394262598963244670358108123764396f); // NOTE(michiel): (pi^8 / 256) / 8!

        // sin(x) = x - (x^3/3!) + (x^5/5!) - (x^7/7!) + (x^9/9!)
        // sin(x) = x * (1 - (x^2/3!) + (x^4/5!) - (x^6/7!) + (x^8/9!)
        // sinA(x) = x^2/3! + x^6/7! = x^2 * (1/3! + x^4/7!)
        // sinB(x) = x^4/5! + x^8/9! = x^4 * (1/5! + x^4/9!)
        // sin(x) = x * (1 - sinA(x) + sinB(x))
        f32_4x s0 = F32_4x(0.4112335167120566015164229156653163954615592956f); // NOTE(michiel): (pi^2 /   4) / 3!
        f32_4x s1 = F32_4x(0.0507339015802095935625537492796865990385413170f); // NOTE(michiel): (pi^4 /  16) / 5!
        f32_4x s2 = F32_4x(0.0029804972519075654743825332104734116001054644f); // NOTE(michiel): (pi^6 /  64) / 7!
        f32_4x s3 = F32_4x(0.0001021400305377140213481876318546426318789599f); // NOTE(michiel): (pi^8 / 256) / 9!

        f32_4x angSq = square(angles);
        f32_4x angQd = square(square(angles));

        f32_4x cosA = angSq * (c0 + angQd * c2);
        f32_4x cosB = angQd * (c1 + angQd * c3);
        f32_4x sinA = angSq * (s0 + angQd * s2);
        f32_4x sinB = angQd * (s1 + angQd * s3);

        sincos.cos = one_4x - cosA + cosB;
        sincos.sin = halfPi_4x * angles * (one_4x - sinA + sinB);
    }

    SinCos result;
    result.cos = select(sincos.sin, do_cos_mask, sincos.cos);
    result.cos = select(result.cos, do_invc_mask, -result.cos);
    result.sin = select(sincos.cos, do_cos_mask, sincos.sin);
    result.sin = select(result.sin, do_invs_mask, -result.sin);

    return result;
}

int main(int argc, char **argv)
{
    // NOTE(michiel): Compile with one of (only tested on linux):
    // clang++ -msse4 src/sincos.cpp
    // g++ -msse4 src/sincos.cpp
    f32_4x angles0 = F32_4x(0.0f, 0.25f, 0.5f, 0.75f);
    f32_4x angles1 = F32_4x(1.0f, 0.125f, 0.08333333333f, 0.1666666667f);

    SinCos sinCos0 = sin_cos(angles0);
    SinCos sinCos1 = sin_cos(angles1);
    printf("Sin(%f) = %f, Cos(%f) = %f\n", angles0.e[0], sinCos0.sin.e[0], angles0.e[0], sinCos0.cos.e[0]);
    printf("Sin(%f) = %f, Cos(%f) = %f\n", angles0.e[1], sinCos0.sin.e[1], angles0.e[1], sinCos0.cos.e[1]);
    printf("Sin(%f) = %f, Cos(%f) = %f\n", angles0.e[2], sinCos0.sin.e[2], angles0.e[2], sinCos0.cos.e[2]);
    printf("Sin(%f) = %f, Cos(%f) = %f\n", angles0.e[3], sinCos0.sin.e[3], angles0.e[3], sinCos0.cos.e[3]);
    printf("Sin(%f) = %f, Cos(%f) = %f\n", angles1.e[0], sinCos1.sin.e[0], angles1.e[0], sinCos1.cos.e[0]);
    printf("Sin(%f) = %f, Cos(%f) = %f\n", angles1.e[1], sinCos1.sin.e[1], angles1.e[1], sinCos1.cos.e[1]);
    printf("Sin(%f) = %f, Cos(%f) = %f\n", angles1.e[2], sinCos1.sin.e[2], angles1.e[2], sinCos1.cos.e[2]);
    printf("Sin(%f) = %f, Cos(%f) = %f\n", angles1.e[3], sinCos1.sin.e[3], angles1.e[3], sinCos1.cos.e[3]);

    return 0;
}
