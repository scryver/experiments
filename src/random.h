inline u32
rotate_right(u32 value, s32 amount)
{
    amount &= 31;
    u32 result = ((value >> amount) | (value << (32 - amount)));
    return result;
}

struct RandomSeriesPCG
{
    u64 state;
    u64 selector;
};

inline RandomSeriesPCG
random_seed_pcg(u64 state, u64 selector)
{
     RandomSeriesPCG result;
    
    result.state = state;
    result.selector = (selector << 1) | 1;
    
    return result;
}

inline u32 
random_next_u32(RandomSeriesPCG *series)
{
    u64 state = series->state;
    state = state * 6364136223846793005ULL + series->selector;
    series->state = state;
    
    u32 preRotate = (u32)((state ^ (state >> 18)) >> 27);
    u32 result = rotate_right(preRotate, (s32)(state >> 59));
    
    return result;
}

inline u32
random_choice(RandomSeriesPCG *series, u32 choiceCount)
{
    u32 result = (random_next_u32(series) % choiceCount);
    return result;
}

inline f32
random_unilateral(RandomSeriesPCG *series)
{
    f32 result = (f32)random_next_u32(series) / (f32)U32_MAX;
    return result;
}

inline f32
random_bilateral(RandomSeriesPCG *series)
{
    f32 result = random_unilateral(series) * 2.0f - 1.0f;
    return result;
}

inline u32
slow_gaussian_choice(RandomSeriesPCG *series, u32 choiceCount, u32 gaussionCount = 8)
{
    u64 sum = 0;
    for (u32 i = 0; i < gaussionCount; ++i)
    {
        sum += (random_next_u32(series) % choiceCount);
    }
    sum /= gaussionCount;
    return sum;
}

inline f32
slow_gaussian(RandomSeriesPCG *series)
{
    f32 f1 = random_bilateral(series);
    f32 f2 = random_bilateral(series);
    
    f32 s = f1 * f1 + f2 * f2;
    while ((s >= 1.0f) || (s == 0.0f))
    {
        f1 = random_bilateral(series);
        f2 = random_bilateral(series);
        s = f1 * f1 + f2 * f2;
    }
    s = sqrt((-2.0f * log(s)) / s);
    return f1 * s;
}
