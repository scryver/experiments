struct Complex64
{
    f64 real;
    f64 imag;
};

internal Complex64
complex(f64 real, f64 imag)
{
    Complex64 result;
    result.real = real;
    result.imag = imag;
    return result;
}

internal Complex64 &
operator +=(Complex64 &a, Complex64 b)
{
    a.real += b.real;
    a.imag += b.imag;
    return a;
}

internal Complex64
operator +(Complex64 a, Complex64 b)
{
    Complex64 result = a;
    result += b;
    return result;
}

internal Complex64 &
operator -=(Complex64 &a, Complex64 b)
{
    a.real -= b.real;
    a.imag -= b.imag;
    return a;
}

internal Complex64
operator -(Complex64 a, Complex64 b)
{
    Complex64 result = a;
    a -= b;
    return result;
}

internal Complex64
operator *(Complex64 a, Complex64 b)
{
    Complex64 result;
    result.real = a.real * b.real - a.imag * b.imag;
    result.imag = a.real * b.imag + a.imag * b.real;
    return result;
}

internal Complex64 &
operator *=(Complex64 &a, Complex64 b)
{
    a = a * b;
    return a;
}

internal Complex64 &
operator *=(Complex64 &a, f64 scalar)
{
    a.real *= scalar;
    a.imag *= scalar;
    return a;
}

internal Complex64
operator *(Complex64 a, f64 scalar)
{
    Complex64 result = a;
    result *= scalar;
    return result;
}

internal Complex64
operator *(f64 scalar, Complex64 a)
{
    return a * scalar;
}

internal Complex64
operator /(Complex64 a, Complex64 b)
{
    Complex64 result;
    f64 divisor = 1.0 / (square(b.real) + square(b.imag));
    result.real = (a.real * b.real + a.imag * b.imag) * divisor;
    result.imag = (a.imag * b.real - a.real * b.imag) * divisor;
    return result;
}

internal Complex64 &
operator /=(Complex64 &a, Complex64 b)
{
    a = a / b;
    return a;
}

internal Complex64
conjugate(Complex64 a)
{
    Complex64 result;
    result.real = a.real;
    result.imag = -a.imag;
    return result;
}

internal Complex64
euler_power(f64 imagPower)
{
    Complex64 result;
    result.real = cos(imagPower);
    result.imag = sin(imagPower);
    return result;
}

internal f64
absolute(Complex64 c)
{
    return square_root(square(c.real) + square(c.imag));
}

internal Complex64
square(Complex64 c)
{
    return c * c;
}



struct Complex32_4x
{
    f32_4x real;
    f32_4x imag;
};

internal Complex32_4x
complex(Complex32 c)
{
    Complex32_4x result;
    result.real = F32_4x(c.real);
    result.imag = F32_4x(c.imag);
    return result;
}

internal Complex32_4x
complex(Complex32 c0, Complex32 c1, Complex32 c2, Complex32 c3)
{
    Complex32_4x result;
    result.real = F32_4x(c0.real, c1.real, c2.real, c3.real);
    result.imag = F32_4x(c0.imag, c1.imag, c2.imag, c3.imag);
    return result;
}

internal Complex32_4x
complex(f32_4x real, f32_4x imag)
{
    Complex32_4x result;
    result.real = real;
    result.imag = imag;
    return result;
}

internal Complex32_4x
operator +(Complex32_4x a, Complex32_4x b)
{
    Complex32_4x result;
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
    return result;
}

internal Complex32_4x &
operator +=(Complex32_4x &a, Complex32_4x b)
{
    a = a + b;
    return a;
}

internal Complex32_4x
operator -(Complex32_4x a, Complex32_4x b)
{
    Complex32_4x result;
    result.real = a.real - b.real;
    result.imag = a.imag - b.imag;
    return result;
}

internal Complex32_4x &
operator -=(Complex32_4x &a, Complex32_4x b)
{
    a = a - b;
    return a;
}

internal Complex32_4x
operator *(Complex32_4x a, Complex32_4x b)
{
    Complex32_4x result;
    result.real = a.real * b.real - a.imag * b.imag;
    result.imag = a.real * b.imag + a.imag * b.real;
    return result;
}

internal Complex32_4x &
operator *=(Complex32_4x &a, Complex32_4x b)
{
    a = a * b;
    return a;
}

internal Complex32_4x
operator *(Complex32_4x a, f32_4x scalar)
{
    Complex32_4x result;
    result.real = a.real * scalar;
    result.imag = a.imag * scalar;
    return result;
}

internal Complex32_4x &
operator *=(Complex32_4x &a, f32_4x scalar)
{
    a = a * scalar;
    return a;
}

internal Complex32_4x
operator *(f32_4x scalar, Complex32_4x a)
{
    return a * scalar;
}

internal Complex32_4x
operator /(Complex32_4x a, Complex32_4x b)
{
    Complex32_4x result;
    f32_4x divisor = F32_4x(1.0f) / (square(b.real) + square(b.imag));
    result.real = (a.real * b.real + a.imag * b.imag) * divisor;
    result.imag = (a.imag * b.real - a.real * b.imag) * divisor;
    return result;
}

internal Complex32_4x &
operator /=(Complex32_4x &a, Complex32_4x b)
{
    a = a / b;
    return a;
}

internal Complex32_4x
conjugate(Complex32_4x a)
{
    Complex32_4x result;
    result.real = a.real;
    result.imag = -a.imag;
    return result;
}

internal Complex32_4x
euler_power(f32_4x imagPower)
{
    Complex32_4x result;
    result.real = cos_f32_4x(imagPower);
    result.imag = sin_f32_4x(imagPower);
    return result;
}

internal f32_4x
absolute(Complex32_4x c)
{
    return square_root(square(c.real) + square(c.imag));
}

internal Complex32_4x
square(Complex32_4x c)
{
    return c * c;
}
