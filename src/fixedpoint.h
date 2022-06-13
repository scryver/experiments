struct FixedPoint
{
    u32 pointAt;
    u32 digits[8];
};

internal FixedPoint
fixed_point(u32 integer, u32 fractional, u32 pointAt)
{
    FixedPoint result = {};
    result.pointAt = pointAt;
    u32 index = pointAt / 32;
    u32 offset = pointAt % 32;
    u32 invOffset = 32 - offset;
    i_expect(index < array_count(result.digits));
    result.digits[index] = (integer << offset) | (fractional >> invOffset);
    if (index > 0) {
        result.digits[index - 1] = fractional << offset;
    }
    if (index < (array_count(result.digits) - 1)) {
        result.digits[index + 1] = integer >> invOffset;
    }
    return result;
}

internal FixedPoint
operator -(FixedPoint a)
{
    FixedPoint result;
    result.pointAt = a.pointAt;

    u64 carry = 1;
    for (u32 digitIndex = 0;
         digitIndex < array_count(a.digits);
         ++digitIndex)
    {
        u64 temp = ~((u32)a.digits[digitIndex]) + carry;
        carry = temp >> 32;
        result.digits[digitIndex] = temp & U32_MAX;
    }
    return result;
}

internal FixedPoint &
operator +=(FixedPoint &a, FixedPoint b)
{
    u64 carry = 0;
    for (u32 digitIndex = 0;
         digitIndex < array_count(a.digits);
         ++digitIndex)
    {
        u64 temp = a.digits[digitIndex] + b.digits[digitIndex] + carry;
        carry = temp >> 32;
        a.digits[digitIndex] = temp & U32_MAX;
    }
    return a;
}

internal FixedPoint
operator +(FixedPoint a, FixedPoint b)
{
    FixedPoint result = a;
    result += b;
    return result;
}

internal FixedPoint &
operator -=(FixedPoint &a, FixedPoint b)
{
    u64 borrow = 0;
    for (u32 digitIndex = 0;
         digitIndex < array_count(a.digits);
         ++digitIndex)
    {
        u64 temp = a.digits[digitIndex] - b.digits[digitIndex] + borrow;
        borrow = temp >> 32;
        a.digits[digitIndex] = temp & U32_MAX;
    }
    return a;
}

internal FixedPoint
operator -(FixedPoint a, FixedPoint b)
{
    FixedPoint result = a;
    result -= b;
    return result;
}

#if 0
internal void
fast_shift_up(FixedPoint *a, u32 b)
{
    a->pointAt -= b;
}

internal void
fast_shift_down(FixedPoint *a, u32 b)
{
    a->pointAt += b;
}
#endif

internal FixedPoint &
operator <<=(FixedPoint &a, u32 b)
{
    u64 carry = 0;
    for (u32 digitIndex = 0;
         digitIndex < array_count(a.digits);
         ++digitIndex)
    {
        u64 temp = ((u64)a.digits[digitIndex] << b) | carry;
        carry = temp >> 32;
        a.digits[digitIndex] = temp & U32_MAX;
    }
    return a;
}

internal FixedPoint
operator <<(FixedPoint a, u32 b)
{
    FixedPoint result = a;
    result <<= b;
    return result;
}

internal FixedPoint &
operator >>=(FixedPoint &a, u32 b)
{
    b32 negative = a.digits[array_count(a.digits) - 1] & 0x80000000;
    u64 carry = 0;
    if (negative) {
        carry = ((1 << b) - 1) << (64 - b);
    }
    for (u32 digitIndex = array_count(a.digits);
         digitIndex > 0;
         --digitIndex)
    {
        u64 temp = ((u64)a.digits[digitIndex - 1] << (32 - b)) | carry;
        carry = (temp & U32_MAX) << 32;
        a.digits[digitIndex - 1] = temp >> 32;
    }
    return a;
}

internal FixedPoint
operator >>(FixedPoint a, u32 b)
{
    FixedPoint result = a;
    result >>= b;
    return result;
}

internal FixedPoint &
operator *=(FixedPoint &a, FixedPoint b)
{
    u64 carry = 0;
    for (u32 digitIndex = 0;
         digitIndex < array_count(a.digits);
         ++digitIndex)
    {
        u64 temp = a.digits[digitIndex] * b.digits[digitIndex] + carry;
        carry = temp >> 32;
        a.digits[digitIndex] = temp & U32_MAX;
    }
    a.pointAt -= (8 * 32 - b.pointAt);
    return a;
}

internal FixedPoint
operator *(FixedPoint a, FixedPoint b)
{
    FixedPoint result = a;
    result *= b;
    return result;
}

#if 0
internal FixedPoint &
operator /=(FixedPoint &a, FixedPoint b)
{
    u64 carry = 0;
    for (u32 digitIndex = array_count(a.digits);
         digitIndex > 0;
         ++digitIndex)
    {
        u64 temp = ((u64)a.digits[digitIndex - 1] << 32) / b.digits[digitIndex] + carry;
        carry = temp >> 32;
        a.digits[digitIndex - 1] = temp & U32_MAX;
    }
    a.pointAt += (8 * 32 - b.pointAt);
    return a;
}

internal FixedPoint
operator /(FixedPoint a, FixedPoint b)
{
    FixedPoint result = a;
    result /= b;
    return result;
}
#endif

internal FixedPoint
square(FixedPoint fp)
{
    FixedPoint result = fp;
    result *= fp;
    return result;
}

internal b32
operator <(FixedPoint a, FixedPoint b)
{
    b32 result = false;
    for (u32 digitIndex = array_count(a.digits);
         digitIndex > 0;
         --digitIndex)
    {
        if (a.digits[digitIndex - 1] < b.digits[digitIndex - 1])
        {
            result = true;
            break;
        }
        else if (a.digits[digitIndex - 1] > b.digits[digitIndex - 1])
        {
            break;
        }
    }
    return result;
}
