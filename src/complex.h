struct Complex32
{
    f32 real;
    f32 imag;
};

internal Complex32
complex(f32 real, f32 imag)
{
    Complex32 result;
    result.real = real;
    result.imag = imag;
    return result;
}

internal Complex32 &
operator +=(Complex32 &a, Complex32 b)
{
    a.real += b.real;
    a.imag += b.imag;
    return a;
}

internal Complex32
operator +(Complex32 a, Complex32 b)
{
    Complex32 result = a;
    result += b;
    return result;
}

internal Complex32 &
operator -=(Complex32 &a, Complex32 b)
{
    a.real -= b.real;
    a.imag -= b.imag;
    return a;
}

internal Complex32
operator -(Complex32 a, Complex32 b)
{
    Complex32 result = a;
    a -= b;
    return result;
}

internal Complex32
operator *(Complex32 a, Complex32 b)
{
    Complex32 result;
    result.real = a.real * b.real - a.imag * b.imag;
    result.imag = a.real * b.imag + a.imag * b.real;
    return result;
}

internal Complex32 &
operator *=(Complex32 &a, Complex32 b)
{
    a = a * b;
    return a;
}

internal Complex32 &
operator *=(Complex32 &a, f32 scalar)
{
    a.real *= scalar;
    a.imag *= scalar;
    return a;
}

internal Complex32
operator *(Complex32 a, f32 scalar)
{
    Complex32 result = a;
    result *= scalar;
    return result;
}

internal Complex32
operator *(f32 scalar, Complex32 a)
{
    return a * scalar;
}

internal Complex32
operator /(Complex32 a, Complex32 b)
{
    Complex32 result;
    f32 divisor = 1.0f / (square(b.real) + square(b.imag));
    result.real = (a.real * b.real + a.imag * b.imag) * divisor;
    result.imag = (a.imag * b.real - a.real * b.imag) * divisor;
    return result;
}

internal Complex32 &
operator /=(Complex32 &a, Complex32 b)
{
    a = a / b;
    return a;
}

internal Complex32
conjugate(Complex32 a)
{
    Complex32 result;
    result.real = a.real;
    result.imag = -a.imag;
    return result;
}

internal Complex32
euler_power(f32 imagPower)
{
    Complex32 result;
    result.real = cos(imagPower);
    result.imag = sin(imagPower);
    return result;
}

internal f32
abs(Complex32 c)
{
    return square_root(square(c.real) + square(c.imag));
}

internal Complex32
square(Complex32 c)
{
    return c * c;
}

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
abs(Complex64 c)
{
    return square_root(square(c.real) + square(c.imag));
}

internal Complex64
square(Complex64 c)
{
    return c * c;
}
