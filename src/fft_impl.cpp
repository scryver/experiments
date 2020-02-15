internal void
dft(u32 dftCount, f32 *signal, Complex32 *dftSignal)
{
    f32 oneOverCount = 1.0f / (f32)dftCount;
    for (u32 dftIndex = 0; dftIndex < dftCount; ++dftIndex)
    {
        Complex32 sum = {};
        f32 powerStep = -(f32)dftIndex * oneOverCount;
        for (u32 sampleIndex = 0; sampleIndex < dftCount; ++sampleIndex)
        {
            f32 inp = signal[sampleIndex];
            f32 k = powerStep * (f32)sampleIndex;
            sum.real = sum.real + inp * cos_f32(k);
            sum.imag = sum.imag + inp * sin_f32(k);
        }
        dftSignal[dftIndex] = sum;
    }
}

internal void
fft_recurse(u32 dftCount, f32 *signal, Complex32 *dftSignal, u32 step = 1)
{
    i_expect(is_pow2(dftCount));
    
    if (dftCount == 1)
    {
        dftSignal[0].real = signal[0];
        dftSignal[0].imag = 0;
    }
    else
    {
        u32 halfCount = dftCount / 2;
        fft_recurse(halfCount, signal, dftSignal, step * 2);
        fft_recurse(halfCount, signal + step, dftSignal + halfCount, step * 2);
        
        f32 oneOverCount = 1.0f / (f32)dftCount;
        for (u32 index = 0; index < halfCount; ++index)
        {
            Complex32 T;
            T.real = cos_f32(-(f32)index * oneOverCount);
            T.imag = sin_f32(-(f32)index * oneOverCount);
            
            Complex32 E = dftSignal[index];
            Complex32 O = dftSignal[index + halfCount];
            dftSignal[index] = E + T * O;
            dftSignal[index + halfCount] = E - T * O;
        }
    }
}

internal u32
reverse_bits(u32 b, u32 msb)
{
    u32 mask = (1 << msb) - 1;
    u32 result = b;
    --msb;
    for (b >>= 1; b; b>>= 1)
    {
        result <<= 1;
        result |= b & 1;
        --msb;
    }
    result <<= msb;
    return result & mask;
}

internal void
fft_iter(u32 dftCount, f32 *signal, Complex32 *dftSignal)
{
    i_expect(is_pow2(dftCount));
    i_expect(dftCount > 2);
    
    BitScanResult highBit = find_most_significant_set_bit(dftCount);
    for (u32 index = 0; index < dftCount; ++index)
    {
        u32 reversedIndex = reverse_bits(index, highBit.index);
        Complex32 *c = dftSignal + reversedIndex;
        c->real = signal[index + 0].y;
        c->imag = 0.0f;
    }
    u32 halfM = 1;
    u32 m = 2;
    while (m <= dftCount)
    {
        f32 oneOverM = 1.0f / (f32)m;
        Complex32 Wm;
        Wm.real = cos_f32(-oneOverM);
        Wm.imag = sin_f32(-oneOverM);
        
        for (u32 k = 0; k < dftCount; k += m)
        {
            Complex32 w;
            w.real = 1.0f;
            w.imag = 0.0f;
            for (u32 j = 0; j < halfM; ++j)
            {
                Complex32 E = dftSignal[k + j];
                Complex32 O = w * dftSignal[k + j + halfM];
                dftSignal[k + j] = E + O;
                dftSignal[k + j + halfM] = E - O;
                w = w * Wm;
            }
        }
        halfM = m;
        m <<= 1;
    }
}

