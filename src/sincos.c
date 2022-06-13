#include <stdio.h>
//#include <intrin.h>
#include <x86intrin.h>

#define internal static
typedef unsigned int u32;
typedef float f32;

#define F32_PI 3.141592653589793238462643383279502884197169399f

typedef struct SinCos
{
    __m128 sin;
    __m128 cos;
} SinCos;

internal SinCos
sin_cos(__m128 angles)
{
    // NOTE(michiel): Angles are in radians / 2pi, so instead of a range of [0, 2pi] it has a range of [0, 1],
    // this simplifies the argument reduction scheme and pushes the burden of big values upon the user.
    // See https://www.desmos.com/calculator/uilrk0fvzk for the used splitting of the two functions.
    // Only a small range around 0 for both approximations is used.

    // NOTE(michiel): Constants
    __m128 eights_4x = _mm_set1_ps(0.125f);
    __m128 one_4x = _mm_set1_ps(1.0f);
    __m128 two_4x = _mm_set1_ps(2.0f);
    __m128 three_4x = _mm_set1_ps(3.0f);
    __m128 four_4x = _mm_set1_ps(4.0f);
    __m128 half_4x = _mm_set1_ps(0.5f);
    __m128 oneAndHalf_4x = _mm_set1_ps(1.5f);
    __m128 twoAndHalf_4x = _mm_set1_ps(2.5f);
    u32 signBit = (u32)(1 << 31);
    __m128 signMask = _mm_set1_ps(*(f32 *)&signBit);

    angles = _mm_add_ps(angles, eights_4x);
    angles = _mm_sub_ps(angles,_mm_floor_ps(angles));
    angles = _mm_sub_ps(angles, eights_4x);
    angles = _mm_mul_ps(angles, four_4x);

    __m128 quadrant4 = _mm_cmpgt_ps(angles, twoAndHalf_4x);
    __m128 quadrant3 = _mm_cmpgt_ps(angles, oneAndHalf_4x);
    __m128 quadrant2 = _mm_cmpgt_ps(angles, half_4x);

    __m128 do_invs_mask = quadrant3;
    __m128 do_invc_mask = _mm_andnot_ps(quadrant4, quadrant2);

    quadrant2 = _mm_andnot_ps(quadrant3, quadrant2);
    quadrant3 = _mm_andnot_ps(quadrant4, quadrant3);

    __m128 quadrants = _mm_or_ps(_mm_or_ps(_mm_and_ps(quadrant4, three_4x),
                                           _mm_and_ps(quadrant3, two_4x)),
                                 _mm_and_ps(quadrant2, one_4x));
    angles = _mm_sub_ps(angles, quadrants);

    __m128 do_cos_mask = _mm_or_ps(quadrant3, _mm_cmpeq_ps(quadrants, _mm_setzero_ps()));

    SinCos sincos;
    // SinCos sincos_approx_small(__m128 angles)
    {
        // NOTE(michiel): Valid for angles between -0.5 and 0.5
        __m128 halfPi_4x = _mm_set1_ps(0.5f * F32_PI);

        // cos(x) = 1 - (x^2/2!) + (x^4/4!) - (x^6/6!) + (x^8/8!)
        // cosA(x) = x^2/2! + x^6/6! = x^2 * (1/2! + x^4/6!)
        // cosB(x) = x^4/4! + x^8/8! = x^4 * (1/4! + x^4/8!)
        // cos(x) = 1 - cosA(x) + cosB(x)
        __m128 c0 = _mm_set1_ps(1.2337005501361697490381175157381221652030944824f); // NOTE(michiel): (pi^2 /   4) / 2!
        __m128 c1 = _mm_set1_ps(0.2536695079010479747516626503056613728404045105f); // NOTE(michiel): (pi^4 /  16) / 4!
        __m128 c2 = _mm_set1_ps(0.0208634807633529574533159944849103339947760105f); // NOTE(michiel): (pi^6 /  64) / 6!
        __m128 c3 = _mm_set1_ps(0.0009192602748394262598963244670358108123764396f); // NOTE(michiel): (pi^8 / 256) / 8!

        // sin(x) = x - (x^3/3!) + (x^5/5!) - (x^7/7!) + (x^9/9!)
        // sin(x) = x * (1 - (x^2/3!) + (x^4/5!) - (x^6/7!) + (x^8/9!)
        // sinA(x) = x^2/3! + x^6/7! = x^2 * (1/3! + x^4/7!)
        // sinB(x) = x^4/5! + x^8/9! = x^4 * (1/5! + x^4/9!)
        // sin(x) = x * (1 - sinA(x) + sinB(x))
        __m128 s0 = _mm_set1_ps(0.4112335167120566015164229156653163954615592956f); // NOTE(michiel): (pi^2 /   4) / 3!
        __m128 s1 = _mm_set1_ps(0.0507339015802095935625537492796865990385413170f); // NOTE(michiel): (pi^4 /  16) / 5!
        __m128 s2 = _mm_set1_ps(0.0029804972519075654743825332104734116001054644f); // NOTE(michiel): (pi^6 /  64) / 7!
        __m128 s3 = _mm_set1_ps(0.0001021400305377140213481876318546426318789599f); // NOTE(michiel): (pi^8 / 256) / 9!

        __m128 angSq = _mm_mul_ps(angles, angles);
        __m128 angQd = _mm_mul_ps(angSq, angSq);

        __m128 cosA = _mm_mul_ps(angSq, _mm_add_ps(c0, _mm_mul_ps(angQd, c2)));
        __m128 cosB = _mm_mul_ps(angQd, _mm_add_ps(c1, _mm_mul_ps(angQd, c3)));
        __m128 sinA = _mm_mul_ps(angSq, _mm_add_ps(s0, _mm_mul_ps(angQd, s2)));
        __m128 sinB = _mm_mul_ps(angQd, _mm_add_ps(s1, _mm_mul_ps(angQd, s3)));

        sincos.cos = _mm_add_ps(_mm_sub_ps(one_4x, cosA), cosB);
        sincos.sin = _mm_mul_ps(halfPi_4x, _mm_mul_ps(angles, _mm_add_ps(_mm_sub_ps(one_4x, sinA), sinB)));
    }

    SinCos result;
    result.cos = _mm_or_ps(_mm_andnot_ps(do_cos_mask, sincos.sin),
                           _mm_and_ps(do_cos_mask, sincos.cos));
    result.cos = _mm_or_ps(_mm_andnot_ps(do_invc_mask, result.cos),
                           _mm_and_ps(do_invc_mask, _mm_xor_ps(result.cos, signMask)));

    result.sin = _mm_or_ps(_mm_andnot_ps(do_cos_mask, sincos.cos),
                           _mm_and_ps(do_cos_mask, sincos.sin));
    result.sin = _mm_or_ps(_mm_andnot_ps(do_invs_mask, result.sin),
                           _mm_and_ps(do_invs_mask, _mm_xor_ps(result.sin, signMask)));

    return result;
}

int main(int argc, char **argv)
{
    // NOTE(michiel): Compile with one of (only tested on linux):
    // clang -msse4 sincos.c
    // gcc -msse4 sincos.c
    f32 angles[] = {
        0.0f, 0.25f, 0.5f, 0.75f,
        1.0f, 0.125f, 0.083333333333f, 0.1666666667f,
    };
    __m128 angles0 = _mm_loadu_ps(angles);
    __m128 angles1 = _mm_loadu_ps(angles + 4);

    SinCos sinCos0 = sin_cos(angles0);
    SinCos sinCos1 = sin_cos(angles1);
    f32 sinCosDump[16];

    _mm_storeu_ps(sinCosDump +  0, sinCos0.sin);
    _mm_storeu_ps(sinCosDump +  4, sinCos0.cos);
    _mm_storeu_ps(sinCosDump +  8, sinCos1.sin);
    _mm_storeu_ps(sinCosDump + 12, sinCos1.cos);

    printf("Sin(% f) = % f, Cos(% f) = % f\n", angles[0], sinCosDump[0], angles[0], sinCosDump[4]);
    printf("Sin(% f) = % f, Cos(% f) = % f\n", angles[1], sinCosDump[1], angles[1], sinCosDump[5]);
    printf("Sin(% f) = % f, Cos(% f) = % f\n", angles[2], sinCosDump[2], angles[2], sinCosDump[6]);
    printf("Sin(% f) = % f, Cos(% f) = % f\n", angles[3], sinCosDump[3], angles[3], sinCosDump[7]);
    printf("Sin(% f) = % f, Cos(% f) = % f\n", angles[4], sinCosDump[ 8], angles[4], sinCosDump[12]);
    printf("Sin(% f) = % f, Cos(% f) = % f\n", angles[5], sinCosDump[ 9], angles[5], sinCosDump[13]);
    printf("Sin(% f) = % f, Cos(% f) = % f\n", angles[6], sinCosDump[10], angles[6], sinCosDump[14]);
    printf("Sin(% f) = % f, Cos(% f) = % f\n", angles[7], sinCosDump[11], angles[7], sinCosDump[15]);

    return 0;
}
